// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ColorMap.h"
#include <string.h>

#if !defined GRAPHICS_SYSTEM_COLORMAP_IN_XRAM || GRAPHICS_SYSTEM_COLORMAP_IN_XRAM
  #define XRAM __attribute__((section(".scratch_x.ColorMap"))) // the 4k page with the core1 stack
#else
  #define XRAM
#endif


namespace kio::Graphics
{

// clang-format off

constexpr Color default_i1_colors[2] =
{
	bright_green,
	Color::fromRGB8(15, 15, 15),
};

constexpr Color default_i2_colors[4] =
{
	vga::black,
	vga::dark_grey,
	vga::light_grey,
	vga::bright_white,
};

constexpr Color default_i4_colors[16] =
{
// table[%rgbc] -> r(rrcc0)+g(ggcc0)+b(bbcc0)

#define RGBC(R, G, B, C) Color::fromRGB4(R * 12 + C * 3, G * 12 + C * 3, B * 12 + C * 3)
#define RGBx(R, G, B)	 RGBC(R, G, B, 0), RGBC(R, G, B, 1)
#define RGxx(R, G)		 RGBx(R, G, 0), RGBx(R, G, 1)
#define Rxxx(R)			 RGxx(R, 0), RGxx(R, 1)

	Rxxx(0),
	Rxxx(1)

#undef Rxxx
#undef RGxx
#undef RGBx
#undef RGBC
};

constexpr Color default_i8_colors[256] =
{
// table[%rrggbbcc] -> r(rrcc0)+g(ggcc0)+b(bbcc0)

#define RGBC(R, G, B, C) Color::fromRGB4(R * 4 + C, G * 4 + C, B * 4 + C)
#define RGBx(R, G, B) RGBC(R, G, B, 0), RGBC(R, G, B, 1), RGBC(R, G, B, 2), RGBC(R, G, B, 3)
#define RGxx(R, G)	  RGBx(R, G, 0), RGBx(R, G, 1), RGBx(R, G, 2), RGBx(R, G, 3)
#define Rxxx(R)		  RGxx(R, 0), RGxx(R, 1), RGxx(R, 2), RGxx(R, 3)

	Rxxx(0),
	Rxxx(1),
	Rxxx(2),
	Rxxx(3)

#undef Rxxx
#undef RGxx
#undef RGBx
#undef RGBC
};

constexpr Color zx_colors[16] =
{ 
	black,
	blue,
	red,
	magenta,
	green,
	cyan,
	yellow,
	white,
	dark_grey,
	bright_blue,
	bright_red,
	bright_magenta,
	bright_green,
	bright_cyan,
	bright_yellow,
	bright_white
};

constexpr Color vga4_colors[16] =
{
	vga::black,
	vga::red,
	vga::green,
	vga::yellow,
	vga::blue,
	vga::magenta,
	vga::cyan,
	vga::white,
	vga::bright_black,
	vga::bright_red,
	vga::bright_green,
	vga::bright_yellow,
	vga::bright_blue,
	vga::bright_magenta,
	vga::bright_cyan,
	vga::bright_white,
};

constexpr Color vga8_colors[256] =
{
	// table starts with 16 vga4 colors
	// then 6*6*6=216 colors rgb(6) (each component 0…5, red highest, blue lowest digit order)
	// then 24 greyscales between black and white giving a total of 26 greyscales
	// --> https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit

	vga::black,
	vga::red,
	vga::green,
	vga::yellow,
	vga::blue,
	vga::magenta,
	vga::cyan,
	vga::white,
	vga::bright_black,
	vga::bright_red,
	vga::bright_green,
	vga::bright_yellow,
	vga::bright_blue,
	vga::bright_magenta,
	vga::bright_cyan,
	vga::bright_white,

#define RGB(R, G, B) Color::fromRGB4(R * 6, G * 6, B * 6)
#define RGx(R, G)	 RGB(R, G, 0), RGB(R, G, 1), RGB(R, G, 2), RGB(R, G, 3), RGB(R, G, 4), RGB(R, G, 5)
#define Rxx(R)		 RGx(R, 0), RGx(R, 1), RGx(R, 2), RGx(R, 3), RGx(R, 4), RGx(R, 5)

	Rxx(0),
	Rxx(1),
	Rxx(2),
	Rxx(3),
	Rxx(4),
	Rxx(5),

#undef RGB
#undef RGx
#undef Rxx

#define W(N) Color::fromRGB8(N * 255 / 26, N * 255 / 26, N * 255 / 26)

	W(1),  W(2),  W(3),  W(4),  W(5),  W(6),
	W(7),  W(8),  W(9),  W(10), W(11), W(12),
	W(13), W(14), W(15), W(16), W(17), W(18),
	W(19), W(20), W(21), W(22), W(23), W(24),

#undef W
};

constexpr const Color* const default_colors[5]
{
	default_i1_colors,
	default_i2_colors,
	default_i4_colors,
	default_i8_colors,
	nullptr
};

// clang-format on

void ColorMap<colordepth_1bpp>::reset(const Color* q) noexcept { memcpy(colors, q, sizeof(Color) * 2); }
void ColorMap<colordepth_2bpp>::reset(const Color* q) noexcept { memcpy(colors, q, sizeof(Color) * 4); }
void ColorMap<colordepth_4bpp>::reset(const Color* q) noexcept { memcpy(colors, q, sizeof(Color) * 16); }
void ColorMap<colordepth_8bpp>::reset(const Color* q) noexcept { memcpy(colors, q, sizeof(Color) * 256); }

ColorMap<colordepth_1bpp>::ColorMap(const Color* src) noexcept
{
	if (src) reset(src); //
}

ColorMap<colordepth_2bpp>::ColorMap(const Color* src) noexcept : ColorMap<colordepth_1bpp>(nullptr)
{
	if (src) reset(src); //
}

ColorMap<colordepth_4bpp>::ColorMap(const Color* src) noexcept : ColorMap<colordepth_2bpp>(nullptr)
{
	if (src) reset(src); //
}

ColorMap<colordepth_8bpp>::ColorMap(const Color* src) noexcept : ColorMap<colordepth_4bpp>(nullptr)
{
	if (src) reset(src); //
}


// The system_colormap is intended to be used by the frame buffer:
ColorMap<colordepth_8bpp> XRAM system_colormap(vga8_colors);


static struct OnInit
{
	OnInit() { system_colormap.rc = 1; } // must never be `delete`ed
} _;

} // namespace kio::Graphics


/*












































*/
