// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ScanlineBuffer.h"
#include "kilipili_cdefs.h"

namespace kio::Video
{

ScanlineBuffer scanline_buffer;

//static
alignas(sizeof(ScanlineBuffer::scanlines)) //
	uint32* ScanlineBuffer::scanlines[max_count];


void ScanlineBuffer::setup(const VgaMode* videomode, uint new_count) throws
{
	assert_eq(count, 0); // must be invalid

	width  = videomode->h_active;
	yscale = videomode->v_active / videomode->height;

	assert_ge(new_count, 2);				   // at least 2 scanlines
	assert_eq(new_count & (new_count - 1), 0); // must be 2^N
	assert_le(new_count * yscale, max_count);  // must not exceed array
	assert_eq(width % 2, 0);				   // dma transfer unit is uint32 = 2 pixels
	assert_eq(yscale, videomode->yscale);	   // test for time beeing
	assert_eq(yscale & (yscale - 1), 0);	   // must be 2^N
	assert_ge(yscale, 1);
	assert_eq(videomode->v_active, yscale * videomode->height);

	try
	{
		for (/*count = 0*/; count < new_count; count++)
		{
			uint32* sl = new uint32[width / 2];
			for (uint y = 0; y < yscale; y++) { scanlines[count * yscale + y] = sl; }
		}
		mask = count - 1;
	}
	catch (...)
	{
		teardown();
		throw OUT_OF_MEMORY;
	}
}

void ScanlineBuffer::teardown() noexcept
{
	while (count) { delete[] scanlines[--count * yscale]; }
}


} // namespace kio::Video
