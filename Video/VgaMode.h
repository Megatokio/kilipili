// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaTiming.h"


namespace kio::Video
{

struct VgaMode
{
	const VgaTiming* default_timing;

	uint16 width;
	uint16 height;
	uint16 xscale;  // 1 == normal, 2 == double wide etc. up to what pio timing allows (not sure I have an assert based on delays)
	uint16 yscale;  // same for y scale (except any yscale is possible)
};


constexpr VgaMode vga_mode_320x240_60 =
{
	.default_timing = &vga_timing_640x480_60,
	.width  = 320,
	.height = 240,
	.xscale = 2,
	.yscale = 2,
};

constexpr VgaMode vga_mode_400x300_60 =
{
	.default_timing = &vga_timing_800x600_60,
	.width  = 400,
	.height = 300,
	.xscale = 2,
	.yscale = 2,
};

constexpr VgaMode vga_mode_512x384_60 =
{
	.default_timing = &vga_timing_1024x768_60,
	.width  = 512,
	.height = 384,
	.xscale = 2,
	.yscale = 2,
};

constexpr VgaMode vga_mode_640x480_60 =
{
	.default_timing = &vga_timing_640x480_60,
	.width  = 640,
	.height = 480,
	.xscale = 1,
	.yscale = 1,
};

constexpr VgaMode vga_mode_800x600_60 =
{
	.default_timing = &vga_timing_800x600_60,
	.width  = 800,
	.height = 600,
	.xscale = 1,
	.yscale = 1,
};

constexpr VgaMode vga_mode_1024x768_60 =
{
	.default_timing = &vga_timing_1024x768_50,
	.width  = 1024,
	.height = 768,
	.xscale = 1,
	.yscale = 1,
};

constexpr const VgaMode* vga_mode[num_screensizes] =
{
	&vga_mode_320x240_60,
	&vga_mode_400x300_60,
	&vga_mode_512x384_60,
	&vga_mode_640x480_60,
	&vga_mode_800x600_60,
	&vga_mode_1024x768_60
};


// use these if you want type or range checking on the index:

constexpr const VgaMode* getVgaMode(ScreenSize ss)
{
	assert(ss <= num_screensizes);
	return vga_mode[ss];
}

template<ScreenSize SS> constexpr const VgaMode* getVgaMode()
{
	return vga_mode[SS];
}

} // namespace














