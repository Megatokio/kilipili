// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "cdefs.h"
#include "standard_types.h"
#include <pico.h>

#define RAM __attribute__((section(".time_critical.SLB"))) // general ram

#if !defined VIDEO_MAX_SCANLINE_BUFFERS
  #define VIDEO_MAX_SCANLINE_BUFFERS 16 // 2 .. 2^N .. 16
#endif

namespace kio::Video
{

/*
	struct VGAScanlineBuffer 
	provides a buffer for prepared scanlines for display by the video hardware.
	The data can be used by a nested DMA with a circular primary DMA channel.
*/
struct ScanlineBuffer
{
	/*
		Setup the buffer for VgaMode and buffer size. 
		The buffer size measured in source bitmap lines. 
		Valid values are 2 .. 2^N .. 16.
		The requested buffer size is silently limited to the available maximum buffer size.
		Note: scanlines[] stores pointers for each physical scanline. In low-res modes,
		where scanlines are repeated, the maximum value for buffer_size decreases accordingly.
	*/
	void setup(const VgaMode& vga_mode, uint buffer_size) throws;

	/*
	*/
	void teardown() noexcept;

	/*
	  
	*/
	bool is_valid() const noexcept { return count != 0; }

	/*
	*/
	__force_inline RAM uint32* operator[](int rolling_index) noexcept
	{
		assert(count);
		return scanlines[(uint(rolling_index) & mask) << vss];
	}

	int	 vss;		// log2 of repetitions of each scanline for lowres screen modes
	uint count = 0; // number of scanlines in buffer (logical scanlines)
	uint mask  = 0; // count - 1

	static constexpr uint max_count = VIDEO_MAX_SCANLINE_BUFFERS;
	static_assert(max_count >= 2 && max_count <= 32 && (max_count & (max_count - 1)) == 0);
	static uint32* scanlines[max_count]; // array of pointers to scanlines, ready for fragment dma
};

extern ScanlineBuffer scanline_buffer;


} // namespace kio::Video

#undef RAM
