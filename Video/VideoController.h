/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "VgaMode.h"
#include "VideoPlane.h"
#include "geometry.h"
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


class VideoController
{
public:
	using IdleAction	= std::function<void()>;
	using VBlankAction	= std::function<void()>;
	using OneTimeAction = std::function<void()>;

	enum State : uint8 {
		STOPPED,
		RUNNING,
	};

	/*	
		get reference to singleton 
		panics on first call if it can't claim the required hardware
	*/
	static VideoController& getRef() noexcept;

	/* 
		setup internal state, buffers and hardware for the requested VideoMode.
		blocks until backend has started.
	*/
	void startVideo(const VgaMode& = vga_mode_640x480_60, uint32 system_clock = 0, uint scanline_buffer_count = 2);

	/*
		stop video. 
		note: video resumes with black screen.
		disposes off all planes and registered actions.
		deallocates buffers.
		blocks until backend has stopped.
	*/
	void stopVideo() noexcept;

	/*
		add a plane to the video output.
		the plane will be added by core1 on the next vblank.
		can be called before startVideo() and any time afterwards.
	*/
	void addPlane(VideoPlanePtr);

	/*
		remove a plane from the video output.
		the plane will be removed by core1 on the next vblank.
		note: stopVideo() also disposes off all planes.
	*/
	void removePlane(VideoPlanePtr);

	/*
		register a function to be called during every vblank.
		the video controller calls onetime actions, the vblank action and 
		plane.vblank of all planes during vblank in this order.		
	*/
	void setVBlankAction(const VBlankAction& fu) noexcept;

	/*
		register a function to be called during next vblank.
		multiple onetime actions can be registered in the same frame. 
	*/
	void addOneTimeAction(const OneTimeAction& fu) noexcept;

	/*
		get state, which is either RUNNING or STOPPED.
	*/
	State getState() const noexcept { return state; }

private:
	static constexpr uint max_planes = 8;

	uint		  num_planes		 = 0;
	VideoPlanePtr planes[max_planes] = {nullptr};
	VBlankAction  vblank_action		 = nullptr;
	OneTimeAction onetime_action	 = nullptr;

	volatile State state		   = STOPPED;
	volatile State requested_state = STOPPED;
	uint32		   requested_system_clock;

	VideoController() noexcept;

	void core1_runner() noexcept;
	void video_runner(int row0, uint32 cc_at_line_start);
	void call_vblank_actions() noexcept;
	void vblank(VideoPlane* vp) noexcept;
	void render(VideoPlane* vp, int row, uint32* fb) noexcept;
};


extern uint scanlines_missed;

} // namespace kio::Video


/*






























*/
