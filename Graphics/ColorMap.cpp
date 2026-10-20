// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ColorMap.h"
#include <string.h>

#if defined GRAPHICS_SYSTEM_COLORMAP_SECTION
  #define CMRAM GRAPHICS_SYSTEM_COLORMAP_SECTION
#else
  #define CMRAM XRAM // core1 stack page
#endif


namespace kilipili
{

// clang-format off

constexpr Color default_i1_colors[2] =
{
	bright_green,
	Color::fromGrey8(15),
};

constexpr Color default_i2_colors[4] =
{
	black,
	dark_grey,
	light_grey,
	white,
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
	vga::light_grey,
	vga::dark_grey,
	vga::light_red,
	vga::light_green,
	vga::light_yellow,
	vga::light_blue,
	vga::light_magenta,
	vga::light_cyan,
	vga::white,
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
	vga::light_grey,
	vga::dark_grey,
	vga::light_red,
	vga::light_green,
	vga::light_yellow,
	vga::light_blue,
	vga::light_magenta,
	vga::light_cyan,
	vga::white,

#define RGB(R, G, B) Color::fromRGB8(R * 51, G * 51, B * 51)
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

// clang-format on


// The system_colormap is intended to be used by the frame buffer.
// rc = -1  => never deleted by RCPtr
ColorMap<colormode_i8> __section(CMRAM ".cm_data") system_colormap(vga8_colors, -1);

int ColorMap<>::bestMatch(Color c, int count) noexcept
{
	if (count == 0) count = this->count;
	assert(count > 0);

	// first round tries exact match:

	for (int i = 0; i < count; i++)
		if (c == colors[i]) return i; // todo: multiple hits

	// then erode away the lower bits:

	constexpr int maxbits = max(Color::rbits, Color::gbits, Color::bbits) + Color::ibits;
	static_assert(maxbits <= 8);

	for (uint8 mask = uint8(0xff << (8 - maxbits));;)
	{
		mask &= mask << 1;
		Color::uRGB rgbimask = Color::fromGrey8(mask);
		Color::uRGB cc		 = c & rgbimask;

		for (int i = 0; i < count; i++)
			if (cc == (colors[i] & rgbimask)) return i; // todo: multiple hits
	}
}


} // namespace kilipili


/*












































*/
