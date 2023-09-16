/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include "Scanline.h"
#include "VgaMode.h"
#include "VgaTiming.h"
#include "Video/VBlankAction.h"
#include "Video/VideoPlane.h"
#include "VideoQueue.h"
#include "geometry.h"
#include "scanvideo_options.h"
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
			addScanlineRenderer(), ...	  (calls removeScanlineRenderer() if needed) -- throws!
			addVBlankCallback(), ...
			setIdleProc()
			startVideo()
			...
			stopVideo()
			removeVBlankCallback(), ...   (also done by teardown)
			removeScanlineRenderer(), ... (also done by teardown)
		teardown()

	addScanlineRenderer()    calls renderer.setup()    for all video_buffer.scanlines[].data[plane]
	removeScanlineRenderer() calls renderer.teardown() for all video_buffer.scanlines[].data[plane]
*/


class Scanvideo
{
	static void core1_runner() noexcept;
	void		video_runner();

public:
	using coord = Graphics::coord;
	using Size	= Graphics::Size;

	static constexpr uint max_vblank_actions = 8;
	static constexpr uint max_planes		 = PICO_SCANVIDEO_PLANE_COUNT;
	using idle_fu							 = void();

	idle_fu* idle_action = nullptr;

	uint		num_planes		   = 0;
	VideoPlane* planes[max_planes] = {nullptr};

	uint		  num_vblank_actions = 0;
	VBlankAction* vblank_actions[max_vblank_actions];
	uint8		  vblank_when[max_vblank_actions];

	bool		  video_output_enabled = false;
	volatile bool video_output_running = false;
	bool		  is_initialized	   = false;

	static Size	 size;
	static coord width() noexcept { return size.width; }
	static coord height() noexcept { return size.height; }


	static Scanvideo& getRef() noexcept; // get reference to singleton (and claim hardware)

	Error setup(const VgaMode*, const VgaTiming*);
	Error setup(const VgaMode* mode) { return setup(mode, mode->default_timing); }
	void  teardown() noexcept;

	void addPlane(VideoPlane*);
	void removePlane(VideoPlane*);
	void addVBlankAction(VBlankAction* fu, uint8 when) noexcept;
	void removeVBlankAction(VBlankAction*);
	void setIdleAction(idle_fu* fu) noexcept { idle_action = fu; }

	void startVideo();
	void stopVideo();

	static bool in_hblank() noexcept;
	static void waitForVBlank() noexcept;
	static void waitForScanline(ScanlineID n) noexcept;

private:
	Scanvideo() noexcept;
};


extern std::atomic<uint32> scanlines_missed;


} // namespace kio::Video
