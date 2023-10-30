// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "standard_types.h"

namespace kio::Video
{

/*	struct VGAScanlineBuffer 
	provides a buffer for prepared scanlines for display by the video hardware.
	The data can be used by a nested DMA with a circular primary DMA channel.
*/
struct ScanlineBuffer
{
	void setup(const VgaMode* videomode, uint count) throws;
	void teardown() noexcept;
	bool is_valid() const noexcept { return mask >= 1; }

	uint32* operator[](int rolling_index) noexcept
	{
		assert(count);
		return scanlines[rolling_index & mask];
	}

	uint16 yscale;	  // repetition of each scanline for lowres screen modes
	uint16 width;	  // length of scanlines in pixels
	uint16 count = 0; // size of scanlines[]
	uint16 mask	 = 0;

private:
	uint32** scanlines = nullptr; // array of pointers to scanlines, ready for fragment dma
};

extern ScanlineBuffer scanline_buffer;


} // namespace kio::Video
