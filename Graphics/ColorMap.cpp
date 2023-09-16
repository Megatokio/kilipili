// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ColorMap.h"

namespace kio::Graphics
{

constexpr Color default_colormap_i1[2] = {
	Color(0, 31, 0),
	Color(1, 1, 1),
};

constexpr Color default_colormap_i2[4] = {
//#define RGB(R,G,B) PICO_SCANVIDEO_PIXEL_FROM_RGB5(R*31, G*31, B*31)
//RGB(0,0,0), RGB(1,0,0), RGB(0,1,0), RGB(1,1,1)
//#undef RGB

#define GREY(N) Color::fromRGB8(N, N, N)
	GREY(0b00000000),
	GREY(0b01010101),
	GREY(0b10101010),
	GREY(0b11111111),
#undef GREY
};

constexpr Color default_colormap_i4[16] = {
// table[%rgbc] -> r(rrcc0)+g(ggcc0)+b(bbcc0)

#define RGBC(R, G, B, C) Color::fromRGB5(R * 24 + C * 6, G * 24 + C * 6, B * 24 + C * 6)
#define RGBx(R, G, B)	 RGBC(R, G, B, 0), RGBC(R, G, B, 1)
#define RGxx(R, G)		 RGBx(R, G, 0), RGBx(R, G, 1)
#define Rxxx(R)			 RGxx(R, 0), RGxx(R, 1)

	Rxxx(0), Rxxx(1)

#undef Rxxx
#undef RGxx
#undef RGBx
#undef RGBC
};

constexpr Color default_colormap_i8[256] = {
// table[%rrggbbcc] -> r(rrcc0)+g(ggcc0)+b(bbcc0)

#define RGBC(R, G, B, C) Color::fromRGB5(R * 8 + C * 2, G * 8 + C * 2, B * 8 + C * 2)

#define RGBx(R, G, B) RGBC(R, G, B, 0), RGBC(R, G, B, 1), RGBC(R, G, B, 2), RGBC(R, G, B, 3)
#define RGxx(R, G)	  RGBx(R, G, 0), RGBx(R, G, 1), RGBx(R, G, 2), RGBx(R, G, 3)
#define Rxxx(R)		  RGxx(R, 0), RGxx(R, 1), RGxx(R, 2), RGxx(R, 3)

	Rxxx(0), Rxxx(1), Rxxx(2), Rxxx(3)

#undef Rxxx
#undef RGxx
#undef RGBx
#undef RGBC
};

constexpr Color zx_colors[16] = {black,		   blue,		red,		   magenta,		green,		cyan,
								 yellow,	   white,		bright_black,  bright_blue, bright_red, bright_magenta,
								 bright_green, bright_cyan, bright_yellow, bright_white};

constexpr const Color* const default_colormaps[5] {
	default_colormap_i1, default_colormap_i2, default_colormap_i4, default_colormap_i8, nullptr};


} // namespace kio::Graphics
