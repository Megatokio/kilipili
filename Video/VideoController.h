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

/** the Video Frontend:
  
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
		INVALID,
		STOPPED,
		RUNNING,
	};

	/**	get reference to singleton 
		panics on first call if it can't claim the required hardware
	*/
	static VideoController& getRef() noexcept;

	/** setup internal state, buffers and hardware for the requested VideoMode.
		The supplied VideoMode struct must be static.
		If blocking = false then caller must later test `getState()` and `core1_error`.
	*/
	Error setup(const VgaMode&, bool blocking = true);

	void teardown(bool blocking = true) noexcept;
	void startVideo(int log2_scanline_buffer_size = 2, bool blocking = true);
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

	volatile State state		   = INVALID;
	volatile State requested_state = INVALID;

	uint8 _padding;


	VideoController() noexcept;

	static void start_core1() noexcept { getRef().core1_runner(); }
	void		core1_runner() noexcept;
	void		video_runner();

	void do_start_video();
	void do_stop_video();
	void do_setup() noexcept;
	void do_teardown() noexcept;
	void wait_for_event() noexcept;
	void call_vblank_actions() noexcept;
};


extern uint scanlines_missed;

} // namespace kio::Video


/*






























*/
