// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBufferHQi8.h"

namespace kio::Video
{

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.SRFu" XWRAP(__LINE__))))	 // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.SRFu" XWRAP(__LINE__)))) // general ram


FrameBufferHQi8::FrameBufferHQi8(RCPtr<Pixmap> px, uint8 colormap_rgb888[256][3]) : pixmap(px)
{
	update_colormap(colormap_rgb888);
}

void FrameBufferHQi8::update_colormap(uint8 rgb888[256][3])
{
	for (uint i = 0; i < 256; i++)
	{
		// color component is shifted from 8.0 bits to e.g. 5.3 bits.
		// calculate 4 colormaps based on rounding at %x.00, %x.01, %x.10 and %x.11:

		// each pixel of the pixmap is drawn by 2x2 pixels in the video display.
		// each pixel of the video display comes from another color from the above maps.
		// the exact assignment is not important and may be varied between frames to hide the pattern.
		// calculate 2 2-pixel-colormaps for the even and odd rows:

		constexpr uint r25 = 1 << (6 - Color::rbits);
		constexpr uint g25 = 1 << (6 - Color::gbits);
		constexpr uint b25 = 1 << (6 - Color::bbits);

		uint8 r = rgb888[i][0];
		uint8 g = rgb888[i][1];
		uint8 b = rgb888[i][2];

		cmaps[0][2 * i + 0] = Color::fromRGB8(r, g, b); // even row, left pixel

		r = uint8(min(r + r25, 255u));
		g = uint8(min(g + g25, 255u));
		b = uint8(min(b + b25, 255u));

		cmaps[1][2 * i + 1] = Color::fromRGB8(r, g, b); // odd row, right pixel

		r = uint8(min(r + r25, 255u));
		g = uint8(min(g + g25, 255u));
		b = uint8(min(b + b25, 255u));

		cmaps[0][2 * i + 1] = Color::fromRGB8(r, g, b); // even row, right pixel

		r = uint8(min(r + r25, 255u));
		g = uint8(min(g + g25, 255u));
		b = uint8(min(b + b25, 255u));

		cmaps[1][2 * i + 0] = Color::fromRGB8(r, g, b); // odd row, left pixel
	}
}

void FrameBufferHQi8::setup(coord width) //
{
	this->width = uint(width);
}

void FrameBufferHQi8::teardown() noexcept {}

void RAM FrameBufferHQi8::vblank() noexcept
{
	// cycle the subpixels: this is purely optional!
	for (uint i = 0; i < 256; i++)
	{
		Color* even = &cmaps[0][i * 2];
		Color* odd	= &cmaps[1][i * 2];

		std::swap(odd[0], even[1]);
		std::swap(odd[1], even[0]);
	}
}

// clang-format off
template<uint sz> struct uint_with_size{using type=uint8;};
template<> struct uint_with_size<2>{using type=uint16;};
template<> struct uint_with_size<4>{using type=uint32;};
// clang-format on

using onecolor	= uint_with_size<sizeof(Color)>::type;
using twocolors = uint_with_size<sizeof(Color) * 2>::type;

void XRAM FrameBufferHQi8::renderScanline(int row, uint32* _dest) noexcept
{
	uint8*	   pixels = pixmap->pixmap + pixmap->row_offset * row;
	twocolors* cmap	  = reinterpret_cast<twocolors*>(cmaps[row & 1]);
	twocolors* dest	  = reinterpret_cast<twocolors*>(_dest);

	for (uint i = 0; i < width / 8; i++)
	{
		*dest++ = cmap[*pixels++];
		*dest++ = cmap[*pixels++];
		*dest++ = cmap[*pixels++];
		*dest++ = cmap[*pixels++];
	}
}


} // namespace kio::Video

/*































*/
