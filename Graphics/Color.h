// Copyright (c) 2022 - 2025 kio@little-bat.de
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
	// xshift = number of bits the component is shifted left in Color::raw.
	// xmask  = bitmask for component in Color::raw.

	static constexpr int rbits	= VIDEO_PIXEL_RCOUNT;
	static constexpr int rshift = VIDEO_PIXEL_RSHIFT;
	static constexpr int rmask	= ((1 << rbits) - 1) << rshift;

	static constexpr int gbits	= VIDEO_PIXEL_GCOUNT;
	static constexpr int gshift = VIDEO_PIXEL_GSHIFT;
	static constexpr int gmask	= ((1 << gbits) - 1) << gshift;

	static constexpr int bbits	= VIDEO_PIXEL_BCOUNT;
	static constexpr int bshift = VIDEO_PIXEL_BSHIFT;
	static constexpr int bmask	= ((1 << bbits) - 1) << bshift;

	static constexpr int ibits	= VIDEO_PIXEL_ICOUNT;
	static constexpr int ishift = VIDEO_PIXEL_ISHIFT;
	static constexpr int imask	= ((1 << ibits) - 1) << ishift;

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
	constexpr Color(int raw) noexcept : raw(uRGB(raw)) {}  // raw value
	constexpr operator uRGB() const noexcept { return raw; }

	// high-level factory methods:
	static constexpr Color fromRGB8(int r, int g, int b) noexcept;
	static constexpr Color fromRGB4(int r, int g, int b) noexcept;
	static constexpr Color fromRGB8(uint rgb) noexcept;
	static constexpr Color fromRGB4(uint rgb) noexcept;
	static constexpr Color fromGrey8(int grey) noexcept;

	// blend this color with another. used for semi-transparency:
	constexpr void blend_with(const Color b) noexcept;

	constexpr int distance(const Color& b) const noexcept;
	constexpr int brightness() const noexcept; // distance to black

	// get the component value:
	constexpr uint8 red() const noexcept { return (raw & rmask) >> rshift; }
	constexpr uint8 green() const noexcept { return (raw & gmask) >> gshift; }
	constexpr uint8 blue() const noexcept { return (raw & bmask) >> bshift; }
	constexpr uint8 grey() const noexcept { return (raw & imask) >> ishift; }

	// shift component value into place for .raw:
	static constexpr uRGB mkred(int n) { return (n << rshift) & rmask; }
	static constexpr uRGB mkgreen(int n) { return (n << gshift) & gmask; }
	static constexpr uRGB mkblue(int n) { return (n << bshift) & bmask; }
	static constexpr uRGB mkgrey(int n) { return (n << ishift) & imask; }

	// get the component value scaled to bits:
	constexpr uint8 red(int bits) const noexcept;
	constexpr uint8 green(int bits) const noexcept;
	constexpr uint8 blue(int bits) const noexcept;
	constexpr uint8 grey(int bits) const noexcept;

	// calculate raw bits from component n with bits:
	static constexpr uRGB mkred(int n, int bits);
	static constexpr uRGB mkgreen(int n, int bits);
	static constexpr uRGB mkblue(int n, int bits);
	static constexpr uRGB mkgrey(int n, int bits);
};

static_assert(sizeof(Color) == sizeof(Color::uRGB));


// =========================== Implementations ================================

constexpr uint8 Color::red(int bits) const noexcept
{
	int d = rshift + rbits - bits;
	return uint8(d >= 0 ? (raw & rmask) >> d : (raw & rmask) << -d);
}
constexpr uint8 Color::green(int bits) const noexcept
{
	int d = gshift + gbits - bits;
	return uint8(d >= 0 ? (raw & gmask) >> d : (raw & gmask) << -d);
}
constexpr uint8 Color::blue(int bits) const noexcept
{
	int d = bshift + bbits - bits;
	return uint8(d >= 0 ? (raw & bmask) >> d : (raw & bmask) << -d);
}
constexpr uint8 Color::grey(int bits) const noexcept
{
	int d = ishift + ibits - bits;
	return uint8(d >= 0 ? (raw & imask) >> d : (raw & imask) << -d);
}

constexpr Color::uRGB Color::mkred(int n, int bits)
{
	int d = rshift + rbits - bits;
	return (d >= 0 ? n << d : n >> -d) & rmask;
}
constexpr Color::uRGB Color::mkgreen(int n, int bits)
{
	int d = gshift + gbits - bits;
	return (d >= 0 ? n << d : n >> -d) & gmask;
}
constexpr Color::uRGB Color::mkblue(int n, int bits)
{
	int d = bshift + bbits - bits;
	return (d >= 0 ? n << d : n >> -d) & bmask;
}
constexpr Color::uRGB Color::mkgrey(int n, int bits)
{
	int d = ishift + ibits - bits;
	return (d >= 0 ? n << d : n >> -d) & imask;
}

constexpr Color Color::fromRGB8(int r, int g, int b) noexcept
{
	if constexpr (ORDER_GREY) { return Color(r * 85 + g * 107 + b * 64) >> (16 - ibits); }
	if constexpr (ORDER_RGB) { return Color(mkred(r, 8) + mkgreen(g, 8) + mkblue(b, 8)); }
	if constexpr (ORDER_RGBI)
	{
		// faster: take the common low bits only from the green value
		// better: average of all 3 components, weighted 4+5+3 (scaled to 256: 85+107+64 to avoid division)
		constexpr bool faster = true;

		uint8 grey = faster ? uint8(g << (gbits + 8)) :
							  (uint8(r << rbits) * 85 + uint8(g << gbits) * 107 + uint8(b << bbits) * 64);

		return Color(mkred(r, 8) + mkgreen(g, 8) + mkblue(b, 8) + mkgrey(grey, 16));
	}
}

constexpr Color Color::fromRGB4(int r, int g, int b) noexcept
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
			int grey = uint8(r << rbits) * 85 + uint8(g << gbits) * 107 + uint8(b << bbits) * 64;
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

constexpr Color Color::fromGrey8(int grey) noexcept
{
	if constexpr (ORDER_GREY) { return Color(grey >> (8 - ibits)); }
	if constexpr (ORDER_RGB) { return Color(mkred(grey, 8) + mkgreen(grey, 8) + mkblue(grey, 8)); }
	if constexpr (ORDER_RGBI)
	{
		return Color(mkred(grey, 8) + mkgreen(grey, 8) + mkblue(grey, 8) + mkgrey(grey, 8 - bbits));
	}
}

constexpr void Color::blend_with(const Color b) noexcept
{
	// this function must be *fast* because it is used to draw translucent sprites in the video compositor

	if constexpr (ORDER_GREY) { raw = (raw + b.raw + 1) >> 1; }
	else if constexpr (ORDER_RGB || ORDER_RGBI)
	{
		constexpr uint lsb = 1 << VIDEO_PIXEL_RSHIFT | 1 << VIDEO_PIXEL_GSHIFT | 1 << VIDEO_PIXEL_BSHIFT |
							 (ORDER_RGBI << VIDEO_PIXEL_ISHIFT);
		constexpr uint mask = total_colormask - lsb;

		uRGB cy = (raw | b.raw) & lsb;
		raw		= (((raw & mask) + (b.raw & mask)) >> 1) + cy;
	}
}

inline constexpr int Color::brightness() const noexcept
{
	// distance to black
	// color components are weighted r=4, g=5, b=3

	if constexpr (ORDER_GREY) { return raw & imask; }
	if constexpr (ORDER_RGB) { return red(8) * 4 + green(8) * 5 + blue(8) * 3; }
	if constexpr (ORDER_RGBI) { return grey(8 - gbits) * (4 + 5 + 3) + red(8) * 4 + green(8) * 5 + blue(8) * 3; }
}

inline constexpr int Color::distance(const Color& b) const noexcept
{
	// color components are weighted r=4, g=5, b=3

	if constexpr (ORDER_GREY) { return abs(raw - b.raw); }
	if constexpr (ORDER_RGB)
	{
		static_assert(gbits >= rbits && gbits >= bbits);
		int delta = abs(red() - b.red()) * (4 << (gbits - rbits));
		delta += abs(green() - b.green()) * (5 << (gbits - gbits));
		delta += abs(blue() - b.blue()) * (3 << (gbits - bbits));
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
	snprintf(bu, 20, "rgb=%02x,%02x,%02x", c.red(8), c.green(8), c.blue(8));
#elif (ORDER_RGBI)
	snprintf(
		bu, 20, "rgb=%02x,%02x,%02x", c.red(8) + c.grey(8 - Color::rbits), c.green(8) + c.grey(8 - Color::gbits),
		c.blue(8) + c.grey(8 - Color::bbits));
#endif

	return kio::dupstr(bu);
}
