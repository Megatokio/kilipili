// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaTiming.h"


namespace kio::Video
{

// clang-format off

struct VgaMode
{
	const VgaTiming* timing;

	uint16 width;
	uint16 height;
	uint16 yscale;  // 1 == normal, 2 == double height
};


constexpr VgaMode vga_mode_320x240_60 = {
	.timing = &vga_timing_320x480_60,
	.width	= 320,
	.height = 240,
	.yscale = 2,
};

constexpr VgaMode vga_mode_400x300_60 = {
	.timing = &vga_timing_400x600_60,
	.width	= 400,
	.height = 300,
	.yscale = 2,
};

constexpr VgaMode vga_mode_512x384_60 = {
	.timing = &vga_timing_512x768_60,
	.width	= 512,
	.height = 384,
	.yscale = 2,
};

constexpr VgaMode vga_mode_640x480_60 = {
	.timing = &vga_timing_640x480_60,
	.width	= 640,
	.height = 480,
	.yscale = 1,
};

constexpr VgaMode vga_mode_800x600_60 = {
	.timing = &vga_timing_800x600_60,
	.width	= 800,
	.height = 600,
	.yscale = 1,
};

constexpr VgaMode vga_mode_1024x768_60 = {
	.timing = &vga_timing_1024x768_60,
	.width	= 1024,
	.height = 768,
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

// clang-format on


// use these if you want type or range checking on the index:

constexpr const VgaMode* getVgaMode(ScreenSize ss)
{
	assert(ss <= num_screensizes);
	return vga_mode[ss];
}

template<ScreenSize SS>
constexpr const VgaMode* getVgaMode()
{
	return vga_mode[SS];
}

} // namespace kio::Video
