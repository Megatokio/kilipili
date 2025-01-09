// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ScanlineBuffer.h"
#include "Color.h"
#include "basic_math.h"
#include "cdefs.h"
#include <stdio.h>
#include <string.h>

namespace kio::Video
{

ScanlineBuffer scanline_buffer;

//static
alignas(sizeof(ScanlineBuffer::scanlines)) //
	uint32* ScanlineBuffer::scanlines[max_count];


void ScanlineBuffer::setup(const VgaMode& vga_mode, uint buffer_size) throws
{
	using Color = Graphics::Color;
	assert_eq(count, 0);									 // must be invalid
	assert_eq(buffer_size & (buffer_size - 1), 0);			 // must be 2^N
	assert_ge(buffer_size, 2u);								 // must buffer at least 2 lines
	assert_eq(vga_mode.h_active() % (4 / sizeof(Color)), 0); // dma transfer unit is uint32 = 2 or 4 pixels

	uint size	   = vga_mode.h_active() / (4 / sizeof(Color));
	vss			   = vga_mode.vss;
	uint new_count = min(buffer_size, max_count >> vss);

	try
	{
		for (count = 0; count < new_count; count++)
		{
			uint32* sl = new uint32[size];
			memset(sl, 0, size * sizeof(uint32));
			for (uint y = 0; y < (1 << vss); y++) { scanlines[(count << vss) + y] = sl; }
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
	while (count) { delete[] scanlines[--count << vss]; }
}


} // namespace kio::Video
