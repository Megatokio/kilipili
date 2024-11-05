// Copyright (c) 1995 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause
//
// YM file register extractor and interleaver
// based on YM file register extractor and interleaver by Daniel Tufvesson 2014
// based on YM file format specification by Arnaud Carré

// http://www.waveguide.se/?article=32&file=ymextract.c
// http://www.waveguide.se/?article=ym-playback-on-the-ymz284
// https://github.com/Andy4495/AY3891x/tree/main/extras/tools
// https://github.com/nguillaumin/ym-jukebox/tree/master/data
// https://www.genesis8bit.fr/frontend/music.php
// http://leonard.oxg.free.fr/			(incomplete format description)
// https://github.com/arnaud-carre/StSound/tree/main/StSoundLibrary

#include "YMFileConverter.h"
#include "Audio/Ay38912.h"
#include "CompressedRsrcFileWriter.h"
#include "LzhDecoder.h"
#include "cdefs.h"
#include "cstrings.h"
#include "extern/StSoundLibrary/StSoundLibrary.h"
#include "standard_types.h"
#include <cstring>
#include <dirent.h>
#include <endian.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>

namespace kio::Audio
{

//constexpr uint8 ayRegisterBits[] = {8, 4, 8, 4, 8, 4, 5, 8, 5, 5, 5, 8, 8, 4, 0, 0};

//constexpr uint8 ayRegisterBitMasks[] = {0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0xff,
//										0x1f, 0x1f, 0x1f, 0xff, 0xff, 0x0f, 0xff, 0xff};
//
//constexpr uint8 ayRegisterResetValues[] = {0xff, 0x0f, 0xff, 0x0f, 0xff, 0x0f, 0x1f, 0x3f,
//										   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff};

static uint32 readBE32(uchar** ptr)
{
	uint32 z;
	memcpy(&z, *ptr, 4);
	*ptr += 4;
	return be32toh(z);
}

static uint16 readBE16(uchar** ptr)
{
	uint16 z;
	memcpy(&z, *ptr, 2);
	*ptr += 2;
	return be16toh(z);
}

static cstr readString(uchar** ptr)
{
	cptr p = cptr(*ptr);
	*ptr += strlen(p) + 1;
	return dupstr(p);
}

static void storeBE32(uchar** ptr, uint32 z)
{
	z = htobe32(z);
	memcpy(*ptr, &z, 4);
	*ptr += 4;
}

static void storeBE16(uchar** ptr, uint16 z)
{
	z = htobe16(z);
	memcpy(*ptr, &z, 2);
	*ptr += 2;
}

static void storeString(uchar** ptr, cstr s, uint slen)
{
	memcpy(*ptr, s, slen);
	*ptr += slen;
}

static uint32 getFileSize(FILE* fp)
{
	long fSize;
	long oPos;
	oPos = ftell(fp);
	fseek(fp, 0, SEEK_END);
	fSize = ftell(fp);
	fseek(fp, oPos, SEEK_SET);
	assert_eq(fSize, uint32(fSize));
	return uint32(fSize);
}

void YMFileConverter::dispose_all()
{
	if (file) fclose(file);
	file = nullptr;
	delete[] register_data;
	register_data = nullptr;
}

void YMFileConverter::interleave_registers()
{
	if ((attributes & NotInterleaved) != NotInterleaved) return;

	uint8* zbu = new uint8[num_frames * 16];
	memset(zbu, 0xff, num_frames * 16); //clear port A & B

	uint8* q = register_data;
	for (uint reg = 0; reg < frame_size; reg++)
	{
		for (uint frame = 0; frame < num_frames; frame++)
		{
			zbu[frame * 16 + reg] = *q++; //
		}
	}

	delete[] register_data;
	register_data = zbu;

	attributes -= NotInterleaved;
	frame_size = 16;
	if (file_type < YM5) file_type = YM5;
}

struct __packed LzhHeader
{
	uint8  size; // of header
	uint8  sum;
	char   id[5]; // "-lh5-"
	uint32 packed_size;
	uint32 original_size;
	uint8  reserved[5];
	uint8  level;
	uint8  name_length;
	// char name[];
	// uint16 crc;
};
static_assert(sizeof(LzhHeader) == 22);

void YMFileConverter::unpack_lzh_data(uint8*& io_data, uint32& io_size)
{
	auto* header = reinterpret_cast<LzhHeader*>(io_data);
	if (memcmp(header->id, "-lh5-", 5) != 0) return; // probably uncompressed
	if (header->size != sizeof(LzhHeader) + header->name_length) throw "LZH: wrong header size";
	if (le32toh(header->packed_size) + header->size + 2 > io_size) throw "LZH: file truncated";

	std::unique_ptr<LzhDecoder> decoder {new LzhDecoder}; // the decoder is large => allocate on heap

	uint32 qsize = le32toh(header->packed_size);
	uint32 zsize = le32toh(header->original_size);
	uint8* qbu	 = io_data;
	uint8* zbu	 = new uint8[zsize];
	io_data		 = zbu;
	io_size		 = zsize;
	bool ok		 = decoder->unpack(qbu + header->size + 2, qsize, zbu, zsize);
	delete[] qbu;
	if (!ok) throw "LZH: unpack error";
}

uint YMFileConverter::store_ym5ym6_header(uint8* bu, uint bu_size)
{
	uint tlen = strlen(title) + 1;
	uint alen = strlen(author) + 1;
	uint clen = strlen(comment) + 1;
	uint len  = uint(34 + tlen + alen + clen);
	if (len > bu_size) throw "comments too long";

	uint8* p = bu;
	storeString(&p, file_type == YM5 ? "YM5!LeOnArD!" : "YM6!LeOnArD!", 12);
	storeBE32(&p, num_frames);
	storeBE32(&p, attributes);
	storeBE16(&p, 0); //drums
	storeBE32(&p, ay_clock);
	storeBE16(&p, frame_rate);
	storeBE32(&p, loop_frame);
	storeBE16(&p, 0); //skip
	storeString(&p, title, tlen);
	storeString(&p, author, alen);
	storeString(&p, comment, clen);
	return len;
}

void YMFileConverter::write_ym5ym6_header()
{
	uint8 bu[200];
	uint  len = store_ym5ym6_header(bu, sizeof(bu));
	fwrite(bu, 1, len, file);
}

uint32 YMFileConverter::write_raw_audio_file(FILE* file, float sample_rate)
{
	// returns number of samples written

	hw_sample_frequency = sample_rate;
	Ay38912<1>	ay(float(ay_clock), Ay38912<1>::mono, 1.0f);
	uint32		samples_per_frame = uint32(hw_sample_frequency / frame_rate + 0.5f);
	MonoSample* sample_buffer	  = new MonoSample[samples_per_frame];

	struct Filter
	{
		int	 center {0};
		int	 sample {0};
		void filter(Sample* buffer, uint num_frames)
		{
			for (uint i = 0; i < num_frames; i += 1)
			{
				sample = (sample + buffer[i]) >> 1;
				sample -= center;
				if (sample != Sample(sample))
				{
					center += sample;
					sample = sample < 0 ? -0x8000 : +0x7fff;
					center -= sample;
				}
				center += sample >> 11;
				buffer[i] = Sample(sample);
			}
		}
	} filter;

	// f=0: interleaved: register_data[r * 1 + frame * 16]
	// f=1: sequential:  register_data[r * num_frames + frame * 1]
	bool f	= attributes & NotInterleaved;
	uint rf = f ? num_frames : 1;
	uint ff = f ? 1 : frame_size;

	for (uint32 frame = 0; frame < num_frames; frame++)
	{
		for (uint r = 0; r < 13; r++) ay.setRegister(r, register_data[r * rf + frame * ff]);
		uint8 env = register_data[13 * rf + frame * ff];
		if (env != 0xff) ay.setRegister(13, env);

		ay.audioBufferStart(sample_buffer, samples_per_frame);
		ay.audioBufferEnd();
		filter.filter(&sample_buffer[0].m, samples_per_frame);

		if constexpr (__BYTE_ORDER != __LITTLE_ENDIAN)
		{
			static_assert(sizeof(MonoSample) == 2);
			uint16* bu = uint16ptr(sample_buffer);
			for (uint i = 0; i < samples_per_frame; i++) bu[i] = htole16(bu[i]);
		}

		fwrite(sample_buffer, sizeof(MonoSample), samples_per_frame, file);
	}

	delete[] sample_buffer;

	return num_frames * samples_per_frame;
}

struct WAVHeader
{
	uint8  RIFFMagic[4]	 = {'R', 'I', 'F', 'F'};
	uint32 FileLength	 = 0; // length of data that follows
	uint8  FileType[4]	 = {'W', 'A', 'V', 'E'};
	uint8  FormMagic[4]	 = {'f', 'm', 't', ' '};
	uint32 FormLength	 = htole32(0x10);
	uint16 SampleFormat	 = htole16(1);
	uint16 NumChannels	 = htole16(1);
	uint32 PlayRate		 = htole32(44100);
	uint32 BytesPerSec	 = htole32(44100 * (16 / 8));
	uint16 Pad			 = htole16(16 / 8);
	uint16 BitsPerSample = htole16(16);
	uint8  DataMagic[4]	 = {'d', 'a', 't', 'a'};
	uint32 DataLength	 = 0;
};

uint32 YMFileConverter::exportWavFile(cstr fpath, float sample_rate)
{
	if ((file = fopen(fpath, "wb")) == NULL) throw "Unable to open output file";

	WAVHeader header;
	fwrite(&header, 1, sizeof(WAVHeader), file); // reserve space
	uint32 num_samples = write_raw_audio_file(file, sample_rate);

	long fsize = ftell(file);
	fseek(file, 0, SEEK_SET); // rewind file
	header.PlayRate	   = htole32(uint32(sample_rate));
	header.BytesPerSec = htole32(uint32(sample_rate) * (16 / 8));
	header.DataLength  = htole32(num_samples * (16 / 8));
	header.FileLength  = htole32(num_samples * (16 / 8) + sizeof(WAVHeader) - 8);
	fwrite(&header, 1, sizeof(WAVHeader), file);

	fclose(file);
	file = nullptr;
	return uint32(fsize);
}

uint32 YMFileConverter::exportYMRegisterFile(cstr fpath)
{
	// this file contains uncompressed interleaved registers, 14 per frame.

	if ((file = fopen(fpath, "wb")) == NULL) throw "Unable to open output file";

	fputs("ymrf", file);	 // file ID
	fputc(0, file);			 // format variant
	fputc(0, file);			 // flags
	fputc(frame_rate, file); //
	fputc(14, file);		 // frame size
	uint32 bu[3] = {htole32(num_frames), htole32(loop_frame), htole32(ay_clock)};
	fwrite(bu, 4, 3, file);
	fputs(title, file);
	fputc(0, file);
	fputs(author, file);
	fputc(0, file);
	fputs(comment, file);
	fputc(0, file);

	// f=0: interleaved: register_data[r * 1 + frame * 16]
	// f=1: sequential:  register_data[r * num_frames + frame * 1]
	bool f	= attributes & NotInterleaved;
	uint rf = f ? num_frames : 1;
	uint ff = f ? 1 : frame_size;

	for (uint32 frame = 0; frame < num_frames; frame++)
	{
		for (uint reg = 0; reg < 14; reg++) fputc(register_data[reg * rf + frame * ff], file);
	}

	ssize_t size = ftell(file);
	fclose(file);
	file = nullptr;
	return uint32(size);
}

void YMFileConverter::exportYMMusicWavFile(cstr filename, cstr destfile)
{
	// TODO load data from data[] (needs header again)

	YMMUSIC* pMusic = ymMusicCreate();

	if (ymMusicLoad(pMusic, filename) == false)
	{
		auto err = ymMusicGetLastError(pMusic);
		ymMusicDestroy(pMusic);
		throw usingstr("Error in loading file %s:\n%s\n", filename, err);
	}

	ymMusicInfo_t info;
	ymMusicGetInfo(pMusic, &info);

	printf("Generating wav file from \"%s\"\n", filename);
	printf("%s\n%s\n(%s)\n", info.pSongName, info.pSongAuthor, info.pSongComment);
	printf("Total music time: %d seconds.\n", info.musicTimeInSec);

	FILE* out = fopen(destfile, "wb");
	if (!out)
	{
		ymMusicDestroy(pMusic);
		throw usingstr("Unable to create %s file.\n", destfile);
	}

	WAVHeader head;
	fwrite(&head, 1, sizeof(WAVHeader), out); // reserve space

	ymMusicSetLoopMode(pMusic, YMFALSE);
	ymu32		   totalNbSample	 = 0;
	constexpr uint NBSAMPLEPERBUFFER = 1024;
	ymsample	   convertBuffer[NBSAMPLEPERBUFFER];

	while (ymMusicCompute(pMusic, convertBuffer, NBSAMPLEPERBUFFER))
	{
		fwrite(convertBuffer, sizeof(ymsample), NBSAMPLEPERBUFFER, out);
		totalNbSample += NBSAMPLEPERBUFFER;
	}

	fseek(out, 0, SEEK_SET);
	head.FormLength	   = 0x10;
	head.SampleFormat  = 1;
	head.NumChannels   = 1;
	head.PlayRate	   = 44100;
	head.BitsPerSample = 16;
	head.BytesPerSec   = 44100 * (16 / 8);
	head.Pad		   = (16 / 8);
	head.DataLength	   = totalNbSample * (16 / 8);
	head.FileLength	   = head.DataLength + sizeof(WAVHeader) - 8;
	fwrite(&head, 1, sizeof(WAVHeader), out);
	fseek(out, 0, SEEK_END);
	fclose(out);
	printf("%d samples written (%.02f Mb).\n", totalNbSample, double(totalNbSample * sizeof(ymsample)) / (1024 * 1024));
	ymMusicDestroy(pMusic);
}

uint32 YMFileConverter::exportYMFile(cstr fpath)
{
	if ((file = fopen(fpath, "wb")) == NULL) throw "Unable to open output file";

	interleave_registers();
	write_ym5ym6_header();
	fwrite(register_data, frame_size, num_frames, file);
	long size = ftell(file);
	fclose(file);
	file = nullptr;
	return uint32(size);
}

uint32 YMFileConverter::exportRsrcFile(cstr hdr_fpath, cstr rsrc_fname, uint8 wsize, uint8 lsize)
{
	// create header file with an array with a compressed resource file
	// for a YM register file in flash.

	CompressedRsrcFileWriter writer(hdr_fpath, rsrc_fname, wsize, lsize);

	writer.store("ymrf", 4); // file ID
	writer.store_byte(0);	 // format variant
	writer.store_byte(0);	 // flags
	writer.store_byte(uint8(frame_rate));
	writer.store_byte(14); // frame size
	writer.store(num_frames);
	writer.store(loop_frame);
	writer.store(ay_clock);
	writer.store(title);
	writer.store(author);
	writer.store(comment);

	// f=0: interleaved: register_data[r * 1 + frame * 16]
	// f=1: sequential:  register_data[r * num_frames + frame * 1]
	bool f	= attributes & NotInterleaved;
	uint rf = f ? num_frames : 1;
	uint ff = f ? 1 : frame_size;

	for (uint32 frame = 0; frame < num_frames; frame++)
	{
		for (uint r = 0; r <= 13; r++) writer.store_byte(register_data[r * rf + frame * ff]);
	}

	writer.close();
	return writer.csize;
}

uint32 YMFileConverter::importFile(cstr fpath, bool v)
{
	dispose_all();

	file_type  = UNSET;
	num_frames = 0;
	attributes = NotInterleaved;
	drums	   = 0;
	ay_clock   = 2000000; // as i understand this was the Atari ST chip clock
	frame_rate = 50;
	loop_frame = 0;
	title	   = "";
	author	   = "";
	comment	   = "";

	file = fopen(fpath, "rb");
	if (!file) throw "Unable to open source file";
	uint32 fsize  = getFileSize(file);
	register_data = new uchar[fsize];
	if (fread(register_data, 1, fsize, file) != fsize) throw "Unable to read file";
	fclose(file);
	file		= nullptr;
	uint32 size = fsize;
	unpack_lzh_data(register_data, size);

	if (memcmp(register_data, "YM5!", 4) == 0) file_type = YM5;
	else if (memcmp(register_data, "YM6!", 4) == 0) file_type = YM6;
	else if (memcmp(register_data, "YM2!", 4) == 0) file_type = YM2;
	else if (memcmp(register_data, "YM3!", 4) == 0) file_type = YM3;
	else if (memcmp(register_data, "YM3b", 4) == 0) file_type = YM3b;
	else if (memcmp(register_data, "YM4!", 4) == 0) throw "File is YM4 - not supported";
	else throw "not a YM music file";

	if (v) printf("importing: %s\n", fpath);
	if (v) printf("file size = %d\n", fsize);
	if (v) printf("  version = %.4s\n", cptr(register_data));

	if (file_type < YM5)
	{
		frame_size = 14;
		num_frames = (size - 4) / 14;
		memmove(register_data, register_data + 4, num_frames * frame_size);
		title	= filenamefrompath(fpath);
		comment = "converted by lib kilipili";

		if (v) printf("   frames = %d\n", num_frames);
	}
	else // YM5, YM6
	{
		if (strncmp(cptr(register_data + 4), "LeOnArD!", 8)) throw "File is not a valid YM5/YM6 file";
		uchar* p   = register_data + 12;
		num_frames = readBE32(&p);
		attributes = readBE32(&p);
		drums	   = readBE16(&p);
		ay_clock   = readBE32(&p);
		frame_rate = readBE16(&p);
		loop_frame = readBE32(&p);
		uint skip  = readBE16(&p);
		p		   = p + skip;
		if (drums) throw "DigiDrums not supported.";
		title	= readString(&p);
		author	= readString(&p);
		comment = readString(&p);

		frame_size = 16;
		memmove(register_data, p, num_frames * frame_size);

		if (v) printf("   frames = %d\n", num_frames);
		if (v) printf("   attrib = %#.4x (%s)\n", attributes, attributes & 1 ? "not interleaved" : "interleaved!");
		if (v) printf("    clock = %d Hz\n", ay_clock);
		if (v) printf("     rate = %d Hz\n", frame_rate);
		if (v) printf("  loop to = %d\n", loop_frame);
		if (v) printf("    title = %s\n", title);
		if (v) printf("   author = %s\n", author);
		if (v) printf("  comment = %s\n", comment);
	}
	return fsize;
}


} // namespace kio::Audio

/*




































*/
