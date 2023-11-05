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
	using coord			= Graphics::coord;
	using Size			= Graphics::Size;

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
		If blocking = false then caller must later test `getState()` and `core1_error`.
	*/
	void startVideo(const VgaMode& mode, uint32 system_clock = 0, uint scanline_buffer_size = 2, bool blocking = true);
	void stopVideo(bool blocking = true);
	void addPlane(VideoPlane*);
	void removePlane(VideoPlane*);
	void setVBlankAction(const VBlankAction& fu) noexcept;
	void setIdleAction(const IdleAction& fu) noexcept;
	void addOneTimeAction(const OneTimeAction& fu) noexcept;

	State getState() const noexcept { return state; }

	static Error core1_error;

private:
	static constexpr uint max_planes = 4;

	uint		  num_planes;
	VideoPlane*	  planes[max_planes] = {nullptr};
	IdleAction	  idle_action		 = nullptr;
	VBlankAction  vblank_action		 = nullptr;
	OneTimeAction onetime_action	 = nullptr;

	volatile State state		   = STOPPED;
	volatile State requested_state = STOPPED;
	uint8		   _padding[2];
	uint32		   requested_system_clock;


	VideoController() noexcept;

	void core1_runner() noexcept;
	void video_runner();

	void wait_for_event() noexcept;
	void call_vblank_actions() noexcept;
};


extern uint scanlines_missed;

} // namespace kio::Video


/*






























*/
