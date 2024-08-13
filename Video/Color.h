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
	RGBI with common low bits and monochrome GREY colors,
	which should cover almost all cases.
*/
struct Color
{
#define ORDER_RGB  (VIDEO_PIXEL_ICOUNT == 0 && VIDEO_PIXEL_RSHIFT == 0 && VIDEO_PIXEL_GSHIFT < VIDEO_PIXEL_BSHIFT)
#define ORDER_BGR  (VIDEO_PIXEL_ICOUNT == 0 && VIDEO_PIXEL_BSHIFT == 0 && VIDEO_PIXEL_GSHIFT < VIDEO_PIXEL_RSHIFT)
#define ORDER_RGBI (VIDEO_PIXEL_ICOUNT != 0 && VIDEO_PIXEL_RSHIFT == 0 && VIDEO_PIXEL_GSHIFT < VIDEO_PIXEL_BSHIFT)
#define ORDER_GREY (VIDEO_PIXEL_ICOUNT == VIDEO_COLOR_PIN_COUNT)

	static_assert(ORDER_RGB + ORDER_BGR + ORDER_RGBI + ORDER_GREY == 1);

#if ORDER_RGB
	// the vgaboard uses RGB565
	// the kiboard uses RGB444
	// there may be unused bits between R and G, and between G and B block.
	// these are added as fake additional low bits to the other color.
	// e.g. in the vgaboard rgb555 becomes rgb565 because there is one unused bit between R and G.
	static constexpr int rshift = 0;
	static constexpr int rbits	= VIDEO_PIXEL_RCOUNT;
	static constexpr int gshift = rshift + rbits;
	static constexpr int gbits	= VIDEO_PIXEL_GCOUNT + VIDEO_PIXEL_GSHIFT - gshift;
	static constexpr int bshift = gshift + gbits;
	static constexpr int bbits	= VIDEO_PIXEL_BCOUNT + VIDEO_PIXEL_BSHIFT - bshift;
	static constexpr int ibits	= 0;
	static constexpr int ishift = 0;

#elif ORDER_RGBI
	// video output may include one or two 'common low bits' for all 3 channels
	// there may be unused bits between B and I, but there must not be unused bits between R and G and B.
	// these are added as fake additional low bits to the I block.
	static constexpr int rshift = 0;
	static constexpr int rbits	= VIDEO_PIXEL_RCOUNT;
	static constexpr int gshift = VIDEO_PIXEL_GSHIFT;
	static constexpr int gbits	= VIDEO_PIXEL_GCOUNT;
	static constexpr int bshift = VIDEO_PIXEL_BSHIFT;
	static constexpr int bbits	= VIDEO_PIXEL_BCOUNT;
	static constexpr int ishift = bshift + bbits;
	static constexpr int ibits	= VIDEO_PIXEL_ICOUNT + VIDEO_PIXEL_ISHIFT - ishift;
	static_assert(gbits > 0 && gbits == rbits && bbits == rbits && ibits > 0);
	static_assert(gshift == rshift + rbits);
	static_assert(bshift == gshift + gbits);
	static_assert(ishift >= bshift + bbits);

#elif ORDER_BGR
	// the picomite, a Pico-based Basic computer, uses BGR121
	// there may be unused bits between B and G, and between G and R block.
	// these are added as fake additional low bits to the other color.
	static constexpr int bshift = 0;
	static constexpr int bbits	= VIDEO_PIXEL_BCOUNT;
	static constexpr int gshift = bshift + bbits;
	static constexpr int gbits	= VIDEO_PIXEL_GCOUNT + VIDEO_PIXEL_GSHIFT - gshift;
	static constexpr int rshift = gshift + gbits;
	static constexpr int rbits	= VIDEO_PIXEL_RCOUNT + VIDEO_PIXEL_RSHIFT - rshift;
	static constexpr int ibits	= 0;
	static constexpr int ishift = 0;

#elif ORDER_GREY
	// video output may be monochrome, possibly even 1 bit b&w only:
	static constexpr int ishift = 0;
	static constexpr int ibits	= VIDEO_PIXEL_ICOUNT;
	static constexpr int rbits	= 0;
	static constexpr int gbits	= 0;
	static constexpr int bbits	= 0;
	static constexpr int rshift = 0;
	static constexpr int gshift = 0;
	static constexpr int bshift = 0;
#endif

#if VIDEO_COLOR_PIN_COUNT <= 8
	using uRGB = uint8;
#elif VIDEO_COLOR_PIN_COUNT <= 16
	using uRGB = uint16;
#endif

	static constexpr uint total_colorbits = rbits + gbits + bbits + ibits;
	static constexpr bool is_monochrome	  = total_colorbits == 1;
	static constexpr bool is_greyscale	  = ORDER_GREY;
	static constexpr bool is_colorful	  = !ORDER_GREY;

	union
	{
		uRGB rgb;
		struct
		{
#if ORDER_RGB
			uRGB red : rbits;
			uRGB green : gbits;
			uRGB blue : bbits;
#elif ORDER_RGBI
			uRGB red : rbits;
			uRGB green : gbits;
			uRGB blue : bbits;
			uRGB grey : ibits;
#elif ORDER_BGR
			uRGB blue : bbits;
			uRGB green : gbits;
			uRGB red : rbits;
#elif ORDER_GREY
			uRGB grey : ibits;
#endif
		};
	};

	// low-level ctor, implicit casts:
	constexpr Color() noexcept : rgb(0) {}
	constexpr Color(uint raw) noexcept : rgb(uRGB(raw)) {}		  // GREY or raw value
	constexpr Color(uint8 r, uint8 g, uint8 b) noexcept;		  // RGB or BGR only
	constexpr Color(uint8 r, uint8 g, uint8 b, uint8 i) noexcept; // RGBI only
	constexpr operator uRGB() const { return rgb; }

	// high-level factory methods:
	static constexpr Color fromRGB8(uint8 r, uint8 g, uint8 b);
	static constexpr Color fromRGB4(uint8 r, uint8 g, uint8 b);
	static constexpr Color fromRGB8(uint rgb);
	static constexpr Color fromRGB4(uint rgb);
	static constexpr Color fromGrey8(uint8 grey);

	// blend this color with another. used for semi-transparency:
	constexpr void blend_with(Color b) noexcept;

	int distance(const Color& b);
};

static_assert(sizeof(Color) == sizeof(Color::uRGB));


// =========================== Implementations ================================

#if ORDER_RGB
constexpr Color::Color(uint8 r, uint8 g, uint8 b) noexcept : red(r), green(g), blue(b) {}
#elif ORDER_BGR
constexpr Color::Color(uint8 r, uint8 g, uint8 b) noexcept : blue(b), green(g), red(r) {}
#elif ORDER_RGBI
constexpr Color::Color(uint8 r, uint8 g, uint8 b, uint8 i) noexcept : red(r), green(g), blue(b), grey(i) {}
#endif

constexpr Color Color::fromRGB8(uint8 r, uint8 g, uint8 b)
{
	if constexpr (ORDER_GREY) { return Color(r * 85 + g * 107 + b * 64) >> (16 - ibits); }
	if constexpr (ORDER_RGB || ORDER_BGR) { return Color(r >> (8 - rbits), g >> (8 - gbits), b >> (8 - bbits)); }
	if constexpr (ORDER_RGBI)
	{
		// faster: take the common low bits only from the green value
		// better: average of all 3 components, weighted 4+5+3 (scaled to 256: 85+107+64 to avoid division)
		constexpr bool faster = true;

		uint8 grey = faster ?
						 uint8(g << gbits) >> (8 - ibits) :
						 (uint8(r << rbits) * 85 + uint8(g << gbits) * 107 + uint8(b << bbits) * 64) >> (16 - ibits);

		return Color(r >> (8 - rbits), g >> (8 - gbits), b >> (8 - bbits), grey);
	}
}

constexpr Color Color::fromRGB4(uint8 r, uint8 g, uint8 b)
{
	if constexpr (ORDER_GREY) { return Color(r * 85 + g * 107 + b * 64) >> (12 - ibits); }
	if constexpr (ORDER_RGB || ORDER_BGR)
	{
		constexpr bool all_le4 = rbits <= 4 && gbits <= 4 && bbits <= 4;
		if constexpr (all_le4) return Color(r >> (4 - rbits), g >> (4 - gbits), b >> (4 - bbits));
		else return Color(uint8(r << (rbits - 4)), uint8(g << (gbits - 4)), uint8(b << (bbits - 4)));
	}
	if constexpr (ORDER_RGBI)
	{
		// faster: take the common low bits only from the green value
		// better: average of all 3 components, weighted 4+5+3 (scaled to 256: 85+107+64 to avoid division)
		constexpr bool faster = true;

		uint8 grey =
			faster ? uint8(g << (gbits + 4)) >> (8 - ibits) :
					 (uint8(r << (rbits + 4)) * 85 + uint8(g << (gbits + 4)) * 107 + uint8(b << (bbits + 4)) * 64) >>
						 (16 - ibits);

		return Color(r >> (4 - rbits), g >> (4 - gbits), b >> (4 - bbits), grey);
	}
}

constexpr Color Color::fromRGB8(uint rgb) // e.g. rgb = 0x00ffffaa
{
	return fromRGB8(uint8(rgb >> 16), uint8(rgb >> 8), uint8(rgb));
}

constexpr Color Color::fromRGB4(uint rgb) // e.g. rgb = 0x0ffa
{
	if constexpr (ORDER_RGB || ORDER_BGR)
	{
		constexpr bool all_eq4 = rbits == 4 && gbits == 4 && bbits == 4;
		if constexpr (all_eq4) return Color(rgb);
	}

	return fromRGB4((rgb >> 8) & 0xf, (rgb >> 4) & 0xf, rgb & 0xf);
}

constexpr Color Color::fromGrey8(uint8 grey)
{
	if constexpr (ORDER_GREY) { return Color(grey >> (8 - ibits)); }
	if constexpr (ORDER_RGB || ORDER_BGR)
	{
		return Color(grey >> (8 - rbits), grey >> (8 - gbits), grey >> (8 - bbits));
	}
	if constexpr (ORDER_RGBI)
	{
		return Color(grey >> (8 - rbits), grey >> (8 - gbits), grey >> (8 - bbits), grey >> (8 - gbits - ibits));
	}
}

constexpr void Color::blend_with(Color b) noexcept
{
	// this function must be *fast* because it is used to draw translucent sprites in the video compositor

	if constexpr (ORDER_GREY) { rgb = (rgb + b.rgb) >> 1; }
	if constexpr (ORDER_RGB)
	{
		constexpr int lsb = (1 << rshift) | (1 << gshift) | (1 << bshift) | (1 << (bshift + bbits));

		uRGB roundup = (rgb | b.rgb) & lsb;
		rgb			 = (((rgb & ~lsb) + (b.rgb & ~lsb)) >> 1) + roundup;
	}
	if constexpr (ORDER_BGR)
	{
		constexpr int lsb = (1 << rshift) | (1 << gshift) | (1 << bshift) | (1 << (rshift + rbits));

		uRGB roundup = (rgb | b.rgb) & lsb;
		rgb			 = (((rgb & ~lsb) + (b.rgb & ~lsb)) >> 1) + roundup;
	}
	if constexpr (ORDER_RGBI)
	{
		constexpr int lsb = (1 << rshift) | (1 << gshift) | (1 << bshift) | (1 << ishift) | (1 << (ishift + ibits));

		uRGB roundup = (rgb | b.rgb) & lsb;
		rgb			 = (((rgb & ~lsb) + (b.rgb & ~lsb)) >> 1) + roundup;
	}
}

inline int Color::distance(const Color& b)
{
	// color components are weighted r=4, g=5, b=3

#if (ORDER_GREY)
	return abs(grey - b.grey);
#elif (ORDER_RGB || ORDER_BGR)
	return abs(red - b.red) * (4 << (gbits - rbits)) + abs(green - b.green) * 5 +
		   abs(blue - b.blue) * (3 << (gbits - bbits));
#elif (ORDER_RGBI)
	int delta = abs(((red << ibits) + grey) - ((b.red << ibits) + b.grey)) * 3;
	delta += abs(((green << ibits) + grey) - ((b.green << ibits) + b.grey)) * 3;
	delta += abs(((blue << ibits) + grey) - ((b.blue << ibits) + b.grey)) * 3;
	return delta;
#endif
}


#if ORDER_RGB || ORDER_BGR
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).red == 2 << Color::rbits >> 4);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).green == 3 << Color::gbits >> 4);
static_assert(Color::fromRGB4(0xf2, 0xf3, 0xf4).blue == 4 << Color::bbits >> 4);
#endif


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
constexpr Color black		   = Color::fromRGB8(0x00, 0x00, 0x00);
constexpr Color dark_grey	   = Color::fromRGB8(0x55, 0x55, 0x55);
constexpr Color grey		   = Color::fromRGB8(0xAA, 0xAA, 0xAA);
constexpr Color blue		   = Color::fromRGB8(0x00, 0x00, 0xAA);
constexpr Color red			   = Color::fromRGB8(0xAA, 0x00, 0x00);
constexpr Color magenta		   = Color::fromRGB8(0xAA, 0x00, 0xAA);
constexpr Color green		   = Color::fromRGB8(0x00, 0xAA, 0x00);
constexpr Color cyan		   = Color::fromRGB8(0x00, 0xAA, 0xAA);
constexpr Color yellow		   = Color::fromRGB8(0xAA, 0xAA, 0x00);
constexpr Color light_grey	   = Color::fromRGB8(0xAA, 0xAA, 0xAA);
constexpr Color bright_blue	   = Color::fromRGB8(0x55, 0x55, 0xFF);
constexpr Color bright_red	   = Color::fromRGB8(0xFF, 0x55, 0x55);
constexpr Color bright_magenta = Color::fromRGB8(0xFF, 0x55, 0xFF);
constexpr Color bright_green   = Color::fromRGB8(0x55, 0xFF, 0x55);
constexpr Color bright_cyan	   = Color::fromRGB8(0x55, 0xFF, 0xFF);
constexpr Color bright_yellow  = Color::fromRGB8(0xFF, 0xFF, 0x55);
constexpr Color bright_white   = Color::fromRGB8(0xFF, 0xFF, 0xFF);
} // namespace vga

} // namespace kio::Video


inline cstr tostr(kio::Video::Color c)
{
	char bu[16];

#if (ORDER_GREY)
	snprintf(bu, 16, "grey=%u", c.grey);
#elif (ORDER_RGB || ORDER_BGR)
	snprintf(bu, 16, "rgb=%u,%u,%u", c.red, c.green, c.blue);
#elif (ORDER_RGBI)
	snprintf(
		bu, 16, "rgb=%u,%u,%u", (c.red << c.ibits) + c.grey, (c.green << c.ibits) + c.grey,
		(c.blue << c.ibits) + c.grey);
#endif

	return kio::dupstr(bu);
}
