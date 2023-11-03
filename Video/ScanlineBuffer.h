// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "kilipili_cdefs.h"
#include "standard_types.h"

namespace kio::Video
{

/*	struct VGAScanlineBuffer 
	provides a buffer for prepared scanlines for display by the video hardware.
	The data can be used by a nested DMA with a circular primary DMA channel.
*/
struct ScanlineBuffer
{
	void setup(const VgaMode& videomode, uint count) throws;
	void teardown() noexcept;
	bool is_valid() const noexcept { return count != 0; }

	uint32* operator[](int rolling_index) noexcept
	{
		assert(count);
		return scanlines[(uint(rolling_index) & mask) << vss];
	}

	int	 vss;		// repetition of each scanline for lowres screen modes
	uint width;		// length of scanlines in pixels
	uint count = 0; // number of scanlines in buffer
	uint mask  = 0; // count - 1

	static constexpr uint max_count = 32;		// 4 .. 2^N .. 32
	static uint32*		  scanlines[max_count]; // array of pointers to scanlines, ready for fragment dma
};

extern ScanlineBuffer scanline_buffer;


} // namespace kio::Video
