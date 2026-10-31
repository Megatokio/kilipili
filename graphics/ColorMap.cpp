// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ColorMap.h"
#include "common/basic_math.h"
#include <utility>


namespace kilipili
{

// clang-format off

constexpr Color default_i1_colors[2] =
{
	black,
	white,
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


static const Color* default_cmap(int count) noexcept
{
	assert(count >= 1);

	if (count > 8) return vga8_colors;
	constexpr const Color* c4[4] = {default_i1_colors, default_i1_colors, default_i2_colors, zx_colors};
	return c4[msbit(uint8(2 * count - 1))];
}

static void copy(Color* z, const Color* q, int cnt) noexcept
{
	for (int i = 0; i < cnt; i++) z[i] = q[i];
}
static void copy_up(Color* z, const Color* q, int cnt) noexcept
{
	for (int i = cnt; --i >= 0;) z[i] = q[i];
}

void ColorMap<0>::reset(const Color* src, int count) noexcept
{
	assert(count >= 0);

	if (count == 0) count = this->count;
	else count = min(count, this->count);

	if (src == nullptr) src = default_cmap(count);
	copy(this->colors, src, count);
}

ColorMap<0>::ColorMap(const Color* src, int bits) noexcept : //
	count(uint16(1 << bits))
{
	assert(bits >= 0 && bits <= 8);
	assert(count > 0 && count <= 256);

	if (src == nullptr) src = default_cmap(count);
	copy(this->colors, src, count);
}

uint16 ColorMap<0>::bestMatch(Color c, int count) noexcept
{
	if (count == 0) count = this->count;

	// first round tries exact match:

	for (uint16 i = 0; i < count; i++)
		if (c == colors[i]) return i;

	// then erode away the lower bits:

	constexpr int maxbits = max(Color::rbits, Color::gbits, Color::bbits) + Color::ibits;
	static_assert(maxbits <= 8);

	for (uint8 mask = uint8(0xff << (8 - maxbits));;)
	{
		mask &= mask << 1;
		Color::uRGB rgbimask = Color::fromGrey8(mask);
		Color::uRGB cc		 = c & rgbimask;

		for (uint16 i = 0; i < count; i++)
			if (cc == (colors[i] & rgbimask)) return i; // todo: multiple hits
	}
}

void ColorMap<0>::reverse() noexcept
{
	for (int i = 0; i < count / 2; i++) std::swap(colors[i], colors[count - 1 - i]);
}

void ColorMap<0>::complement() noexcept
{
	for (int i = 0; i < count; i++) colors[i].raw ^= Color::total_colormask;
}

void ColorMap<0>::lighter() noexcept
{
	for (int i = 0; i < count; i++) colors[i].blend_with(white);
}

void ColorMap<0>::darker() noexcept
{
	for (int i = 0; i < count; i++) colors[i].blend_with(black);
}

void ColorMap<0>::shuffle() noexcept
{
	for (int i = 0; i < count; i++) { std::swap(colors[i], colors[uint(random()) % count]); }
}

void ColorMap<0>::roll(int dist) noexcept
{
	assert(dist <= count && dist >= -count);

	if (dist > count / 2) dist -= count;
	if (dist < -count / 2) dist += count;

	Color z[128];
	bool  dn = dist < 0;
	dist	 = abs(dist);
	int cnt	 = count - dist;
	assert(dist <= 128);

	if (dn)
	{
		copy(z, colors, dist);
		copy(colors, colors + dist, cnt);
		copy(colors + cnt, z, dist);
	}
	else // up
	{
		copy(z, colors + cnt, dist);
		copy_up(colors + dist, colors, cnt);
		copy(colors, z, dist);
	}
}


} // namespace kilipili


/*












































*/
