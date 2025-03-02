// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "VgaMode.h"
#include "VideoPlane.h"
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
			addPlane(), ...		
			addVBlankAction(), ...
			setIdleAction()
			startVideo()
			...
			stopVideo()
			removeVBlankAction(), ...   (also done by teardown)
			removePlane(), ...			(also done by teardown)
		teardown()

	addPlane()    calls renderer.setup()    for all video_buffer.scanlines[].data[plane]
	removePlane() calls renderer.teardown() for all video_buffer.scanlines[].data[plane]
*/


using IdleAction	= std::function<void()>;
using VBlankAction	= std::function<void()>;
using OneTimeAction = std::function<void()>;


class VideoController
{
public:
	/* 	setup internal state, buffers and hardware for the requested VideoMode.
		blocks until backend has started.
	*/
	static void
	startVideo(const VgaMode& = vga_mode_640x480_60, uint32 system_clock = 0, uint scanline_buffer_count = 2);

	/*	stop video. 
		note: video resumes with black screen.
		disposes off all planes and registered actions.
		deallocates buffers.
		blocks until backend has stopped.
	*/
	static void stopVideo() noexcept;

	/*	add a plane to the video output.
		the plane will be added by core1 on the next vblank.
		can be called before startVideo() and any time afterwards.
	*/
	static void addPlane(VideoPlanePtr);

	/*	remove a plane from the video output.
		the plane will be removed by core1 on the next vblank.
		note: stopVideo() also disposes off all planes.
	*/
	static void removePlane(VideoPlanePtr);

	/*	register a function to be called during every vblank.
		the video controller calls onetime actions, the vblank action and 
		plane.vblank of all planes during vblank in this order.		
	*/
	static void setVBlankAction(const VBlankAction& fu) noexcept;

	/*	register a function to be called during next vblank.
		multiple onetime actions can be registered in the same frame. 
	*/
	static void addOneTimeAction(const OneTimeAction& fu) noexcept;

	/*	test whether video output is running.
	*/
	static bool isRunning() noexcept;
};

extern uint			 scanlines_missed;
extern volatile bool locked_out;

} // namespace kio::Video


/*






























*/
