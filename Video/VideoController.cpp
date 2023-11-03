/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022 - 2023 kio@little-bat.de
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "VideoController.h"
#include "ScanlineBuffer.h"
#include "StackInfo.h"
#include "VideoBackend.h"
#include "VideoPlane.h"
#include "kilipili_cdefs.h"
#include "utilities/LoadSensor.h"
#include "utilities/utilities.h"
#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <pico/multicore.h>
#include <pico/platform.h>
#include <pico/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace kio::Video
{

#define RAM __attribute__((section(".time_critical.Scanvideo")))

using namespace kio::Graphics;

uint  scanlines_missed			   = 0;
Error VideoController::core1_error = NO_ERROR;

static spin_lock_t* spinlock = nullptr;

struct Locker
{
	uint32 state; // status register
	Locker() noexcept { state = spin_lock_blocking(spinlock); }
	~Locker() noexcept { spin_unlock(spinlock, state); }
};

#define locker() Locker _locklist


// =========================================================

using namespace Graphics;

VideoController::VideoController() noexcept
{
	VideoBackend::initialize();
	if (!spinlock) spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
}

VideoController& VideoController::getRef() noexcept
{
	// may panic on first call if HW can't be claimed

	static VideoController videocontroller;
	return videocontroller;
}

Error VideoController::setup(const VgaMode& mode, bool blocking)
{
	assert(get_core_num() == 0);
	assert(state == INVALID);

	vga_mode	   = mode;
	core1_error	   = NO_ERROR;
	idle_action	   = nullptr;
	vblank_action  = nullptr;
	onetime_action = nullptr;
	num_planes	   = 0;

	requested_state = STOPPED;
	multicore_launch_core1(start_core1);
	while (blocking && state == INVALID && core1_error == NO_ERROR) { wfe(); }
	return core1_error;
}

void VideoController::do_setup() noexcept { assert(get_core_num() == 1); }

void VideoController::startVideo(bool blocking)
{
	assert(get_core_num() == 0);
	assert(requested_state != INVALID);

	requested_state = RUNNING;
	__sev();
	while (blocking && state != RUNNING && core1_error == NO_ERROR) { wfe(); }
}

void VideoController::do_start_video()
{
	assert(get_core_num() == 1);

	try
	{
		VideoBackend::start(vga_mode, PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT);
	}
	catch (Error e)
	{
		panic("%s", e); // OOMEM
	}
}

void VideoController::stopVideo(bool blocking)
{
	assert(get_core_num() == 0);

	requested_state = STOPPED;
	__sev();
	while (blocking && state != STOPPED) { wfe(); }
}

void VideoController::do_stop_video()
{
	assert(get_core_num() == 1);

	VideoBackend::stop();
}

void VideoController::teardown(bool blocking) noexcept
{
	assert(get_core_num() == 0);

	requested_state = INVALID;
	while (blocking && state != INVALID) { wfe(); }
}

void VideoController::do_teardown() noexcept
{
	assert(get_core_num() == 1);

	idle_action	   = nullptr;
	vblank_action  = nullptr;
	onetime_action = nullptr;

	while (num_planes) { planes[--num_planes]->teardown(); }
}

void VideoController::core1_runner() noexcept
{
	assert(get_core_num() == 1);
	stackinfo();

	try
	{
		print_core();
		print_stack_free();
		init_stack_guard();

		do_setup();
		state = STOPPED;
		__sev();

		while (requested_state != INVALID)
		{
			wfe();

			if (onetime_action)
			{
				onetime_action();
				onetime_action = nullptr;
				__sev();
			}

			if (requested_state == RUNNING)
			{
				do_start_video();
				state = RUNNING;
				__sev();
				video_runner();
				do_stop_video();
				state = STOPPED;
				__sev();
			}
		}

		do_teardown();
		state = INVALID;
		__sev();
	}
	catch (std::exception& e)
	{
		panic("core1: %s", e.what());
	}
	catch (Error e)
	{
		panic("core1: %s", e);
	}
	catch (...)
	{
		panic("core1: exception");
	}
}

inline void RAM VideoController::wait_for_event() noexcept
{
	stackinfo();

	idle_start();
	if (idle_action) idle_action();
	idle_end();
}

inline void RAM VideoController::call_vblank_actions() noexcept
{
	// calls in this sequence:
	//   onetime_action
	//   vblank_action
	//   plane.vblank

	stackinfo();

	locker();

	if (onetime_action)
	{
		onetime_action();
		onetime_action = nullptr;
		__sev();
	}

	if (vblank_action)
	{
		vblank_action(); //
	}

	for (uint i = 0; i < num_planes; i++)
	{
		planes[i]->vblank(); //
	}
}

void RAM VideoController::video_runner()
{
	stackinfo();

	call_vblank_actions(); // guaranteed to be called before renderScanline

	int height = vga_mode.height;
	int row0   = line_at_frame_start;
	int row	   = current_scanline() + 1;

	for (;; row++)
	{
		if unlikely (row >= height) // next frame
		{
			if unlikely (requested_state != RUNNING) break;
			else call_vblank_actions();

			while (!in_vblank && line_at_frame_start == row0)
			{
				// wait until this frame is fully done
				// ATTN: the timing irpt is actually called ~2 lines early!
				wait_for_event();
			}

			row = 0;
			row0 += height;

			for (uint n = 0; n < scanline_buffer.count; n++, row++)
			{
				// render the first lines of the screen

				uint32* scanline = scanline_buffer[row0 + row];
				for (uint i = 0; i < num_planes; i++)
				{
					planes[i]->renderScanline(row, scanline); //
				}
			}

			while (in_vblank)
			{
				// wait until line_at_frame_start advances to next frame
				wait_for_event();
			}

			assert(line_at_frame_start == row0);
		}

		while (unlikely(row >= current_scanline() + int(scanline_buffer.count)))
		{
			// wait until video backend no longer displays from this scanline
			wait_for_event(); //
		}

		uint32* scanline = scanline_buffer[row0 + row];
		for (uint i = 0; i < num_planes; i++)
		{
			planes[i]->renderScanline(row, scanline); //
		}

		if unlikely (current_scanline() >= row)
		{
			scanlines_missed++;
			row++;
		}
	}
}

void VideoController::addPlane(VideoPlane* plane)
{
	assert(state != INVALID);
	assert(plane != nullptr);
	assert(num_planes < count_of(planes));

	// plane must be added by core1 because plane->setup() may initialize the hw interp:

	addOneTimeAction([this, plane] {
		assert(num_planes < max_planes);
		planes[num_planes] = plane;
		plane->setup(vga_mode.width); // throws
		num_planes++;
	});
}

void VideoController::removePlane(VideoPlane* plane)
{
	// plane must be removed by core1 because plane->teardown() may unclaim the hw interp:
	// note: normally not used because teardown() removes all planes.

	assert(state != INVALID);

	addOneTimeAction([this, plane] {
		for (uint i = num_planes; i;)
		{
			if (planes[--i] != plane) continue;

			plane->teardown();
			while (++i < num_planes) planes[i - 1] = planes[i];
			num_planes--;
			return;
		}
	});
}

void VideoController::setVBlankAction(const VBlankAction& fu) noexcept
{
	locker();
	vblank_action = fu;
}

void VideoController::setIdleAction(const IdleAction& fu) noexcept
{
	locker();
	idle_action = fu;
}

void VideoController::addOneTimeAction(const std::function<void()>& fu) noexcept
{
	assert(state != INVALID);

	while (volatile bool f = onetime_action != nullptr) { wfe(); }
	locker();
	onetime_action = fu;
}


} // namespace kio::Video


/*




































*/
