// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Graphics/BitBlit.h"
#include "VideoPlane.h"

namespace kio::Video
{

class UniColorBackdrop : public VideoPlane
{
public:
	UniColorBackdrop(Color color) noexcept : //
		color(Graphics::flood_filled_color<Graphics::colordepth_16bpp>(color))
	{}

	virtual ~UniColorBackdrop() noexcept override = default;
	virtual void setup(coord width) override { this->width = uint(width); }
	virtual void teardown() noexcept override {}
	virtual void vblank() noexcept override {}

	/*
		render one scanline into the buffer.
		note: this function must go into RAM else no video during flash lock-out!
		@param row    the current row, starting at 0
		@param buffer destination for the pixel data
	*/
	virtual void renderScanline(int __unused row, uint32* buffer) noexcept override
	{
		// a multiple of 8 bytes is always guaranteed:
		// note: worst case: width=200 & sizeof(Color)=1
		for (uint n = width / (8 / sizeof(Color)); n; n--)
		{
			*buffer++ = color;
			*buffer++ = color;
		}
	}

	uint32 color;
	uint   width;
};

} // namespace kio::Video
