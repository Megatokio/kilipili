// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ScanlineBuffer.h"
#include "kilipili_cdefs.h"

namespace kio::Video
{

ScanlineBuffer scanline_buffer;


void ScanlineBuffer::setup(const VgaMode* videomode, uint new_count) throws
{
	assert_eq(mask, 0);						   // must be invalid
	assert_ge(new_count, 2);				   // at least 2 scanlines
	assert_eq(new_count & (new_count - 1), 0); // must be 2^N

	teardown();

	width  = videomode->h_active;
	yscale = videomode->v_active / videomode->height;

	assert_eq(width % 2, 0);
	assert_eq(yscale, videomode->yscale);
	assert_eq(yscale & (yscale - 1), 0); // must be 2^N
	assert_eq(videomode->v_active, yscale * videomode->height);

	new_count *= yscale;
	scanlines = new uint32*[new_count];

	try
	{
		for (count = 0; count < new_count; count += yscale)
		{
			uint32* sl = new uint32[width / 2];
			for (uint y = 0; y < yscale; y++) scanlines[count + y] = sl;
		}

		mask = count - 1;
	}
	catch (...)
	{
		throw OUT_OF_MEMORY;
	}
}

void ScanlineBuffer::teardown() noexcept
{
	if (scanlines)
	{
		mask = 0;
		while (count)
		{
			count -= yscale;
			delete[] scanlines[count];
		}
		delete[] scanlines;
		scanlines = nullptr;
	}
}


} // namespace kio::Video
