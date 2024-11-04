// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFile.h"
#include "basic_math.h"
#include <cstring>

namespace kio ::Devices
{

// ***********************************************************************
// Rom File

RsrcFile::RsrcFile(const uchar* data, uint32 size) : //
	File(readable),
	data(data),
	fsize(size)
{}

void RsrcFile::setFpos(ADDR new_fpos)
{
	if unlikely (new_fpos > fsize) fpos = fsize;
	else fpos = uint32(new_fpos);
}

SIZE RsrcFile::read(void* data, SIZE size, bool partial)
{
	if unlikely (size > fsize - fpos)
	{
		if (partial) size = fsize - fpos;
		else throw END_OF_FILE;
	}
	memcpy(data, this->data + fpos, size);
	fpos += size;
	return size;
}


// ***********************************************************************
// Compressed Rom File

CompressedRomFile::CompressedRomFile(const uchar* cdata, uint32 usize, uint32 csize, uint8 parameters) :
	File(readable),
	cdata(cdata),
	usize(usize),
	csize(csize)
{
	uint8 ss_lookahead = parameters & 15;
	uint8 ss_window	   = parameters >> 4;

	assert(4 <= ss_lookahead);		  // currently used: 6
	assert(ss_lookahead < ss_window); //
	assert(ss_window <= 14);		  // currently used: 12

	decoder = heatshrink_decoder_alloc(256, ss_window, ss_lookahead);
	if (!decoder) throw OUT_OF_MEMORY;
}

void CompressedRomFile::close(bool)
{
	if (decoder) heatshrink_decoder_free(decoder);
	decoder = nullptr;
}

void CompressedRomFile::setFpos(ADDR new_fpos)
{
	if unlikely (new_fpos > usize)
	{
		heatshrink_decoder_reset(decoder);
		fpos = usize;
		cpos = csize;
		return;
	}

	if (uint32(new_fpos) < fpos)
	{
		heatshrink_decoder_reset(decoder);
		fpos = 0;
		cpos = 0;
	}

	while (fpos < uint32(new_fpos))
	{
		uint8 buffer[100];
		read(buffer, min(uint32(new_fpos) - fpos, uint32(sizeof(buffer))));
	}
}

SIZE CompressedRomFile::read(void* _data, SIZE count, bool partial)
{
	if (count == 0) return 0;
	uint8* data = reinterpret_cast<uint8*>(_data);

	for (uint32 n, cnt = 0;;)
	{
		int err = heatshrink_decoder_poll(decoder, data + cnt, count - cnt, &n);
		assert(err >= 0);
		cnt += n;
		fpos += n;
		if (cnt == count) return count;
		if (cpos == csize)
		{
			assert(fpos == usize);
			if (partial) return cnt;
			else throw END_OF_FILE;
		}
		err = heatshrink_decoder_sink(decoder, cdata + cpos, csize - cpos, &n);
		assert(err >= 0);
		cpos += n;
	}
}

} // namespace kio::Devices

/*





























*/
