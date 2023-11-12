// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include "tempmem.h"
#include "video_options.h"
#include <cstdio>

namespace kio::Video
{

#if VIDEO_PIXEL_RCOUNT == 5 && VIDEO_PIXEL_GCOUNT == 5 && VIDEO_PIXEL_BCOUNT == 5 && VIDEO_PIXEL_RSHIFT == 0 && \
	VIDEO_PIXEL_GSHIFT == 6 && VIDEO_PIXEL_BSHIFT == 11

struct Color
{
	union
	{
		uint16 rgb;
		struct
		{
			uint16 red		: 5;
			uint16 _padding : 1;
			uint16 green	: 5;
			uint16 blue		: 5;
		};
	};

	static constexpr uint cbits = 5;		   // color components bits
	static constexpr uint css	= 8 - cbits;   // color components size shift from uint8
	static constexpr uint cmask = 0xff >> css; // color components mask
	static constexpr uint cmax	= 0xff >> css; // color components max value

	Color() noexcept = default;
	constexpr Color(uint rgb) noexcept : rgb(uint16(rgb)) {}
	constexpr Color(uint8 r, uint8 g, uint8 b) noexcept : red(r), _padding(0), green(g), blue(b) {}
	constexpr operator uint16() const { return rgb; }

	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b) { return Color(r >> css, g >> css, b >> css); }
	static constexpr Color fromRGB5(uint8 r, uint8 g, uint8 b) { return Color(r, g, b); }
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b) { return fromRGB8(r * 17, g * 17, b * 17); }

	static constexpr Color fromRGB8(uint rgb) { return fromRGB8((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff); }
	static constexpr Color fromRGB4(uint rgb) { return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf); }

	void blend_with(Color b) noexcept
	{
		uint16 roundup = (rgb | b.rgb) & 0b0000100001000001;
		rgb			   = ((rgb >> 1) & 0b0111101111001111) + ((b.rgb >> 1) & 0b0111101111001111) + roundup;
	}
};

static_assert(sizeof(Color) == sizeof(uint16));
static_assert(Color::fromRGB5(5, 0, 0).red == 5);
static_assert(Color::fromRGB5(0x22, 0x33, 0x44).red == 2);
static_assert(Color::fromRGB5(0x22, 0x33, 0x44).green == 0x13);
static_assert(Color::fromRGB5(0x22, 0x33, 0x44).blue == 4);
#endif

#if VIDEO_PIXEL_RCOUNT == 3 && VIDEO_PIXEL_GCOUNT == 3 && VIDEO_PIXEL_BCOUNT == 2 && VIDEO_PIXEL_RSHIFT == 0 && \
	VIDEO_PIXEL_GSHIFT == 3 && VIDEO_PIXEL_BSHIFT == 6

struct Color
{
	union
	{
		uint8 rgb;
		struct
		{
			uint8 red	: 3;
			uint8 green : 3;
			uint8 blue	: 2;
		};
	};

	static constexpr uint cbits = 3;		   // color components bits
	static constexpr uint css	= 8 - cbits;   // color components size shift from uint8
	static constexpr uint cmask = 0xff >> css; // color components mask
	static constexpr uint cmax	= 0xff >> css; // color components max value

	Color() noexcept = default;
	constexpr Color(uint rgb) noexcept : rgb(uint8(rgb)) {}
	constexpr Color(uint8 r, uint8 g, uint8 b) noexcept : red(r), green(g), blue(b >> 1) {}
	constexpr operator uint8() const { return rgb; }

	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b) { return Color(r >> css, g >> css, b >> css); }
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b) { return Color(r >> 1, g >> 1, b >> 2); }

	static constexpr Color fromRGB8(uint rgb) { return fromRGB8((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff); }
	static constexpr Color fromRGB4(uint rgb) { return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf); }

	void blend_with(Color b) noexcept
	{
		uint8 roundup = (rgb | b.rgb) & 0b01001001;
		rgb			  = ((rgb >> 1) & 0b01011011) + ((b.rgb >> 1) & 0b01011011) + roundup;
	}
};
#endif

#if VIDEO_PIXEL_RCOUNT == 2 && VIDEO_PIXEL_GCOUNT == 2 && VIDEO_PIXEL_BCOUNT == 2 && VIDEO_PIXEL_RSHIFT == 0 && \
	VIDEO_PIXEL_GSHIFT == 2 && VIDEO_PIXEL_BSHIFT == 4

struct Color
{
	union
	{
		uint8 rgb;
		struct
		{
			uint8 red	   : 2;
			uint8 green	   : 2;
			uint8 blue	   : 2;
			uint8 _padding : 2;
		};
	};

	static constexpr uint cbits = 2;		   // color components bits
	static constexpr uint css	= 8 - cbits;   // color components size shift from uint8
	static constexpr uint cmask = 0xff >> css; // color components mask
	static constexpr uint cmax	= 0xff >> css; // color components max value

	Color() noexcept = default;
	constexpr Color(uint rgb) noexcept : rgb(uint8(rgb)) {}
	constexpr Color(uint8 r, uint8 g, uint8 b) noexcept : red(r), green(g), blue(b), _padding(0) {}
	constexpr operator uint8() const { return rgb; }

	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b) { return Color(r >> css, g >> css, b >> css); }
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b) { return Color(r >> 2, g >> 2, b >> 2); }

	static constexpr Color fromRGB8(uint rgb) { return fromRGB8((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff); }
	static constexpr Color fromRGB4(uint rgb) { return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf); }

	void blend_with(Color b) noexcept
	{
		uint8 roundup = (rgb | b.rgb) & 0b00010101;
		rgb			  = ((rgb >> 1) & 0b00010101) + ((b.rgb >> 1) & 0b00010101) + roundup;
	}
};
#endif

#if VIDEO_PIXEL_RCOUNT == 4 && VIDEO_PIXEL_GCOUNT == 4 && VIDEO_PIXEL_BCOUNT == 4 && VIDEO_PIXEL_RSHIFT == 0 && \
	VIDEO_PIXEL_GSHIFT == 4 && VIDEO_PIXEL_BSHIFT == 8

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
			uint16 _padding : 4;
		};
	};

	static constexpr uint cbits = 4;		   // color components bits
	static constexpr uint css	= 8 - cbits;   // color components size shift from uint8
	static constexpr uint cmask = 0xff >> css; // color components mask
	static constexpr uint cmax	= 0xff >> css; // color components max value

	Color() noexcept = default;
	constexpr Color(uint rgb) noexcept : rgb(uint16(rgb)) {}
	constexpr Color(uint8 r, uint8 g, uint8 b) noexcept : red(r), green(g), blue(b), _padding(0) {}
	constexpr operator uint16() const { return rgb; }

	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b) { return Color(r >> 4, g >> 4, b >> 4); }
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b) { return Color(r, g, b); }

	static constexpr Color fromRGB8(uint rgb) { return fromRGB8((rgb >> 16) & 0xff, (rgb >> 8) & 0xff, rgb & 0xff); }
	static constexpr Color fromRGB4(uint rgb) { return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf); }

	void blend_with(Color b) noexcept
	{
		uint16 roundup = (rgb | b.rgb) & 0x0111;
		rgb			   = ((rgb >> 1) & 0x0777) + ((b.rgb >> 1) & 0x0777) + roundup;
	}
};

static_assert(sizeof(Color) == sizeof(uint16));
static_assert(Color::fromRGB4(9, 0, 0).red == 9);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).red == 2);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).green == 3);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).blue == 4);
#endif


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

} // namespace kio::Video


inline cstr tostr(kio::Video::Color c)
{
	char bu[16];
	snprintf(bu, 16, "rgb=%u,%u,%u", c.red, c.green, c.blue);
	return kio::dupstr(bu);
}
