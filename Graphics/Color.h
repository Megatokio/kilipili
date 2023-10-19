// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <cstdio>
#ifndef UNIT_TEST
  #include <pico/config.h>
#endif


// the vgaboard doesn't define these
// also use these for the test suite
#ifndef PICO_SCANVIDEO_ALPHA_PIN
  #define PICO_SCANVIDEO_ALPHA_PIN 5u
#endif
#ifndef PICO_SCANVIDEO_PIXEL_RSHIFT
  #define PICO_SCANVIDEO_PIXEL_RSHIFT 0u
#endif
#ifndef PICO_SCANVIDEO_PIXEL_GSHIFT
  #define PICO_SCANVIDEO_PIXEL_GSHIFT 6u
#endif
#ifndef PICO_SCANVIDEO_PIXEL_BSHIFT
  #define PICO_SCANVIDEO_PIXEL_BSHIFT 11u
#endif
#ifndef PICO_SCANVIDEO_PIXEL_RCOUNT
  #define PICO_SCANVIDEO_PIXEL_RCOUNT 5
#endif
#ifndef PICO_SCANVIDEO_PIXEL_GCOUNT
  #define PICO_SCANVIDEO_PIXEL_GCOUNT 5
#endif
#ifndef PICO_SCANVIDEO_PIXEL_BCOUNT
  #define PICO_SCANVIDEO_PIXEL_BCOUNT 5
#endif


namespace kio::Graphics
{

#if PICO_SCANVIDEO_PIXEL_RCOUNT == 5 && PICO_SCANVIDEO_PIXEL_GCOUNT == 5 && PICO_SCANVIDEO_PIXEL_BCOUNT == 5 && \
	PICO_SCANVIDEO_PIXEL_RSHIFT == 0 && PICO_SCANVIDEO_PIXEL_GSHIFT == 6 && PICO_SCANVIDEO_PIXEL_BSHIFT == 11

struct Color
{
	union
	{
		uint16 rgb;
		struct
		{
			uint16 red	 : 5;
			uint16 alpha : 1;
			uint16 green : 5;
			uint16 blue	 : 5;
		};
	};

	static constexpr uint cbits = 5;		   // color components bits
	static constexpr uint css	= 8 - cbits;   // color components size shift from uint8
	static constexpr uint cmask = 0xff >> css; // color components mask
	static constexpr uint cmax	= 0xff >> css; // color components max value

	Color() noexcept = default;
	constexpr Color(uint16 rgb) noexcept : rgb(rgb) {}
	constexpr Color(uint8 r, uint8 g, uint8 b, uint8 a = 1) noexcept : red(r), alpha(a), green(g), blue(b) {}
	constexpr	   operator uint16() const { return rgb; }
	constexpr bool is_transparent() const noexcept { return alpha == 0; }
	constexpr bool is_opaque() const noexcept { return alpha != 0; }

	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b, uint8 a = 1) { return Color(r >> 3, g >> 3, b >> 3, a); }
	static constexpr Color fromRGB5(uint8 r, uint8 g, uint8 b, uint8 a = 1) { return Color(r, g, b, a); }
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b, uint8 a = 1)
	{
		return fromRGB8(r * 17, g * 17, b * 17, a);
	}

	static constexpr Color fromRGB8(uint rgb) { return fromRGB8((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff); }
	static constexpr Color fromRGB4(uint rgb) { return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf); }
};

static_assert(sizeof(Color) == sizeof(uint16));
static_assert(Color::fromRGB5(5, 0, 0).red == 5);
static_assert(Color::fromRGB5(0x22, 0x33, 0x44, 5).red == 2);
static_assert(Color::fromRGB5(0x22, 0x33, 0x44, 5).green == 0x13);
static_assert(Color::fromRGB5(0x22, 0x33, 0x44, 5).blue == 4);
static_assert(Color::fromRGB5(0x22, 0x33, 0x44, 5).alpha == 1);
#endif

#if PICO_SCANVIDEO_PIXEL_RCOUNT == 4 && PICO_SCANVIDEO_PIXEL_GCOUNT == 4 && PICO_SCANVIDEO_PIXEL_BCOUNT == 4 && \
	PICO_SCANVIDEO_PIXEL_RSHIFT == 0 && PICO_SCANVIDEO_PIXEL_GSHIFT == 4 && PICO_SCANVIDEO_PIXEL_BSHIFT == 8

struct Color
{
	union
	{
		uint16 rgb;
		struct
		{
			uint16 red		: 4;
			uint16 green	: 4;
			uint16 blue		: 4;
			uint16 alpha	: 1;
			uint16 _padding : 3;
		};
	};

	static constexpr uint cbits = 4;		   // color components bits
	static constexpr uint css	= 8 - cbits;   // color components size shift from uint8
	static constexpr uint cmask = 0xff >> css; // color components mask
	static constexpr uint cmax	= 0xff >> css; // color components max value

	Color() noexcept = default;
	constexpr Color(uint16 rgb) noexcept : rgb(rgb) {}
	constexpr Color(uint8 r, uint8 g, uint8 b, uint8 a = 1) noexcept : red(r), green(g), blue(b), alpha(a), _padding(0)
	{}
	constexpr	   operator uint16() const { return rgb; }
	constexpr bool is_transparent() const noexcept { return alpha == 0; }
	constexpr bool is_opaque() const noexcept { return alpha != 0; }

	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b, uint8 a = 1) { return Color(r >> 4, g >> 4, b >> 4, a); }
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b, uint8 a = 1) { return Color(r, g, b, a); }

	static constexpr Color fromRGB8(uint rgb) { return fromRGB8((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff); }
	static constexpr Color fromRGB4(uint rgb) { return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf); }
};

static_assert(sizeof(Color) == sizeof(uint16));
static_assert(Color::fromRGB4(9, 0, 0).red == 9);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4, 5).red == 2);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4, 5).green == 3);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4, 5).blue == 4);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4, 5).alpha == 1);
#endif


constexpr Color transparent	   = Color::fromRGB8(0, 0, 0, 0);
constexpr Color black		   = Color::fromRGB8(0x00, 0x00, 0x00);
constexpr Color blue		   = Color::fromRGB8(0x00, 0x00, 0xCC);
constexpr Color red			   = Color::fromRGB8(0xCC, 0x00, 0x00);
constexpr Color magenta		   = Color::fromRGB8(0xCC, 0x00, 0xCC);
constexpr Color green		   = Color::fromRGB8(0x00, 0xCC, 0x00);
constexpr Color cyan		   = Color::fromRGB8(0x00, 0xCC, 0xCC);
constexpr Color yellow		   = Color::fromRGB8(0xCC, 0xCC, 0x00);
constexpr Color white		   = Color::fromRGB8(0xCC, 0xCC, 0xCC);
constexpr Color bright_black   = Color::fromRGB8(0x00, 0x00, 0x00);
constexpr Color bright_blue	   = Color::fromRGB8(0x00, 0x00, 0xFF);
constexpr Color bright_red	   = Color::fromRGB8(0xFF, 0x00, 0x00);
constexpr Color bright_magenta = Color::fromRGB8(0xFF, 0x00, 0xFF);
constexpr Color bright_green   = Color::fromRGB8(0x00, 0xFF, 0x00);
constexpr Color bright_cyan	   = Color::fromRGB8(0x00, 0xFF, 0xFF);
constexpr Color bright_yellow  = Color::fromRGB8(0xFF, 0xFF, 0x00);
constexpr Color bright_white   = Color::fromRGB8(0xFF, 0xFF, 0xFF);
constexpr Color grey		   = Color::fromRGB8(0x88, 0x88, 0x88);

} // namespace kio::Graphics


inline cstr tostr(kio::Graphics::Color c)
{
	static char bu[16];
	if (c.is_transparent()) snprintf(bu, 16, "rgba=%u,%u,%u,1", c.red, c.green, c.blue);
	else snprintf(bu, 16, "rgb=%u,%u,%u", c.red, c.green, c.blue);
	return bu;
}
