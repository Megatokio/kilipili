// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "color_options.h"
#include "standard_types.h"
#include "tempmem.h"
#include <cstdio>

namespace kio::Graphics
{

/*
	A `Color` represents what is used by the Video Hardware.
	It is used in the library to represent a `true color`.
	It is configurable by #defines in the `boards` header file.
	Currently supported are 8bit and 16 bit color in RGB and BGR order,
	RGBI with common low bits and monochrome GREY colors,
	which should cover almost all cases.
*/
struct Color
{
#define ORDER_RGB  (VIDEO_PIXEL_ICOUNT == 0)
#define ORDER_GREY (VIDEO_PIXEL_ICOUNT == VIDEO_COLOR_PIN_COUNT)
#define ORDER_RGBI (VIDEO_PIXEL_ICOUNT != 0 && VIDEO_PIXEL_ICOUNT < VIDEO_COLOR_PIN_COUNT)

	static_assert(ORDER_RGB + ORDER_RGBI + ORDER_GREY == 1);
	static_assert(ORDER_RGB || (VIDEO_PIXEL_RCOUNT == VIDEO_PIXEL_GCOUNT && VIDEO_PIXEL_RCOUNT == VIDEO_PIXEL_BCOUNT));

	// the vgaboard uses RGB565
	// the kiboard uses RGB444
	// video output may include one or two 'common low bits' for all 3 channels
	// the picomite, a Pico-based Basic computer, uses BGR121
	// video output may be monochrome, possibly even 1 bit b&w only

	// xbits  = number of bits for component
	// xmask  = bitmask for component shifted in place in Color::raw.
	// xshift = number of bits to shift left from int0 position (all bits outside) to it's place in Color::raw.
	//          you must subtract the number of bits your source value has.
	//			if xshift-N becomes negative then you must shift right.

	static constexpr int rbits	= VIDEO_PIXEL_RCOUNT;
	static constexpr int rmask	= ((1 << VIDEO_PIXEL_RCOUNT) - 1) << VIDEO_PIXEL_RSHIFT;
	static constexpr int rshift = VIDEO_PIXEL_RCOUNT + VIDEO_PIXEL_RSHIFT;

	static constexpr int gbits	= VIDEO_PIXEL_GCOUNT;
	static constexpr int gmask	= ((1 << VIDEO_PIXEL_GCOUNT) - 1) << VIDEO_PIXEL_GSHIFT;
	static constexpr int gshift = VIDEO_PIXEL_GCOUNT + VIDEO_PIXEL_GSHIFT;

	static constexpr int bbits	= VIDEO_PIXEL_BCOUNT;
	static constexpr int bmask	= ((1 << VIDEO_PIXEL_BCOUNT) - 1) << VIDEO_PIXEL_BSHIFT;
	static constexpr int bshift = VIDEO_PIXEL_BCOUNT + VIDEO_PIXEL_BSHIFT;

	static constexpr int ibits	= VIDEO_PIXEL_ICOUNT;
	static constexpr int imask	= ((1 << VIDEO_PIXEL_ICOUNT) - 1) << VIDEO_PIXEL_ISHIFT;
	static constexpr int ishift = VIDEO_PIXEL_ICOUNT + VIDEO_PIXEL_ISHIFT;

	static constexpr uint total_colorbits = rbits + gbits + bbits + ibits;
	static constexpr uint total_colormask = rmask | gmask | bmask | imask;
	static constexpr bool is_monochrome	  = total_colorbits == 1;
	static constexpr bool is_greyscale	  = ORDER_GREY;
	static constexpr bool is_colorful	  = !ORDER_GREY;

#if VIDEO_COLOR_PIN_COUNT <= 8
	using uRGB = uint8;
#elif VIDEO_COLOR_PIN_COUNT <= 16
	using uRGB = uint16;
#endif

	uRGB raw;

	// low-level ctor, implicit casts:
	constexpr Color() noexcept : raw(0) {}
	constexpr Color(uint raw) noexcept : raw(uRGB(raw)) {} // raw value
	constexpr operator uRGB() const noexcept { return raw; }

	// high-level factory methods:
	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b) noexcept;
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b) noexcept;
	static constexpr Color fromRGB8(uint rgb) noexcept;
	static constexpr Color fromRGB4(uint rgb) noexcept;
	static constexpr Color fromGrey8(uint8 grey) noexcept;

	// blend this color with another. used for semi-transparency:
	constexpr void blend_with(const Color b) noexcept;

	constexpr int distance(const Color& b) const noexcept;
	constexpr int brightness() const noexcept; // distance to black

	// components scaled to n bit:
	constexpr uint8 red(uint b = 8) const noexcept
	{
		return rshift >= b ? uint8((raw & rmask) >> (rshift - b)) : uint8((raw & rmask) << (b - rshift));
	}
	constexpr uint8 green(uint b = 8) const noexcept
	{
		return gshift >= b ? uint8((raw & gmask) >> (gshift - b)) : uint8((raw & gmask) << (b - gshift));
	}
	constexpr uint8 blue(uint n = 8) const noexcept
	{
		return bshift >= n ? uint8((raw & bmask) >> (bshift - n)) : uint8((raw & bmask) << (n - bshift));
	}
	constexpr uint8 grey(uint b = 8) const noexcept
	{
		return ishift >= b ? uint8((raw & imask) >> (ishift - b)) : uint8((raw & imask) << (b - ishift));
	}

	// helper to calculate raw bits from components with n bits:
	static constexpr uRGB mkred(uint n, uint b = 8)
	{
		return (rshift >= b ? n << (rshift - b) : n >> (b - rshift)) & rmask;
	}
	static constexpr uRGB mkgreen(uint n, uint b = 8)
	{
		return (gshift >= b ? n << (gshift - b) : n >> (b - gshift)) & gmask;
	}
	static constexpr uRGB mkblue(uint n, uint b = 8)
	{
		return (bshift >= b ? n << (bshift - b) : n >> (b - bshift)) & bmask;
	}
	static constexpr uRGB mkgrey(uint n, uint b = 8)
	{
		return (ishift >= b ? n << (ishift - b) : n >> (b - ishift)) & imask;
	}
};

static_assert(sizeof(Color) == sizeof(Color::uRGB));


// =========================== Implementations ================================

constexpr Color Color::fromRGB8(uint8 r, uint8 g, uint8 b) noexcept
{
	if constexpr (ORDER_GREY) { return Color(r * 85 + g * 107 + b * 64) >> (16 - ibits); }
	if constexpr (ORDER_RGB) { return Color(mkred(r) + mkgreen(g) + mkblue(b)); }
	if constexpr (ORDER_RGBI)
	{
		// faster: take the common low bits only from the green value
		// better: average of all 3 components, weighted 4+5+3 (scaled to 256: 85+107+64 to avoid division)
		constexpr bool faster = true;

		uint8 grey = faster ? uint8(g << (gbits + 8)) :
							  (uint8(r << rbits) * 85 + uint8(g << gbits) * 107 + uint8(b << bbits) * 64);

		return Color(mkred(r) + mkgreen(g) + mkblue(b) + mkgrey(grey, 16));
	}
}

constexpr Color Color::fromRGB4(uint8 r, uint8 g, uint8 b) noexcept
{
	if constexpr (ORDER_GREY) { return Color(r * 85 + g * 107 + b * 64) >> (12 - ibits); }
	if constexpr (ORDER_RGB) { return Color(mkred(r, 4) + mkgreen(g, 4) + mkblue(b, 4)); }
	if constexpr (ORDER_RGBI)
	{
		// faster: take the common low bits only from the green value
		// better: average of all 3 components, weighted 4+5+3 (scaled to 256: 85+107+64 to avoid division)
		constexpr bool faster = true;

		if constexpr (faster)
		{
			uint8 grey = uint8(g << gbits);
			return Color(mkred(r, 4) + mkgreen(g, 4) + mkblue(b, 4) + mkgrey(grey, 8));
		}
		else
		{
			uint grey = uint8(r << rbits) * 85 + uint8(g << gbits) * 107 + uint8(b << bbits) * 64;
			return Color(mkred(r, 4) + mkgreen(g, 4) + mkblue(b, 4) + mkgrey(grey, 12));
		}
	}
}

constexpr Color Color::fromRGB8(uint rgb) noexcept // e.g. rgb = 0x00ffffaa
{
	return fromRGB8(uint8(rgb >> 16), uint8(rgb >> 8), uint8(rgb));
}

constexpr Color Color::fromRGB4(uint rgb) noexcept // e.g. rgb = 0x0ffa
{
	return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf);
}

constexpr Color Color::fromGrey8(uint8 grey) noexcept
{
	if constexpr (ORDER_GREY) { return Color(grey >> (8 - ibits)); }
	if constexpr (ORDER_RGB) { return Color(mkred(grey) + mkgreen(grey) + mkblue(grey)); }
	if constexpr (ORDER_RGBI) { return Color(mkred(grey) + mkgreen(grey) + mkblue(grey) + mkgrey(grey, 8 - bbits)); }
}

constexpr void Color::blend_with(const Color b) noexcept
{
	// this function must be *fast* because it is used to draw translucent sprites in the video compositor

	if constexpr (ORDER_GREY) { raw = (raw + b.raw + 1) >> 1; }
	else if constexpr (ORDER_RGB || ORDER_RGBI)
	{
		constexpr uint lsb = 1 << VIDEO_PIXEL_RSHIFT | 1 << VIDEO_PIXEL_GSHIFT | 1 << VIDEO_PIXEL_BSHIFT |
							 (ORDER_RGBI << VIDEO_PIXEL_ISHIFT);
		constexpr uint allbits = rmask + gmask + bmask + imask;
		constexpr uint mask	   = allbits - lsb;

		uRGB cy = (raw | b.raw) & lsb;
		raw		= (((raw & mask) + (b.raw & mask)) >> 1) + cy;
	}
}

inline constexpr int Color::brightness() const noexcept
{
	// distance to black
	// color components are weighted r=4, g=5, b=3
	
	if constexpr (ORDER_GREY) { return raw&imask; }
	if constexpr (ORDER_RGB) { return red(8) * 4 + green(8) * 5 + blue(8) * 3; }
	if constexpr (ORDER_RGBI) { return grey(8 - gbits) * (4 + 5 + 3) + red(8) * 4 + green(8) * 5 + blue(8) * 3; }
}

inline constexpr int Color::distance(const Color& b) const noexcept
{
	// color components are weighted r=4, g=5, b=3

	if constexpr (ORDER_GREY) { return abs(raw - b.raw); }
	if constexpr (ORDER_RGB)
	{
		int delta = abs(red(rshift) - b.red(rshift)) * (4 << (16 - rshift));
		delta += abs(green(gshift) - b.green(gshift)) * (5 << (16 - gshift));
		delta += abs(blue(bshift) - b.blue(bshift)) * (3 << (16 - bshift));
		return delta;
	}
	if constexpr (ORDER_RGBI)
	{
		const int deltagrey = grey(ibits) - b.grey(ibits);

		int delta = abs(red(rbits + ibits) - b.red(rbits + ibits) + deltagrey) * 4;
		delta += abs(green(gbits + ibits) - b.green(gbits + ibits) + deltagrey) * 5;
		delta += abs(blue(bbits + ibits) - b.blue(bbits + ibits) + deltagrey) * 3;
		return delta;
	}
}


// =========================== Some Basic Colors ================================

constexpr Color black		   = Color::fromRGB8(0x00, 0x00, 0x00);
constexpr Color dark_grey	   = Color::fromRGB8(0x44, 0x44, 0x44);
constexpr Color grey		   = Color::fromRGB8(0x88, 0x88, 0x88);
constexpr Color blue		   = Color::fromRGB8(0x00, 0x00, 0xCC);
constexpr Color red			   = Color::fromRGB8(0xCC, 0x00, 0x00);
constexpr Color magenta		   = Color::fromRGB8(0xCC, 0x00, 0xCC);
constexpr Color green		   = Color::fromRGB8(0x00, 0xCC, 0x00);
constexpr Color cyan		   = Color::fromRGB8(0x00, 0xCC, 0xCC);
constexpr Color yellow		   = Color::fromRGB8(0xCC, 0xCC, 0x00);
constexpr Color white		   = Color::fromRGB8(0xCC, 0xCC, 0xCC);
constexpr Color bright_blue	   = Color::fromRGB8(0x00, 0x00, 0xFF);
constexpr Color bright_red	   = Color::fromRGB8(0xFF, 0x00, 0x00);
constexpr Color bright_magenta = Color::fromRGB8(0xFF, 0x00, 0xFF);
constexpr Color bright_green   = Color::fromRGB8(0x00, 0xFF, 0x00);
constexpr Color bright_cyan	   = Color::fromRGB8(0x00, 0xFF, 0xFF);
constexpr Color bright_yellow  = Color::fromRGB8(0xFF, 0xFF, 0x00);
constexpr Color bright_white   = Color::fromRGB8(0xFF, 0xFF, 0xFF);

// 4 bit VGA colors:
// note: CGA RGBI monitors used to reduce G in yellow to 0x55
// --> https://en.wikipedia.org/wiki/ANSI_escape_code#3-bit_and_4-bit
namespace vga
{
constexpr Color black	= Color::fromRGB8(0x00, 0x00, 0x00);
constexpr Color blue	= Color::fromRGB8(0x00, 0x00, 0xAA);
constexpr Color red		= Color::fromRGB8(0xAA, 0x00, 0x00);
constexpr Color magenta = Color::fromRGB8(0xAA, 0x00, 0xAA);
constexpr Color green	= Color::fromRGB8(0x00, 0xAA, 0x00);
constexpr Color cyan	= Color::fromRGB8(0x00, 0xAA, 0xAA);
constexpr Color yellow	= Color::fromRGB8(0xAA, 0xAA, 0x00);
constexpr Color white	= Color::fromRGB8(0xAA, 0xAA, 0xAA);

constexpr Color bright_black   = Color::fromRGB8(0x55, 0x55, 0x55);
constexpr Color bright_blue	   = Color::fromRGB8(0x55, 0x55, 0xFF);
constexpr Color bright_red	   = Color::fromRGB8(0xFF, 0x55, 0x55);
constexpr Color bright_magenta = Color::fromRGB8(0xFF, 0x55, 0xFF);
constexpr Color bright_green   = Color::fromRGB8(0x55, 0xFF, 0x55);
constexpr Color bright_cyan	   = Color::fromRGB8(0x55, 0xFF, 0xFF);
constexpr Color bright_yellow  = Color::fromRGB8(0xFF, 0xFF, 0x55);
constexpr Color bright_white   = Color::fromRGB8(0xFF, 0xFF, 0xFF);

constexpr Color light_grey = white;
constexpr Color dark_grey  = bright_black;
} // namespace vga

} // namespace kio::Graphics


inline cstr tostr(kio::Graphics::Color c)
{
	using namespace kio::Graphics;
	char bu[20];

#if (ORDER_GREY)
	snprintf(bu, 20, "grey=%02x", c.grey());
#elif (ORDER_RGB)
	snprintf(bu, 20, "rgb=%02x,%02x,%02x", c.red(), c.green(), c.blue());
#elif (ORDER_RGBI)
	snprintf(
		bu, 20, "rgb=%02x,%02x,%02x", c.red() + c.grey(8u - Color::rbits), c.green() + c.grey(8u - Color::gbits),
		c.blue() + c.grey(8u - Color::bbits));
#endif

	return kio::dupstr(bu);
}
