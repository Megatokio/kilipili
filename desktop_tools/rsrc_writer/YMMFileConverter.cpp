// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause
//
// YM file to YMM file converter
// based on YM file register extractor and interleaver by Daniel Tufvesson 2014
// based on YM file format specification by Arnaud Carré

#include "YMMFileConverter.h"
#include "Audio/Ay38912.h"
#include "Devices/DevNull.h"
#include "Devices/LzhDecoder.h"
#include "Devices/StdFile.h"
#include "cdefs.h"
#include "cstrings.h"
#include "standard_types.h"
#include <common/Array.h>
#include <common/basic_math.h>
#include <cstring>
#include <dirent.h>
#include <stdlib.h>

using namespace kio::Devices;


namespace kio::Audio
{


struct BitArray
{
	Array<uint8> data;
	uint		 accu	  = 0; // accumulator
	uint		 bits	  = 0; // remaining num bits in accu
	uint		 fpos	  = 0; // decoder
	uint		 _padding = 0;

	uint32 get_bitpos() const { return fpos * 8 - bits; }
	void   set_bitpos(uint32 new_fpos)
	{
		fpos = (new_fpos + 7) / 8;
		bits = fpos * 8 - new_fpos;
		accu = bits ? data[fpos - 1] & ((1 << bits) - 1) : 0;
	}

	BitArray(uint prealloc = 0) : data(0u, prealloc) {}
	void append_bits(uint value, uint nbits);
	void append_number(uint value);
	void finish();
	uint read_bits(uint nbits);
	uint read_number();
	uint count() const { return data.count(); }
	void rewind() { accu = bits = fpos = 0; }
};

void BitArray::finish()
{
	append_bits(0, 7);
	rewind();
}

void BitArray::append_bits(uint value, uint nbits)
{
	assert(value >> nbits == 0);

	accu = (accu << nbits) | value;
	bits += nbits;
	while (bits >= 8) { data.append(uint8(accu >> (bits -= 8))); }
}

void BitArray::append_number(uint value)
{
	// the rle encoded number is preceeded by (b-1) null bits
	// where b = number of bits needed to store the number.
	// e.g. 1 needs 1 bit  => preceeded by 0 null bits
	//      5 needs 3 bits => preceeded by 2 null bits

	assert(value >= 1);

	uint bits;
	for (bits = 1; value >> bits; bits++) {}
	append_bits(0, bits - 1); // we can't add prefix and value in one call,
	append_bits(value, bits); // because 2*nbits-1 can actually be larger than 31.
}

uint BitArray::read_bits(uint nbits)
{
	// the bits are added at lsb, so they come out at msb:

	while (bits < nbits)
	{
		assert(fpos < data.count());
		assert(bits <= 24);
		accu = (accu << 8) + data[fpos++];
		bits += 8;
	}
	bits -= nbits;
	uint rval = accu >> bits;
	accu -= rval << bits;
	return rval;
}

uint BitArray::read_number()
{
	assert(bits < 8);
	assert((accu >> bits) == 0); // accu must be clean outside valid bits

	// pull bits until the msb of the number is in the accu:
	while (accu == 0)
	{
		assert(fpos < data.count());
		assert(bits <= 24);
		accu = data[fpos++];
		bits += 8;
	}

	// find the msbit:
	uint msbit = bits - 1;
	while (accu >> msbit == 0) msbit--;
	uint nbits = bits - msbit;

	// remove the preceeding 0-bits:
	bits = msbit + 1;
	assert((accu >> msbit) == 1);

	// read and return the number:
	return read_bits(nbits);
}

struct ValueCount
{
	int value; // register_value or backref_offset
	int count; // repeat_count or -backref_chunksize

	bool is_literal() const { return value >= 0; }
	bool is_backref() const { return value < 0; }
};

static bool operator==(const ValueCount& a, const ValueCount& b)
{
	return a.value == b.value && a.count == b.count; //
}

struct RleCode
{
	uint8 value;
	uint8 count;
};

static RleCode dead_RleCode; // for BackrefBuffer with backref window size = 0

struct BackrefBuffer
{
	RleCode* data		   = nullptr;
	uint16	 mask		   = 0; // data.size - 1
	uint16	 index		   = 0;
	uint8	 bits		   = 0; // data.size = 1 << bits
	uint8	 aybits		   = 0;
	uint8	 regvalue	   = 0;
	uint8	 regcount	   = 0;
	uint16	 backrefoffset = 0;
	uint16	 backrefcount  = 0;
	uint32	 _padding	   = 0; // the real decoder will have no padding b-)

	BackrefBuffer() = default;
	BackrefBuffer(RleCode* p, uint8 bits, uint8 aybits) :
		data(p),
		mask(uint16((1 << bits) - 1)),
		bits(bits),
		aybits(aybits)
	{}
	BackrefBuffer(uint8 aybits) : data(&dead_RleCode), aybits(aybits) {}
	uint8 next_value(BitArray& instream);
};

uint8 BackrefBuffer::next_value(BitArray& instream)
{
	if (regcount--) { return regvalue; }

	if (backrefcount == 0)
	{
		bool is_backref = instream.read_bits(1);
		int	 value		= int(instream.read_bits(is_backref ? bits : aybits));
		int	 count		= int(instream.read_number());

		if (is_backref) // LZ code:
		{
			assert(value >= 0 && value < 1 << bits);
			assert(count >= 1 && count <= UINT16_MAX);
			backrefoffset = uint16(value);
			backrefcount  = uint16(count);
		}
		else // RLE code:
		{
			assert(value >= 0 && value < 1 << aybits);
			assert(count >= 1 && count <= UINT8_MAX);
			data[index++ & mask] = RleCode {uint8(value), uint8(count)};
			regvalue			 = uint8(value);
			regcount			 = uint8(count - 1);
			return uint8(value);
		}
	}

	backrefcount -= 1;
	RleCode& vc	   = data[(index - backrefoffset) & mask];
	uint8	 count = vc.count;
	uint8	 value = vc.value;

	assert(value >= 0 && value < 1 << aybits);
	assert(count >= 1 && count <= UINT8_MAX);
	data[index++ & mask] = RleCode {value, count};
	regvalue			 = value;
	regcount			 = count - 1;
	return value;
}


// ############### Helper Functions ##########################


void YMMFileConverter::deinterleave_registers()
{
	assert(frame_size == 16);

	if ((attributes & NotInterleaved)) return;
	attributes |= NotInterleaved;

	uint8* zbu = new uint8[num_frames * 16];
	uint8* q   = register_data;
	for (uint frame = 0; frame < num_frames; frame++)
	{
		for (uint reg = 0; reg < frame_size; reg++)
		{
			zbu[frame + reg * num_frames] = *q++; //
		}
	}

	delete[] register_data;
	register_data = zbu;

	frame_size = 16;
	if (file_type < YM5) file_type = YM5;
}

Array<uint8> YMMFileConverter::extract_register_stream(uint reg, uint8 mask)
{
	assert(attributes & NotInterleaved);
	assert(frame_size == 16);

	Array<uint8> array {num_frames};
	memcpy(array.getData(), register_data + reg * num_frames, num_frames);

	// the envelope shape register must not be set if it was not actually written to by the player.
	// therefore .ym set this register to 0xff (an illegal value) to indicate: 'don't write'.
	// we map 0xff to 0x0f and 0x0f to any 0b0100 which has the same effect for the envelope:
	if (reg == 13)
		for (uint32 i = 0; i < array.count(); i++)
			if (array[i] != 0xff && (array[i] & 0x0f) == 0x0f)
			{
				printf("*** chg env value\n");
				array[i] = 0b0100;
			}

	if (mask != 0xff)
		for (uint32 i = 0; i < array.count(); i++) { array[i] &= mask; }
	return array;
}

static Array<ValueCount> rle_encode_register_stream(const Array<uint8>& indata)
{
	// convert stream of register_values
	// into stream of { register_value, repeat_count } pairs.
	// repeat count = 1 .. 255 because the decoder may store it in a uint8.

	Array<ValueCount> outdata;
	if (indata.count() == 0) return outdata;

	int value = indata[0];
	int count = 0;
	for (uint i = 0; i < indata.count(); i++)
	{
		if (indata[i] != value || count == 255)
		{
			assert(count);
			outdata << ValueCount {value, count};
			value = indata[i];
			count = 0;
		}
		count++;
	}

	assert(count);
	outdata << ValueCount {value, count};

	return outdata;
}

static ValueCount find_best_backref(ValueCount* p, int maxoffs, int maxlen)
{
	// backrefs are stored with value = -offset

	assert(maxoffs >= 1); // limited by distance to start of data and window size
	assert(maxlen >= 1);  // limited by distance to end of data

	// a longer sequence is always better
	// offset and contents don't matter:
	// offset is encoded with fixed number of bits
	// count increases at most with 2 bits for an additional code
	// but codes are at least 1+4+1 bits long

	ValueCount r {0, 0};

	for (int offs = 1; offs <= maxoffs; offs++)
	{
		if (*p != *(p - offs)) continue;
		int cnt = 1;
		while (cnt < maxlen && *(p + cnt) == *(p + cnt - offs)) cnt++;
		if (cnt > r.count) r = {-offs, cnt};
	}
	return r;
}

static Array<ValueCount> lz_encode_rle_stream(Array<ValueCount>& indata, uint aybits, uint winbits)
{
	// convert stream of { register_value, repeat_count } rle data
	// into stream of { register_value, repeat_count } rle or { -offset, count } backrefs:

	assert(winbits >= 3 && winbits <= 11);

	// count = 2:
	// a backref may be longer:
	// backref = 1 + winbits + 3 bits
	// min literal codes = 2 * (1+4+1) bits
	// => if winbits+4 > 12 <=> if winbits > 8 this can happen
	// => if total buffer winbits >= 12 then this can happen
	// only channel frequency msbit registers have only 4 bits
	// the min literal code with 6 bits means that the register value is not repeated
	// which is generally very unlikely.
	// => this is acceptable if it ever happens.
	//
	// count = 1:
	// a backref may be shorter:
	// backref = 1 + winbits + 1 bits
	// literal code = 1 + aybits + 1+2*n bits
	// => this can happen very often!
	//
	// test result:
	// W8:  554949 bytes -> 527714 bytes:  -4.9%
	// W10: 278175 bytes -> 270446 bytes:  -2.7%
	// W12: 165728 bytes -> 164188 bytes:  -1.0%

	Array<ValueCount> outdata;
	int				  indata_count = int(indata.count());
	if (indata_count == 0) return outdata;

	outdata << indata[0]; // first code must be verbatim: there is nothing to copy

	for (int i = 1; i < indata_count;)
	{
		int		   maxo			= min((1 << winbits) - 1, i);
		int		   maxl			= min(0xffff, indata_count - i);
		ValueCount best_backref = find_best_backref(&indata[i], maxo, maxl);

		bool use_backref = best_backref.count >= 2;
		if (best_backref.count == 1)
		{
			// backref = 1 + winbits + 1 bits
			// literal = 1 + regbits + 1+2*n bits
			int n		= indata[i].count;
			use_backref = winbits < aybits + (n < 2 ? 0 : n < 4 ? 2 : n < 8 ? 4 : n < 16 ? 6 : 8);
		}

		if (use_backref)
		{
			assert(best_backref.value < 0);
			assert(i + best_backref.value >= 0);
			assert(i + best_backref.count <= indata_count);

			outdata << best_backref;
			i += best_backref.count;
		}
		else // store literal
		{
			assert(indata[i].value >= 0);
			assert(indata[i].count >= 1);

			outdata << indata[i];
			i += 1;
		}
	}

	return outdata;
}

static BitArray encode_as_bitstream(Array<ValueCount>& indata, uint aybits, uint winbits)
{
	// encode RLE & LZ encoded ValueCount stream into a bitstream:

	assert(aybits >= 4 && aybits <= 8);
	assert(winbits >= 0 && winbits <= 15); // 0 => not lz compressed => no lz backrefs!

	BitArray outdata(indata.count() * 2);

	for (uint32 i = 0; i < indata.count(); i++)
	{
		ValueCount& code	   = indata[i];
		bool		is_backref = code.value < 0;
		outdata.append_bits(is_backref, 1);
		if (is_backref) outdata.append_bits(uint(-code.value), winbits);
		else outdata.append_bits(uint(code.value), aybits);
		outdata.append_number(uint(code.count));
	}

	outdata.finish();
	return outdata;
}

void YMMFileConverter::decode_ymm(uint32 rbusz, BitArray& instream, uint winbits)
{
	// decode the bitstream and compare register data with original register data:

	assert(winbits >= 8 && winbits <= 14);
	assert(frame_size == 16);
	assert(attributes & NotInterleaved);

	// allocate total buffer and assign register buffers:
	std::unique_ptr<RleCode[]> allocated_buffer {new RleCode[1 << winbits]};
	BackrefBuffer			   backref_buffers[16];

	RleCode* p = allocated_buffer.get();
	for (int r = 0; r < 16; r++)
	{
		if (uint8 sz = (rbusz >> r >> r) & 0x03)
		{
			sz += winbits - 4 - 2;
			backref_buffers[r] = BackrefBuffer {p, sz, ayRegisterNumBits[r]};
			p += 1 << sz;
		}
		else backref_buffers[r] = BackrefBuffer {ayRegisterNumBits[r]};
	}
	assert(p == allocated_buffer.get() + (1 << winbits));

	for (uint frame = 0; frame < num_frames; frame++)
	{
		for (uint r = 0; r < 16; r++)
		{
			BackrefBuffer& buffer = backref_buffers[r];
			uint8		   value  = buffer.next_value(instream);

			if (value != ((register_data[r * num_frames + frame]) & ayRegisterBitMasks[r]))
				assert(value == ((register_data[r * num_frames + frame]) & ayRegisterBitMasks[r]));
		}
	}
}


// ########################### Main Functions #############################


void YMMFileConverter::export_ymm_file(File* file, SerialDevice* log, uint winbits)
{
	assert(winbits >= 8 && winbits <= 14); // sanity

	file->puts("ymm!");			  // file ID
	file->putc(2);				  // variant
	file->putc(char(winbits));	  // flags
	file->putc(char(frame_rate)); //
	file->putc(16);				  // registers per frame
	file->write_LE(num_frames);
	file->write_LE(loop_frame);
	file->write_LE(ay_clock);
	file->write(title, strlen(title) + 1);
	file->write(author, strlen(author) + 1);
	file->write(comment, strlen(comment) + 1);

	// buffersize = 1024 codes => Ø windowsize = 64 codes, decoder buffersize = 2048 bytes
	// most samples benefit from ~ 1k buffer at least.
	//
	// example results:
	// bytestream with maxlen=0x7fff:
	// buffersize =  512 codes: 253857 codes total  40% more than 1024
	// buffersize = 1024 codes: 180813 codes total
	// buffersize = 2048 codes: 133829 codes total  26% less than 1024
	// buffersize = 4096 codes: 101367 codes total  44% less than 1024

	// maxlen=127 is needed for encoding a bytestream.
	// a bitstream can use arbitrarily high maxlen.
	// limiting maxlen to 127 increases average file size by ~2.65% compared to unlimited maxlen.
	//
	// example results:
	// buffersize = 1024, maxlen = unlim: 178554 codes total
	// buffersize = 1024, maxlen = 127:   184751 codes total
	//
	// encoding the data into a bitstream saves ~25%.
	// efficiency varies with buffersize,
	// possibly because num bits for back_ref offset increases?
	//
	// example results:
	// buffersize = 512,  TOTAL: 251235 codes => 357170 bytes in bitstream (-29%)
	// buffersize = 1024, TOTAL: 178554 codes => 266267 bytes in bitstream (-25%)
	// buffersize = 2048, TOTAL: 131552 codes => 206620 bytes in bitstream (-21%)
	// buffersize = 4096, TOTAL:  99503 codes => 162479 bytes in bitstream (-18%)

	const int buffersize = 1 << winbits;
	const int minwinsize = (buffersize / 16) / 2;

	// for each register calculate the bitstream for
	// BackrefBuffer with no buffer, windowbits-1, windowbits and windowbits+1:

	Array<uint8> register_streams[16];
	BitArray	 bitstreams[16][4];

	for (uint reg = 0; reg < 16; reg++)
	{
		register_streams[reg]		 = extract_register_stream(reg, ayRegisterBitMasks[reg]);
		Array<ValueCount> rle_buffer = rle_encode_register_stream(register_streams[reg]);

		uint reg_bits = ayRegisterNumBits[reg];
		uint win_bits = winbits - 4;

		Array<ValueCount> lz_buffers;
		lz_buffers		   = rle_buffer;
		bitstreams[reg][0] = encode_as_bitstream(lz_buffers, reg_bits, 0);

		lz_buffers		   = lz_encode_rle_stream(rle_buffer, reg_bits, win_bits - 1);
		bitstreams[reg][1] = encode_as_bitstream(lz_buffers, reg_bits, win_bits - 1);

		lz_buffers		   = lz_encode_rle_stream(rle_buffer, reg_bits, win_bits - 0);
		bitstreams[reg][2] = encode_as_bitstream(lz_buffers, reg_bits, win_bits - 0);

		lz_buffers		   = lz_encode_rle_stream(rle_buffer, reg_bits, win_bits + 1);
		bitstreams[reg][3] = encode_as_bitstream(lz_buffers, reg_bits, win_bits + 1);
	}

	log->printf("\n*** YMM File Test Results:\n");
	log->printf("  buffer size = %i\n", buffersize);

	// buffer size per register code: 0=none, 1=half, 2=winbits, 3=double
	uint bsz[16] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};

	// try to increase some buffers on the cost of others:

	for (;;)
	{
		// find register which would benefit the most from increasing the buffer:

		int most_benefit_reg = -1;
		int most_benefit	 = -999999; // more is better
		for (int reg = 0; reg < 16; reg++)
		{
			if (bsz[reg] != 2) continue;
			int benefit = int(bitstreams[reg][2].count() - bitstreams[reg][3].count());
			if (benefit <= most_benefit) continue;
			most_benefit_reg = reg;
			most_benefit	 = benefit;
		}

		// we need 2 buffers which can decrease from std to 1/2 or from 1/2 to no_buffer
		// or one buffer which can decrease from std to 1/2 to no_buffer:
		// we make 2 rounds and may find the same buffer to decrease by 1/2 twice.

		int least_sacrifice_reg	 = -1;
		int least_sacrifice		 = +999999; // less is better
		int least_sacrifice_reg1 = -1;
		int least_sacrifice1	 = +999999;

		for (int i = 0; i < 2; i++)
		{
			least_sacrifice_reg1 = least_sacrifice_reg;
			least_sacrifice1	 = least_sacrifice;
			least_sacrifice_reg	 = -1;
			least_sacrifice		 = +999999;

			for (int reg = 15; reg >= 0; reg--)
			{
				uint i = bsz[reg];
				if (i == 1 || i == 2)
				{
					int sacrifice = int(bitstreams[reg][i - 1].count() - bitstreams[reg][i].count());
					if (sacrifice >= least_sacrifice) continue;
					if (reg == most_benefit_reg) continue; // this can actually happen!
					least_sacrifice_reg = reg;
					least_sacrifice		= sacrifice;
					if (sacrifice == 0) break; // quick exit: port or env register
				}
			}

			assert(least_sacrifice_reg != -1);
			bsz[least_sacrifice_reg] -= 1;
		}

		if (least_sacrifice + least_sacrifice1 < most_benefit)
		{
			log->printf(
				"  - shuffled reg%i+reg%i >> reg%i\n", least_sacrifice_reg, least_sacrifice_reg1, most_benefit_reg);
			bsz[most_benefit_reg] += 1; // trade buffer sizes
			continue;					// and try again
		}
		else
		{
			bsz[least_sacrifice_reg1] += 1; // undo
			bsz[least_sacrifice_reg] += 1;
			break;
		}
	}

	log->printf("  infile: %u bytes = %u * %u\n", num_frames * frame_size, frame_size, num_frames);
	log->printf("  outfile:     no window  sz=%3i  sz=%3i  sz=%3i\n", minwinsize, minwinsize * 2, minwinsize * 4);

	static uint TOTAL = 0;
	uint		total = 0;

	for (int reg = 0; reg < 16; reg++)
	{
		uint i = bsz[reg];
		total += bitstreams[reg][i].count();


		log->printf(
			"  register%3i:%8u%s%7u%s%7u%s%7u%s bytes\n", reg, //
			bitstreams[reg][0].count(), i == 0 ? "*" : " ",	   //
			bitstreams[reg][1].count(), i == 1 ? "*" : " ",	   //
			bitstreams[reg][2].count(), i == 2 ? "*" : " ",	   //
			bitstreams[reg][3].count(), i == 3 ? "*" : " ");   //
	}

	TOTAL += total;
	log->printf("  total: %8u bytes in bitstream\n", total);
	log->printf("  TOTAL: %8u bytes in bitstream\n", TOTAL);

	// *** now write the data ***

	// buffer size variations per register:
	uint32 rbusz = 0;
	for (int r = 0; r < 16; r++) rbusz |= uint32(bsz[r]) << r << r;
	file->write_LE(rbusz);

	// decode the 16 register streams in a round robin fashion
	// and whenever they pull from their stream copy that data into the combined stream

	// allocate total buffer and assign register backref buffers:
	BackrefBuffer			   backref_buffers[16];
	BitArray				   streams[16];
	std::unique_ptr<RleCode[]> allocated_buffer {new RleCode[1 << winbits]};

	RleCode* p = allocated_buffer.get();
	for (int r = 0; r < 16; r++)
	{
		uint sz	   = bsz[r];
		streams[r] = bitstreams[r][sz];
		streams[r].rewind();
		if (sz)
		{
			sz += winbits - 4 - 2;
			backref_buffers[r] = BackrefBuffer {p, uint8(sz), ayRegisterNumBits[r]};
			p += 1 << sz;
		}
		else backref_buffers[r] = BackrefBuffer {ayRegisterNumBits[r]};
	}
	assert(p == allocated_buffer.get() + (1 << winbits));

	BitArray combined_stream(total);
	for (uint frame = 0; frame < num_frames; frame++)
	{
		for (uint r = 0; r < 16; r++)
		{
			BitArray& source  = streams[r];
			uint32	  old_pos = source.get_bitpos();
			uint8	  value	  = backref_buffers[r].next_value(source);
			assert(value == register_streams[r][frame]);

			if (uint nbits = source.get_bitpos() - old_pos)
			{
				source.set_bitpos(old_pos);
				while (nbits)
				{
					uint n = min(nbits, 24u);
					combined_stream.append_bits(source.read_bits(n), n);
					nbits -= n;
				}
			}
		}
	}

	combined_stream.finish();
	file->write(combined_stream.data.getData(), combined_stream.count());

	// decode & compare:

	if constexpr (1)
	{
		combined_stream.rewind();
		decode_ymm(rbusz, combined_stream, winbits);
	}
}

void YMMFileConverter::import_ym_file(File* file, SerialDevice* log, cstr fname)
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

	log->printf("file size = %u\n", csize);
	log->printf("  version = %.4s\n", magic);

	switch (int(file_type))
	{
	case YM2: // MADMAX: YM2 has a speciality in playback with ENV and drums
	case YM3: // Standard Atari:
	{
		frame_size = 14;
		num_frames = (usize - 4) / 14;

		log->printf("   frames = %u\n", num_frames);
		break;
	}
	case YM3b: // standard Atari + Loop Info:
	{
		loop_frame = file->read_BE<uint32>();
		frame_size = 14;
		num_frames = (usize - 8) / 14;

		log->printf("   frames = %u\n", num_frames);
		log->printf("  loop to = %u\n", loop_frame);
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

		log->printf("   frames = %u\n", num_frames);
		log->printf("   attrib = %#.4x (%sinterleaved)\n", attributes, attributes & 1 ? "not " : "");
		log->printf("    clock = %u Hz\n", ay_clock);
		log->printf("     rate = %d Hz\n", frame_rate);
		log->printf("  loop to = %u\n", loop_frame);
		log->printf("    title = %s\n", title);
		log->printf("   author = %s\n", author);
		log->printf("  comment = %s\n", comment);
		break;
	}
	default: IERR();
	}

	assert((frame_size == 16) || (attributes & NotInterleaved));
	register_data = new uchar[num_frames * 16];
	memset(register_data, 0xff, num_frames * 16); // preset port A,B if framesize<16
	file->read(register_data, num_frames * frame_size);
	frame_size = 16;

	deinterleave_registers();
}

uint32 YMMFileConverter::convertFile(cstr infilepath, File* outfile, int verbose, uint winbits)
{
	RCPtr<SerialDevice> log;
	if (verbose) log = new StdFile("ymm.log", FileOpenMode::APPEND);
	else log = new DevNull;
	log->printf("\nconverting file: %s\n", infilepath);

	FilePtr infile = new StdFile(infilepath);
	import_ym_file(infile, log, basename_from_path(infilepath));

	export_ymm_file(outfile, log, winbits);
	return uint32(outfile->getSize());
}

} // namespace kio::Audio


/*




















































*/
