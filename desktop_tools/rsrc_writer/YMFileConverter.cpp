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
#include "Devices/HeatShrinkEncoder.h"
#include "Devices/LzhDecoder.h"
#include "cdefs.h"
#include "cstrings.h"
#include "standard_types.h"
#include <cstring>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

using namespace kio::Devices;

namespace kio::Audio
{

YMFileConverter::~YMFileConverter() { delete[] register_data; }

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
	//if (file_type < YM5) file_type = YM5;
}

uint32 YMFileConverter::write_raw_audio_file(FilePtr file, float sample_rate)
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

		file->write(sample_buffer, sizeof(MonoSample) * samples_per_frame);
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

uint32 YMFileConverter::exportWavFile(FilePtr file, float sample_rate)
{
	WAVHeader header;
	file->write(&header, sizeof(WAVHeader)); // reserve space
	uint32 num_samples = write_raw_audio_file(file, sample_rate);

	uint32 fsize = uint32(file->getSize());
	file->setFpos(0); // rewind file
	header.PlayRate	   = htole32(uint32(sample_rate));
	header.BytesPerSec = htole32(uint32(sample_rate) * (16 / 8));
	header.DataLength  = htole32(num_samples * (16 / 8));
	header.FileLength  = htole32(num_samples * (16 / 8) + sizeof(WAVHeader) - 8);
	file->write(&header, sizeof(WAVHeader));
	return fsize;
}

uint32 YMFileConverter::exportYMMFile(FilePtr file)
{
	// write uncompressed YM Music register file
	// registers per frame = 14
	// the register data is segmented into segments of 100 frames (2 seconds).
	// the register data within a segment is not interleaved, that is:
	// segment_data[] = reg0[100], reg1[100], ... reg13[100]
	// this is a compromise between compressibility
	// and the need to read the file sequentially for play back.

	file->puts("ymm!");			  // file ID
	file->putc(0);				  // variant
	file->putc(0);				  // flags
	file->putc(char(frame_rate)); //
	file->putc(14);				  // registers per frame
	file->write_LE(num_frames);
	file->write_LE(loop_frame);
	file->write_LE(ay_clock);
	file->write(title, strlen(title) + 1);
	file->write(author, strlen(author) + 1);
	file->write(comment, strlen(comment) + 1);

	// f=0: interleaved: register_data[r * 1 + frame * 16]
	// f=1: sequential:  register_data[r * num_frames + frame * 1]
	bool f	= attributes & NotInterleaved;
	uint rf = f ? num_frames : 1;
	uint ff = f ? 1 : frame_size;

	for (uint32 frame0 = 0; frame0 < num_frames; frame0 += 100)
	{
		uint end = min(num_frames, frame0 + 100);
		for (uint reg = 0; reg < 14; reg++)
		{
			for (uint32 frame = frame0; frame < end; frame++)
			{
				file->write<uint8>(register_data[reg * rf + frame * ff]);
			}
		}
	}

	return uint32(file->getSize());
}

void YMFileConverter::import_file(File* file, cstr fname, bool v)
{
	delete[] register_data;
	register_data = nullptr;

	file_type  = UNSET;
	num_frames = 0;
	attributes = NotInterleaved;
	drums	   = 0;
	ay_clock   = 2000000; // Atari ST chip clock
	frame_rate = 50;
	loop_frame = 0;
	title	   = filenamefrompath(fname);
	author	   = "unknown";
	comment	   = "converted by lib kilipili";

	csize = file->getSize();
	if (isLzhEncoded(file)) file = new LzhDecoder(file);
	else if (isHeatShrinkEncoded(file)) file = new HeatShrinkDecoder(file);

	usize = uint32(file->getSize());

	char magic[8];
	file->read(magic, 4);
	if (memcmp(magic, "YM2!", 4) == 0) file_type = YM2;
	else if (memcmp(magic, "YM3!", 4) == 0) file_type = YM3;
	else if (memcmp(magic, "YM3b", 4) == 0) file_type = YM3b;
	else if (memcmp(magic, "YM4!", 4) == 0) throw "File is YM4 - not supported";
	else if (memcmp(magic, "YM5!", 4) == 0) file_type = YM5;
	else if (memcmp(magic, "YM6!", 4) == 0) file_type = YM6;
	else throw "not a YM music file";

	if (v) printf("importing: %s\n", fname);
	if (v) printf("file size = %u\n", csize);
	if (v) printf("  version = %.4s\n", magic);

	switch (int(file_type))
	{
	case YM2: // MADMAX: YM2 has a speciality in playback with ENV and drums
	case YM3: // Standard Atari:
	{
		frame_size = 14;
		num_frames = (usize - 4) / 14;

		if (v) printf("   frames = %u\n", num_frames);
		break;
	}
	case YM3b: // standard Atari + Loop Info:
	{
		loop_frame = file->read_BE<uint32>();
		frame_size = 14;
		num_frames = (usize - 8) / 14;

		if (v) printf("   frames = %u\n", num_frames);
		if (v) printf("  loop to = %u\n", loop_frame);
		break;
	}
	case YM5:
	case YM6:
	{
		file->read(magic, 8);
		if (memcmp(magic, "LeOnArD!", 8) != 0) throw "File is not a valid YM5/YM6 file";
		num_frames = file->read_BE<uint32>();
		attributes = file->read_BE<uint32>();
		drums	   = file->read_BE<uint16>();
		if (drums) throw "DigiDrums not supported.";
		ay_clock   = file->read_BE<uint32>();
		frame_rate = file->read_BE<uint16>();
		loop_frame = file->read_BE<uint32>();
		uint skip  = file->read_BE<uint16>();
		file->setFpos(file->getFpos() + skip);
		title	   = file->gets(1 << 0);
		author	   = file->gets(1 << 0);
		comment	   = file->gets(1 << 0); // 0-terminated only
		frame_size = 16;

		if (v) printf("   frames = %u\n", num_frames);
		if (v) printf("   attrib = %#.4x (%s)\n", attributes, attributes & 1 ? "not interleaved" : "interleaved!");
		if (v) printf("    clock = %u Hz\n", ay_clock);
		if (v) printf("     rate = %d Hz\n", frame_rate);
		if (v) printf("  loop to = %u\n", loop_frame);
		if (v) printf("    title = %s\n", title);
		if (v) printf("   author = %s\n", author);
		if (v) printf("  comment = %s\n", comment);
		break;
	}
	default: IERR();
	}

	register_data = new uchar[num_frames * frame_size];
	file->read(register_data, num_frames * frame_size);
	//file->close();
	//file = nullptr;
	//return usize;
}

YMFileConverter::YMFileConverter(FilePtr file, cstr fname, bool v)
{
	import_file(file, fname, v); //
}

YMFileConverter::YMFileConverter(cstr fpath, bool verbose)
{
	FilePtr file = new StdFile(fpath);
	import_file(file, fpath, verbose);
}

} // namespace kio::Audio

/*




































*/
