// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "cdefs.h"
#include "utilities.h"

namespace kio::Video
{

extern VgaMode vga_mode; // VGAMode in use

extern uint32		   cc_per_scanline;		//
extern uint32		   cc_per_frame;		//
extern uint			   cc_per_px;			// cpu clock cycles per pixel octet
extern uint			   cc_per_us;			// cpu clock cycles per microsecond
extern volatile bool   in_vblank;			// set while in vblank (set and reset ~2 scanlines early)
extern volatile int	   line_at_frame_start; // rolling line number at start of current frame
extern volatile uint32 time_us_at_frame_start;
extern volatile int	   current_frame;

/** get currently displayed line number.
	can be less than 0 (-1 or -2) immediately before frame start.
	is greater or equal to `vga_mode.height` in vblank after the active display area.
*/
inline int current_scanline() noexcept;


/** class VideoBackend implements the hardware part of the video controller.

	Settings in boards/yourboard.h:
	required:
	- define how many gpio pins are used for each color:
		#define VIDEO_COLOR_PIN_BASE  2
		#define VIDEO_COLOR_PIN_COUNT 12
		#define VIDEO_PIXEL_RSHIFT    0u
		#define VIDEO_PIXEL_GSHIFT    4u
		#define VIDEO_PIXEL_BSHIFT    8u
		#define VIDEO_PIXEL_RCOUNT    4
		#define VIDEO_PIXEL_GCOUNT    4
		#define VIDEO_PIXEL_BCOUNT    4
	- define where the hsync and vsync pins are:	
		#define VIDEO_SYNC_PIN_BASE   14		
	optional:	
	- define which clock signals are required and where they are:
		#define VIDEO_CLOCK_ENABLE   false
		#define VIDEO_DEN_ENABLE     false
		#define VIDEO_CLOCK_POLARITY 0
		#define VIDEO_DEN_POLARITY   0
		#define VIDEO_CLOCK_PIN_BASE 0
		
	Possible options in CMakeLists.txt:
	- VIDEO_OPTIMISTICAL_A1W8_RGB = ON|OFF 
		The scanline render function for colormode `a1w8_rgb` (truecolor attributes 
		with 1 bit / pixel in the bitmap and 8 pixel wide attributes) can use a variant
		which optimizes based on the screen contents. This allows highest video resolution
		up to 1280*768 to be displayed in this mode. But if the screen contents becomes too 
		colorful, then the video display will fail. (rolling screen contents displayed as 
		the pixel supply can't catch up.) Because the test, whether a pixel octet can be 
		optimized or not takes time, the overall time for the bad cases takes even longer 
		than non-optimized. Specifically 1024*768 is also not guaranteed to display colorful 
		contents properly, while 1024*768 can *just* be displayed without optimization 
		regardless of the screen contents.  
	- VIDEO_RECOVERY_PER_LINE = ON|OFF
		This enables a test for scanline buffer underflow in each scanline. The advantage is,
		that the video display recovers fast, just displaying single wrong lines. The 
		disadvantage is, that this test takes time and makes even more scanlines fail. 
		Specifically with this option enabled, screen resolution 1024*768 without 
		VIDEO_OPTIMISTICAL_A1W8_RGB misses some lines per frame while with this option disabled 
		it *just* works.		

	General capabilities and drawbacks:
	  - The supply of pixel data is not part of the backend. The backend displays pixels from a 
	    rolling scanline buffer which is filled in by the video frontend. this makes the backend 
	    very stable and less likely to crash. 
	  - In most cases a scanline buffer with only 2 lines is enough.
	  - The video backend can produce a VGA video signal for almost any screen resolution. 
	    Limitations generally come from available RAM and CPU processing power for the pixel supply,
	    which is not part of the video backend.
	  - It uses 3 DMA channels and 2 state machines in PIO1 and a shared interrupt on DMA_IRQ_1.
	  - The pixel clock must be a multiple of the system clock. (min. times 2)
	  - The system clock must be a multiple of 1 MHz.
	  - Currently there is no event/interrupt per scanline which makes waiting with wfe() impossible. 
	
	Interesting implementation details:
	  -	The sync signals are emitted by a single DMA which is reprogrammed 4 times per frame.
	  -	The screenlines are emitted by a nested DMA which is started once and never touched and 
		synchronized again. The DMA displays pixels from a rolling (circular) scanline buffer.
	  -	There is no interrupt per scanline: elimination of this interrupt was required to make 
		screen resolution 1024*768 work. Together with the fact, that the pixels are read from
		a curcular buffer, there is no way to tell where the display currently is other than 
		very precise counting. This is the reason why the system clock must be a multiple of 1 MHz.
	  	The video frontend must know which row is currently actually displayed in order to
		synchronize it filling the scanline buffer, not lagging behind and not overwriting pixels 
		before they are displayed.
*/
class VideoBackend
{
public:
	/**
		Initialize the hardware and claim the DMA channels and state machines	  
	*/
	static void initialize() noexcept; // panics

	/**
		Start video display in the requested resolution found in VgaMode.
		Pixels will be display from the cyclic scanline_buffer, 
		starting at scanline_buffer[0] for the first scanline in the first frame.
	*/
	static void start(const VgaMode&, uint32 min_sys_clock = 0) throws;

	/**
		Stop video display, releasing all resources.
		Actually does not stop video output but outputs a black screen.
		The scanline_buffer can now be purged and initialized for the new video mode to come.
	*/
	static void stop() noexcept;
};


inline void waitForVBlank() noexcept
{
	while (!in_vblank) { wfe(); }
}

inline void waitForScanline(int scanline) noexcept
{
	// TODO: we no longer get events for every scanline!
	//		 we could setup a timer
	if (uint(scanline) >= uint(vga_mode.height)) return waitForVBlank();
	idle_start();
	while (current_scanline() - scanline < 0) {} // wfe();
	idle_end();
}

inline int current_scanline() noexcept
{
	uint time_us_in_frame = time_us_32() - time_us_at_frame_start;
	uint cc_in_frame	  = time_us_in_frame * cc_per_us;
	return int(cc_in_frame) / int(cc_per_scanline);
}

inline int screen_width() noexcept { return vga_mode.width; }
inline int screen_height() noexcept { return vga_mode.height; }


} // namespace kio::Video
