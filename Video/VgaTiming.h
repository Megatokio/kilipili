// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "kilipili_common.h"


namespace kio::Video
{

enum ScreenSize : uint8 // screen size in pixels: width x height
{
	screensize_320x240	= 0,
	screensize_400x300	= 1,
	screensize_512x384	= 2,
	screensize_640x480	= 3,
	screensize_800x600	= 4,
	screensize_1024x768 = 5,
};

constexpr uint num_screensizes = screensize_1024x768 + 1;

inline constexpr ScreenSize operator+(ScreenSize a, int b) noexcept { return ScreenSize(int(a) + b); }


struct VgaTiming
{
	uint32 pixel_clock;

	uint16 h_active;
	uint16 h_front_porch;
	uint16 h_pulse;
	uint16 h_back_porch;
	bool   h_sync_polarity, _padding = 0;

	uint16 v_active;
	uint16 v_front_porch;
	uint16 v_pulse;
	uint16 v_back_porch;
	bool   v_sync_polarity, _padding2 = 0;

	constexpr uint16 h_total() const { return h_front_porch + h_pulse + h_back_porch + h_active; }
	constexpr uint16 v_total() const { return v_front_porch + v_pulse + v_back_porch + v_active; }
};


// clang-format off

// VGA TIMING
// no 2 sources use the same timing ...

constexpr VgaTiming vga_timing_640x480_60 = {
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                     vert				  polarity
	// ----- --------  ---------  --------  ---------------------   --------------------  -------------
	// VESA  25.175    31.46875   59.94     640 +16 +96 +48 = 800   480 +10 +2 +33 = 525  -hsync -vsync
	// kio   25.175    31.46875   59.94     640 +16 +96 +48 = 800   480  +3 +2 +40 = 525  -hsync -vsync

	// note: with the VESA vertical timing the image starts 7 lines early (top 7 lines are cut off)

	// note: cvt 640 480 60
	//		 # 640x480 59.38 Hz (CVT 0.31M3) hsync: 29.69 kHz; pclk: 23.75 MHz
	//		 Modeline "640x480_60.00"   23.75  640 664 720 800  480 483 487 500 -hsync +vsync

	.pixel_clock = 25000000,

	.h_active = 640,		 
	.h_front_porch = 16, 
	.h_pulse = 96, 
	.h_back_porch = 48, 
	.h_sync_polarity = 0,

	.v_active = 480,		 
	.v_front_porch = 3,  
	.v_pulse = 2,	 
	.v_back_porch = 40, 
	.v_sync_polarity = 0,
};

static_assert(vga_timing_640x480_60.h_total() == 800);
static_assert(vga_timing_640x480_60.v_total() == 525);

constexpr VgaTiming vga_timing_640x480_50 = {
	// this works on my TV set:

	.pixel_clock = 22000000,

	.h_active = 640,		 
	.h_front_porch = 16, 
	.h_pulse = 64,
	.h_back_porch	 = 80, // 80+64+16+640=800
	.h_sync_polarity = 0,

	.v_active = 480,		 
	.v_front_porch = 16, 
	.v_pulse = 2,
	.v_back_porch	 = 52, // 52+480+16+2=550
	.v_sync_polarity = 0,
};

static_assert(vga_timing_640x480_50.h_total() == 800);
static_assert(vga_timing_640x480_50.v_total() == 550);

constexpr VgaTiming vga_timing_800x600_60 = {
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                      vert				   polarity
	// ----- --------  ---------  --------  -----------------------  --------------------  -------------
	// VESA  40.00     37.8787    60.32     800 +40 +128 +88 = 1056  600 +1 +4 +23 = 628   +hsync +vsync

	.pixel_clock = 40000000,

	.h_active = 800,		 
	.h_front_porch = 40, 
	.h_pulse = 128, 
	.h_back_porch = 88, 
	.h_sync_polarity = 1,

	.v_active = 600,		 
	.v_front_porch = 1,  
	.v_pulse = 4,	  
	.v_back_porch = 23, 
	.v_sync_polarity = 1,
};

static_assert(vga_timing_800x600_60.h_total() == 1056);
static_assert(vga_timing_800x600_60.v_total() == 628);

constexpr VgaTiming vga_timing_1024x768_60 = {
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                        vert                 polarity
	// ----- --------  ---------  --------  ------------------------   -------------------- -------------
	// VESA  65.00     48.363     60.00384  1024 +24 +136 +160 = 1344  768 +3 +6 +29 = 806  -hsync -vsync

	// note: cvt 1024 768 60
	//		 # 1024x768 59.92 Hz (CVT 0.79M3) hsync: 47.82 kHz; pclk: 63.50 MHz
	//		 Modeline "1024x768_60.00"   63.50  1024 1072 1176 1328  768 771 775 798 -hsync +vsync

	.pixel_clock = 65000000,

	.h_active = 1024,		 
	.h_front_porch = 24, 
	.h_pulse = 136, 
	.h_back_porch = 160, 
	.h_sync_polarity = 0,

	.v_active = 768,		 
	.v_front_porch = 3,  
	.v_pulse = 6,	  
	.v_back_porch = 29,  
	.v_sync_polarity = 0,
};

static_assert(vga_timing_1024x768_60.h_total() == 1344);
static_assert(vga_timing_1024x768_60.v_total() == 806);

constexpr VgaTiming vga_timing_1024x768_50 = {
	// note: cvt 1024 768 50
	//		 # 1024x768 49.98 Hz (CVT 0.79M3) hsync: 39.63 kHz; pclk: 52.00 MHz
	//		 Modeline "1024x768_50.00"   52.00  1024 1072 1168 1312  768 771 775 793 -hsync +vsync

	// 54MHz or 57MHz: Multiplier=5 --> a1w8_rgb: 227 scanlines displayed, 541 missing.
	// detected: 1280x768, horizontally not locked to real monitor pixels

	//	.pixel_clock = 54000000,
	//
	//	.h_active      = 1024,
	//	.h_front_porch = 24,
	//	.h_pulse       = 136,
	//	.h_back_porch  = 160,
	//	.h_sync_polarity = 0,
	//
	//	.v_active      = 768,
	//	.v_front_porch = 3,
	//	.v_pulse       = 6,
	//	.v_back_porch  = 29,
	//	.v_sync_polarity = 0,

	// 54MHz or 57MHz: Multiplier=5
	// detected: 1280x768, horizontally not locked to real monitor pixels
	// the image is ~ 16 pixel too narrow, ~10 left + ~6 right side. so it's neither 1280 nor 1024. weird.
	// htotal=1376 => a1w8_rgb: clock=270MHz, avg=254.5MHz, max=267.8MHz
	// htotal=1368 => a1w8_rgb: clock=270MHz, avg=256.9MHz, max=270.0MHz  <-- the current absolute limit!

	.pixel_clock = 54000000, 
	.h_active = 1024,	   
	.h_front_porch = 32,
	.h_pulse	  = 160, // right side of the pulse seemingly doesn't matter for my TV
	.h_back_porch = 160 - 8, 
	.h_sync_polarity = 0,

	.v_active = 768,		 
	.v_front_porch = 3,   
	.v_pulse = 6,		
	.v_back_porch = 29, 
	.v_sync_polarity = 0,
};

// clang-format on

static_assert(vga_timing_1024x768_50.h_total() == 1368);
static_assert(vga_timing_1024x768_50.v_total() == 806);


constexpr const VgaTiming* vga_timing[num_screensizes] {
	&vga_timing_640x480_60, &vga_timing_800x600_60, &vga_timing_1024x768_60,
	&vga_timing_640x480_60, &vga_timing_800x600_60, &vga_timing_1024x768_60,
};


// use these if you want type or range checking on the index:

constexpr const VgaTiming* getVgaTiming(ScreenSize ss)
{
	assert(ss <= num_screensizes);
	return vga_timing[ss];
}

template<ScreenSize SS>
constexpr const VgaTiming* getVgaTiming()
{
	return vga_timing[SS];
}


} // namespace kio::Video

inline cstr tostr(kio::Video::ScreenSize ss)
{
	static constexpr char tbl[kio::Video::num_screensizes][9] = //
		{"320*240", "400*300", "512*384", "640*480", "800*400", "1024*768"};
	return tbl[ss];
}


/*

































*/
