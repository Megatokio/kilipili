/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Scanline.h"
#include "VBlankAction.h"
#include "VgaMode.h"
#include "VideoPlane.h"
#include "geometry.h"
#include <pico/sem.h>
#include <pico/types.h>


namespace kio::Video
{

/*
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
	using idle_fu = void();
	using coord	  = Graphics::coord;
	using Size	  = Graphics::Size;

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
	Error setup(const VgaMode*, bool blocking = true);

	void teardown(bool blocking = true) noexcept;
	void startVideo(bool blocking = true);
	void stopVideo(bool blocking = true);
	void addPlane(VideoPlane*);
	void removePlane(VideoPlane*);
	void addVBlankAction(VBlankAction* fu, uint8 when) noexcept;
	void removeVBlankAction(VBlankAction*);
	void setIdleAction(idle_fu* fu) noexcept { idle_action = fu; }
	void addOneTimeAction(std::function<void()> fu) noexcept;

	static bool in_hblank() noexcept;
	static void waitForVBlank() noexcept;
	static void waitForScanline(ScanlineID n) noexcept;

	static coord width() noexcept { return size.width; }
	static coord height() noexcept { return size.height; }

	State getState() const noexcept { return state; }

	static constexpr uint max_vblank_actions  = 8;
	static constexpr uint max_onetime_actions = 4;
	static constexpr uint max_planes		  = 4;


public:
	Error core1_error = NO_ERROR;

private:
	static Size size;

	volatile State state		   = INVALID;
	volatile State requested_state = INVALID;

	const VgaMode*		  vga_mode			 = nullptr;
	idle_fu*			  idle_action		 = nullptr;
	uint				  num_planes		 = 0;
	VideoPlane*			  planes[max_planes] = {nullptr};
	uint				  num_vblank_actions = 0;
	VBlankAction*		  vblank_actions[max_vblank_actions];
	uint8				  vblank_when[max_vblank_actions];
	uint				  num_onetime_actions = 0;
	std::function<void()> onetime_actions[max_onetime_actions]; // TODO this could be a queue


	VideoController() noexcept;

	static void start_core1() noexcept { getRef().core1_runner(); }
	void		core1_runner() noexcept;
	void		video_runner();

	void  start_video();
	void  stop_video();
	Error do_setup() noexcept;
	void  do_teardown() noexcept;
};


extern std::atomic<uint32> scanlines_missed;


} // namespace kio::Video
