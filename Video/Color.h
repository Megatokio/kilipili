// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include "tempmem.h"
#include "video_options.h"
#include <cstdio>

namespace kio::Video
{

/*
	A `Color` represents what is used by the Video Hardware.
	It is used in the library to represent a `true color`.
	It is configurable by #defines in the `boards` header file.
	Currently supported are 8bit and 16 bit color in RGB and BGR order,
	which should cover almost all cases.
*/
struct Color
{
#define ORDER_RGB (VIDEO_PIXEL_RSHIFT == 0 && VIDEO_PIXEL_GSHIFT < VIDEO_PIXEL_BSHIFT)
#define ORDER_BGR (VIDEO_PIXEL_BSHIFT == 0 && VIDEO_PIXEL_GSHIFT < VIDEO_PIXEL_RSHIFT)
	static_assert(ORDER_RGB || ORDER_BGR);

#if ORDER_RGB
	// the vgaboard uses RGB565
	// the kiboard uses RGB444
	static constexpr int rshift = 0;
	static constexpr int rbits	= VIDEO_PIXEL_RCOUNT;
	static constexpr int gshift = rshift + rbits;
	static constexpr int gbits	= VIDEO_PIXEL_GCOUNT + VIDEO_PIXEL_GSHIFT - gshift;
	static constexpr int bshift = gshift + gbits;
	static constexpr int bbits	= VIDEO_PIXEL_BCOUNT + VIDEO_PIXEL_BSHIFT - bshift;
#else
	// the picomite, a Pico-based Basic computer, uses BGR121
	static constexpr int bshift = 0;
	static constexpr int bbits	= VIDEO_PIXEL_BCOUNT;
	static constexpr int gshift = bshift + bbits;
	static constexpr int gbits	= VIDEO_PIXEL_GCOUNT + VIDEO_PIXEL_GSHIFT - gshift;
	static constexpr int rshift = gshift + gbits;
	static constexpr int rbits	= VIDEO_PIXEL_RCOUNT + VIDEO_PIXEL_RSHIFT - rshift;
#endif

#if VIDEO_COLOR_PIN_COUNT <= 8
	using uRGB = uint8;
#else
	using uRGB					= uint16;
#endif

	union
	{
		uRGB rgb;
		struct
		{
#if ORDER_RGB
			uRGB red : rbits;
			uRGB green : gbits;
			uRGB blue : bbits;
#else
			 uRGB blue : bbits;
			 uRGB green : gbits;
			 uRGB red : rbits;
#endif
		};
	};

	// low-level ctor, implicit casts:
	constexpr Color() noexcept : rgb(0) {}
	constexpr Color(uint rgb) noexcept : rgb(uRGB(rgb)) {}
	constexpr Color(uint8 r, uint8 g, uint8 b) noexcept;
	constexpr operator uRGB() const { return rgb; }

	// high-level factory methods:
	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b);
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b);
	static constexpr Color fromRGB8(uint rgb);
	static constexpr Color fromRGB4(uint rgb);

	// blend this color with another. used for semi-transparency:
	constexpr void blend_with(Color b) noexcept;

	int distance(const Color& b)
	{
		// color components are weighted r=3, g=4, b=2
		return abs(red - b.red) * (3 << (gbits - rbits)) + abs(green - b.green) * 4 +
			   abs(blue - b.blue) * (2 << (gbits - bbits));
	}
};

static_assert(sizeof(Color) == sizeof(Color::uRGB));


// =========================== Implementations ================================

#if ORDER_RGB
constexpr Color::Color(uint8 r, uint8 g, uint8 b) noexcept : red(r), green(g), blue(b) {}
#else
constexpr Color::Color(uint8 r, uint8 g, uint8 b) noexcept : blue(b), green(g), red(r) {}
#endif

constexpr Color Color::fromRGB8(uint8 r, uint8 g, uint8 b)
{
	return Color(r >> (8 - rbits), g >> (8 - gbits), b >> (8 - bbits));
}

constexpr Color Color::fromRGB4(uint8 r, uint8 g, uint8 b)
{
	constexpr bool all_le4 = rbits <= 4 && gbits <= 4 && bbits <= 4;

	if constexpr (all_le4) return Color(r >> (4 - rbits), g >> (4 - gbits), b >> (4 - bbits));
	else return Color(uint8(r << (rbits - 4)), uint8(g << (gbits - 4)), uint8(b << (bbits - 4)));
}

constexpr Color Color::fromRGB8(uint rgb) //
{
	return fromRGB8(uint8(rgb >> 16), uint8(rgb >> 8), uint8(rgb));
}

constexpr Color Color::fromRGB4(uint rgb)
{
	constexpr bool all_eq4 = rbits == 4 && gbits == 4 && bbits == 4;

	if constexpr (all_eq4) return Color(rgb);
	else return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf);
}

constexpr void Color::blend_with(Color b) noexcept
{
	constexpr int lsb = (1 << rshift) | (1 << gshift) | (1 << bshift) | (1 << (bshift + bbits));

	uRGB roundup = (rgb | b.rgb) & lsb;
	rgb			 = (((rgb & ~lsb) + (b.rgb & ~lsb)) >> 1) + roundup;
}

static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).red == 2 << Color::rbits >> 4);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).green == 3 << Color::gbits >> 4);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).blue == 4 << Color::bbits >> 4);


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

} // namespace kio::Video


inline cstr tostr(kio::Video::Color c)
{
	char bu[16];
	snprintf(bu, 16, "rgb=%u,%u,%u", c.red, c.green, c.blue);
	return kio::dupstr(bu);
}
