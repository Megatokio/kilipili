// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/geometry.h"
#include "common/standard_types.h"

namespace kio::Video
{

// clang-format off

struct VgaMode
{
	uint32 pixel_clock;

	uint8 h_front_porch;
	uint8 h_pulse;
	uint8 h_back_porch;
	bool  h_sync_polarity;

	uint8 v_front_porch;
	uint8 v_pulse;
	uint8 v_back_porch;
	bool  v_sync_polarity;

	int   vss;    // height << vss = v_active 
	int   width;  
	int   height;
	
	constexpr uint h_active() const noexcept { return uint(width); }
	constexpr uint h_total() const { return h_front_porch + h_pulse + h_back_porch + h_active(); }
	constexpr uint v_active() const noexcept { return uint(height) << vss; }
	constexpr uint v_total() const { return v_front_porch + v_pulse + v_back_porch + v_active(); }
	constexpr Size  size() const noexcept { return Size(width,height); }
	constexpr Point center() const noexcept { return Point(width/2,height/2); }

	constexpr VgaMode half() const noexcept 
	{
		return VgaMode{
			.pixel_clock     = pixel_clock >> 1,		
			.h_front_porch   = uint8(h_front_porch >> 1), 
			.h_pulse         = uint8(h_pulse >> 1), 
			.h_back_porch    = uint8(h_back_porch >> 1), 
			.h_sync_polarity = h_sync_polarity,		
			.v_front_porch   = v_front_porch,  
			.v_pulse         = v_pulse,	 
			.v_back_porch    = v_back_porch, 
			.v_sync_polarity = v_sync_polarity,		
			.vss             = vss + 1,
			.width	         = width >> 1,
			.height          = height >> 1,
		};
	}
};


// VGA TIMING
// no 2 sources use the same timing ...


constexpr VgaMode vga_mode_640x480_60 = 
{
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                     vert				  polarity
	// ----- --------  ---------  --------  ---------------------   --------------------  -------------
	// VESA  25.175    31.46875   59.94     640 +16 +96 +48 = 800   480 +10 +2 +33 = 525  -hsync -vsync
	// kio   25.175    31.46875   59.94     640 +16 +96 +48 = 800   480  +3 +2 +40 = 525  -hsync -vsync

	// note: with the VESA vertical timing the image starts 7 lines early (top 7 lines are cut off)

	.pixel_clock = 25000000,

	.h_front_porch = 16, 
	.h_pulse = 96, 
	.h_back_porch = 48, 
	.h_sync_polarity = 0,

	.v_front_porch = 3,  
	.v_pulse = 2,	 
	.v_back_porch = 40, 
	.v_sync_polarity = 0,

	.vss = 0,
	.width	= 640,
	.height = 480,
};

static_assert(vga_mode_640x480_60.h_total() == 800);
static_assert(vga_mode_640x480_60.v_total() == 525);

constexpr VgaMode vga_mode_320x240_60 = vga_mode_640x480_60.half();
constexpr VgaMode vga_mode_160x120_60 = vga_mode_320x240_60.half();


constexpr VgaMode vga_mode_800x600_60 = 
{
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                      vert				   polarity
	// ----- --------  ---------  --------  -----------------------  --------------------  -------------
	// VESA  40.00     37.8787    60.324    800 +40 +128 +88 = 1056  600 +1 +4 +23 = 628   +hsync +vsync

	.pixel_clock = 40000000,

	.h_front_porch = 40, 
	.h_pulse = 128, 
	.h_back_porch = 88, 
	.h_sync_polarity = 1,

	.v_front_porch = 1,  
	.v_pulse = 4,	  
	.v_back_porch = 23, 
	.v_sync_polarity = 1,

	.vss = 0,
	.width	= 800,
	.height = 600,
};

static_assert(vga_mode_800x600_60.h_total() == 1056);
static_assert(vga_mode_800x600_60.v_total() == 628);

constexpr VgaMode vga_mode_400x300_60 = vga_mode_800x600_60.half();
constexpr VgaMode vga_mode_200x150_60 = vga_mode_400x300_60.half();


constexpr VgaMode vga_mode_1024x768_60 = 
{
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                        vert                 polarity
	// ----- --------  ---------  --------  ------------------------   -------------------- -------------
	// VESA  65.00     48.363     60.00384  1024 +24 +136 +160 = 1344  768 +3 +6 +29 = 806  -hsync -vsync

	// note: cvt 1024 768 60
	//		 # 1024x768 59.92 Hz (CVT 0.79M3) hsync: 47.82 kHz; pclk: 63.50 MHz
	//		 Modeline "1024x768_60.00"   63.50  1024 1072 1176 1328  768 771 775 798 -hsync +vsync

	.pixel_clock = 65000000,

	.h_front_porch = 24, 
	.h_pulse = 136, 
	.h_back_porch = 160, 
	.h_sync_polarity = 0,

	.v_front_porch = 3,  
	.v_pulse = 6,	  
	.v_back_porch = 29,  
	.v_sync_polarity = 0,

	.vss = 0,
	.width	= 1024,
	.height = 768,
};

static_assert(vga_mode_1024x768_60.h_total() == 1344);
static_assert(vga_mode_1024x768_60.v_total() == 806);

constexpr VgaMode vga_mode_512x384_60 = vga_mode_1024x768_60.half();
constexpr VgaMode vga_mode_256x192_60 = vga_mode_512x384_60.half();


constexpr VgaMode vga_mode_1280x768_60 = 
{
	// this is VESA mode 1366*768@60Hz REDUCED BLANKING 	
	// but we use only 1280 = 40*32 pixels 	
	// successfully displays colormode `a1w8` with option VIDEO_OPTIMISTIC_A1W8 enabled
		
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                       vert                 polarity
	// ----- --------- ---------- --------- ------------------------ --------------------- -------------
	// VESA  72.00     48.000     60.000    1366 +14 +56 +64 = 1500   768 +1 +3 +28 = 800  +hsync +vsync
	//       72.00     48.000     60.000    1280 +56 +56 +108 = 1500  768 +1 +3 +28 = 800  +hsync +vsync

	.pixel_clock = 72000000,

	.h_front_porch = 56, 
	.h_pulse = 56, 
	.h_back_porch = 108, 
	.h_sync_polarity = 1,

	.v_front_porch = 1,  
	.v_pulse = 3,	  
	.v_back_porch = 28,  
	.v_sync_polarity = 1,

	.vss = 0,
	.width	= 1280,
	.height = 768,
};

constexpr VgaMode vga_mode_640x384_60 = vga_mode_1280x768_60.half();
constexpr VgaMode vga_mode_320x192_60 = vga_mode_640x384_60.half();


constexpr VgaMode vga_mode_1360x768_60 = 
{
	// this is VESA mode 1366*768@60Hz REDUCED BLANKING 	
	// but we use only 1360 = int(1366/16)*16
	// this will probably never display colormode `a1w8_rgb` ...
	
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                      vert                 polarity
	// ----- --------- ---------- --------- ------------------------ -------------------- -------------
	// VESA  72.00     48.000     60.000    1366 +14 +56 +64 = 1500  768 +1 +3 +28 = 800  +hsync +vsync
	//       72.00     48.000     60.000    1360 +18 +56 +66 = 1500  768 +1 +3 +28 = 800  +hsync +vsync

	.pixel_clock = 72000000,

	.h_front_porch = 18, 
	.h_pulse = 56, 
	.h_back_porch = 66, 
	.h_sync_polarity = 1,

	.v_front_porch = 1,  
	.v_pulse = 3,	  
	.v_back_porch = 28,  
	.v_sync_polarity = 1,

	.vss = 0,
	.width	= 1360,
	.height = 768,
};


constexpr VgaMode vga_mode_672x384_60_v1 = 
{
	// this is VESA mode 1366*768@60Hz REDUCED BLANKING 	
	// with black padding l+r.	
	// my monitor recognizes this as 1280x768 and masks 16 pixels l+r. :-(

	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                       vert                 polarity
	// ----- --------- ---------- --------- ------------------------ --------------------- -------------
	// VESA  72.00     48.000     60.000    1366 +14 +56 +64 = 1500  768 +1 +3 +28 = 800   +hsync +vsync
	// half  36.00                          683 +7 +28 +32 = 750     768 +1 +3 +28 = 800   +hsync +vsync
	//       36.00                          672 +12 +28 +38 =750
	
	.pixel_clock = 72000000/2,

	.h_front_porch = 12, 
	.h_pulse = 28, 
	.h_back_porch = 38, 
	.h_sync_polarity = 1,

	.v_front_porch = 1,  
	.v_pulse = 3,	  
	.v_back_porch = 28,  
	.v_sync_polarity = 1,

	.vss = 1,
	.width	= 672,
	.height = 384,
};


constexpr VgaMode vga_mode_672x384_60_v2 = 
{
	// this is VESA mode 1366*768@60Hz NORMAL BLANKING 	
	// but we use only 1366/2/16*16 = 672 px = 84 char

	// again, my monitor thinks this is 1280x768.
	// the placement of the image on my monitor is poor.
	// only clock 2*85.5 MHz possible to achieve full Mhz.
	
	// SRC   pclk:MHz  hsync:kHz  vsync:Hz  hor                      vert                 polarity
	// ----- --------- ---------- --------- ------------------------ -------------------- -------------
	// VESA  85.50     47.712     60.000    1366 +70 +143 +213 = 1792  768 +3 +3 +24 = 798  +hsync +vsync
	//       85.50     47.712     60.000    672  +40 +72  +112 = 896   768 +3 +3 +24 = 798  +hsync +vsync

	.pixel_clock = 85500000/2,

	.h_front_porch = 80/2, 
	.h_pulse = 144/2, 
	.h_back_porch = 224/2, 
	.h_sync_polarity = 1,

	.v_front_porch = 3,  
	.v_pulse = 3,	  
	.v_back_porch = 24,  
	.v_sync_polarity = 1,

	.vss = 1,
	.width	= 672,
	.height = 384,
};


// #####################################################################
//			50 Hz variants
//			no vesa standard, may work with some monitors
// #####################################################################


constexpr VgaMode vga_mode_640x480_50 = 
{
	// this works on my TV set:

	.pixel_clock = 22000000,

	.h_front_porch = 16, 
	.h_pulse = 64,
	.h_back_porch	 = 80, // 80+64+16+640=800
	.h_sync_polarity = 0,

	.v_front_porch = 16, 
	.v_pulse = 2,
	.v_back_porch	 = 52, // 52+480+16+2=550
	.v_sync_polarity = 0,

	.vss = 0,
	.width	= 640,
	.height = 480,
};

constexpr VgaMode vga_mode_320x240_50 = vga_mode_640x480_50.half();

static_assert(vga_mode_640x480_50.h_total() == 800);
static_assert(vga_mode_640x480_50.v_total() == 550);


constexpr VgaMode vga_mode_1024x768_50 = 
{
	// note: cvt 1024 768 50
	//		 # 1024x768 49.98 Hz (CVT 0.79M3) hsync: 39.63 kHz; pclk: 52.00 MHz
	//		 Modeline "1024x768_50.00"   52.00  1024 1072 1168 1312  768 771 775 793 -hsync +vsync

	// 54MHz or 57MHz: Multiplier=5
	// detected as 1280x768, horizontally not locked to real monitor pixels
	// the image is ~ 16 pixel too narrow, ~10 left + ~6 right side. so it's neither 1280 nor 1024. weird.
	// right side of the sync pulse seemingly doesn't matter for my TV

	.pixel_clock = 54000000, 
	.h_front_porch = 32,
	.h_pulse	  = 160, 
	.h_back_porch = 160 - 8, 
	.h_sync_polarity = 0,

	.v_front_porch = 3,   
	.v_pulse = 6,		
	.v_back_porch = 29, 
	.v_sync_polarity = 0,

	.vss = 0,
	.width	= 1024,
	.height = 768,
};

constexpr VgaMode vga_mode_512x384_50 = vga_mode_1024x768_50.half();

static_assert(vga_mode_1024x768_50.h_total() == 1368);
static_assert(vga_mode_1024x768_50.v_total() == 806);

} // namespace kio::Video


// clang-format on
