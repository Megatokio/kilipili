// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "VgaMode.h"
#include "VideoPlane.h"
#include "common/timing.h"
#include "internal/VideoBackend.h"
#include <functional>


namespace kilipili
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

inline int screen_width() noexcept { return vga_mode.width; }
inline int screen_height() noexcept { return vga_mode.height; }

/*	get currently displayed line number.
	can be less than 0 (-1 or -2) immediately before frame start.
	is greater or equal to `vga_mode.height` in vblank after the active display area.
*/
extern int current_scanline() noexcept;

extern void waitForVBlank() noexcept;
extern void waitForScanline(int scanline) noexcept;

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
extern void addVBlankOneTimeAction(const OneTimeAction& fu) noexcept;

/*	test whether video output is running.
*/
extern bool isVideoRunning() noexcept;

/*	test whether video plane is already added to the display stack.
	a plane added recently with wait=false returns false until it is added during vblank.
	a plane added while video is off doesn't show up until video is started.
*/
extern bool isVideoPlaneAdded(const VideoPlane*) noexcept;


extern uint			 scanlines_missed;
extern volatile bool locked_out;

} // namespace kilipili


/*






























*/
