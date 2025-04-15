// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "VgaMode.h"
#include "VideoPlane.h"
#include "timing.h"
#include <functional>
#include <pico/sem.h>
#include <pico/types.h>


namespace kio::Video
{

/*
	The Video Frontend:
  
	use:

	getRef()
	loop:
		setup()
		loop:
			addVideoPlane(), ...		
			addVBlankAction(), ...
			setIdleAction()
			startVideo()
			...
			stopVideo()
			removeVBlankAction(), ...   (also done by stopVideo)
			removePlane(), ...			(also done by stopVideo)
*/


using IdleAction	= std::function<void()>;
using VBlankAction	= std::function<void()>;
using OneTimeAction = std::function<void()>;

// in VideoBackend.cpp:
extern VgaMode		   vga_mode;			// VGAMode in use
extern uint32		   cc_per_scanline;		// cc per logical scanline (scaled by vss)
extern uint32		   cc_per_frame;		//
extern uint			   cc_per_px;			// cpu clock cycles per pixel
extern uint			   cc_per_us;			// cpu clock cycles per microsecond
extern volatile bool   in_vblank;			// set while in vblank (set and reset ~2 scanlines early)
extern volatile int	   line_at_frame_start; // rolling line number at start of current frame
extern volatile uint32 time_us_at_frame_start;
extern volatile uint32 time_cc_at_frame_start;
extern volatile int	   current_frame;

inline int screen_width() noexcept { return vga_mode.width; }
inline int screen_height() noexcept { return vga_mode.height; }

inline void waitForVBlank() noexcept
{
	while (!in_vblank) { wfe(); }
}

/*	get currently displayed line number.
	can be less than 0 (-1 or -2) immediately before frame start.
	is greater or equal to `vga_mode.height` in vblank after the active display area.
*/
inline int current_scanline() noexcept
{
	uint time_us_in_frame = time_us_32() - time_us_at_frame_start;
	uint cc_in_frame	  = time_us_in_frame * cc_per_us;
	return int(cc_in_frame) / int(cc_per_scanline);
}

inline void waitForScanline(int scanline) noexcept
{
	if (uint(scanline) >= uint(vga_mode.height)) return waitForVBlank();
	idle_start();
	while (current_scanline() - scanline < 0) {} // wfe();
	idle_end();
}

/* 	setup internal state, buffers and hardware for the requested VideoMode.
	blocks until backend has started.
*/
extern void startVideo(const VgaMode& = vga_mode_640x480_60, uint32 system_clock = 0, uint scanline_buffer_count = 2);

/*	stop video. 
	note: video resumes with black screen.
	disposes off all planes and registered actions.
	deallocates buffers.
	blocks until backend has stopped.
*/
extern void stopVideo() noexcept;

/*	add a plane to the video output.
	the plane will be added by core1 on the next vblank.
	can be called before startVideo() and any time afterwards.
*/
extern void addVideoPlane(VideoPlanePtr, bool wait = false);

/*	remove a plane from the video output.
	the plane will be removed by core1 on the next vblank.
	note: stopVideo() also disposes off all planes.
*/
extern void removeVideoPlane(VideoPlanePtr, bool wait = false);

/*	register a function to be called during every vblank.
	the video controller calls onetime actions, the vblank action and 
	plane.vblank of all planes during vblank in this order.		
*/
extern void setVBlankAction(const VBlankAction& fu) noexcept;

/*	register a function to be called during next vblank.
	multiple onetime actions can be registered in the same frame. 
*/
extern void addOneTimeAction(const OneTimeAction& fu) noexcept;

/*	test whether video output is running.
*/
extern bool isVideoRunning() noexcept;


extern uint			 scanlines_missed;
extern volatile bool locked_out;

} // namespace kio::Video


/*






























*/
