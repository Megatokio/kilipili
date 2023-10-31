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


uint scanlines_missed = 0;

Size VideoController::size = {0, 0};

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


Error VideoController::setup(const VgaMode* mode, bool blocking)
{
	assert(get_core_num() == 0);

	assert(state == INVALID);
	assert(num_planes == 0);
	assert(num_vblank_actions == 0);
	assert(idle_action == nullptr);
	assert(mode != nullptr);

	vga_mode		= mode;
	core1_error		= NO_ERROR;
	requested_state = STOPPED;
	multicore_launch_core1(start_core1);
	while (blocking && state == INVALID && core1_error == NO_ERROR) { wfe(); }
	return core1_error;
}

void VideoController::startVideo(bool blocking)
{
	assert(get_core_num() == 0);
	assert(requested_state != INVALID);

	requested_state = RUNNING;
	__sev();
	while (blocking && state != RUNNING && core1_error == NO_ERROR) { wfe(); }
}

void VideoController::stopVideo(bool blocking)
{
	assert(get_core_num() == 0);

	requested_state = STOPPED;
	__sev();
	while (blocking && state != STOPPED) { wfe(); }
}

void VideoController::teardown(bool blocking) noexcept
{
	assert(get_core_num() == 0);

	requested_state = INVALID;
	while (blocking && state != INVALID) { wfe(); }
}

void VideoController::do_setup() noexcept
{
	assert(get_core_num() == 1);
	assert(vga_mode != nullptr);

	size.width	= vga_mode->width;
	size.height = vga_mode->height;
}

void VideoController::start_video()
{
	assert(get_core_num() == 1);
	assert(vga_mode != nullptr);

	try
	{
		VideoBackend::start(vga_mode, PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT);
	}
	catch (Error e)
	{
		panic("%s", e); // OOMEM
	}
}

void VideoController::stop_video()
{
	assert(get_core_num() == 1);

	VideoBackend::stop();
}

void VideoController::do_teardown() noexcept
{
	assert(get_core_num() == 1);

	idle_action		   = nullptr;
	num_vblank_actions = 0;

	while (num_onetime_actions)
	{
		onetime_actions[--num_onetime_actions] = [] {};
	}

	while (num_planes)
	{
		planes[--num_planes]->teardown(); //
	}
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

			if (requested_state == RUNNING)
			{
				start_video();
				state = RUNNING;
				__sev();
				video_runner();
				stop_video();
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
	if (auto* fu = idle_action) fu();
	else __wfe();
	idle_end();
}

inline void RAM VideoController::call_vblank_actions() noexcept
{
	stackinfo();

	locker();
	if (unlikely(num_onetime_actions))
	{
		for (uint i = 0; i < num_onetime_actions; i++)
		{
			auto& ota = onetime_actions[i];
			ota();
			ota = [] {};
		}
		num_onetime_actions = 0;
	}
	for (uint i = 0; i < num_vblank_actions; i++) { vblank_actions[i]->vblank(); }
	for (uint i = 0; i < num_planes; i++) { planes[i]->vblank(); }
}

void RAM VideoController::video_runner()
{
	stackinfo();

	int height = vga_mode->v_active;
	int row0   = current_frame_start;
	int row	   = current_scanline;

	row += scanline_buffer.yscale - (row - row0) % scanline_buffer.yscale;

	ScanlineBuffer& scanlines = scanline_buffer;

	for (;; row += scanlines.yscale)
	{
		if unlikely (row - row0 >= height) // next frame
		{
			row0 += height;

			if unlikely (requested_state != RUNNING) break;
			else call_vblank_actions();
		}

		assert(uint(row - row0) < uint(height));

		while (unlikely(row + scanline_buffer.yscale - current_scanline >= scanlines.count))
		{
			wait_for_event(); //
		}

		uint32* scanline = scanlines[row];

		for (uint i = 0; i < num_planes; i++)
		{
			planes[i]->renderScanline((row - row0) / scanlines.yscale, scanline); //
		}

		if unlikely (current_scanline >= row)
		{
			scanlines_missed++;
			row += scanlines.yscale;
		}
	}
}

void VideoController::waitForVBlank() noexcept { Video::waitForVBlank(); }
void VideoController::waitForScanline(int n) noexcept { Video::waitForScanline(n); }

void VideoController::addOneTimeAction(std::function<void()> fu) noexcept
{
	assert(state != INVALID);
	assert(num_onetime_actions < max_onetime_actions);
	locker();
	onetime_actions[num_onetime_actions++] = std::move(fu);
}

void VideoController::addPlane(VideoPlane* plane)
{
	assert(state != INVALID);
	assert(plane != nullptr);
	assert(num_planes < max_planes);

	// plane must be added by core1 because plane->setup() may initialize the hw interp:

	addOneTimeAction([this, plane] {
		assert(num_planes < max_planes);
		planes[num_planes] = plane;
		plane->setup(size.width); // throws
		num_planes++;
	});
}

void VideoController::removePlane(VideoPlane* plane)
{
	// plane must be the last plane!
	// note: normally not used because teardown() removes all planes.

	locker();
	assert(num_planes && planes[num_planes - 1] == plane);

	// decrement early because video_runner() does not lock
	num_planes--;
	plane->teardown();
}

void VideoController::addVBlankAction(VBlankAction* fu, uint8 when) noexcept
{
	assert(state != INVALID);
	assert(fu != nullptr);

	locker();
	assert(num_vblank_actions < max_vblank_actions);

	uint i = num_vblank_actions;
	while (i && when > vblank_when[i])
	{
		vblank_actions[i] = vblank_actions[i - 1];
		vblank_when[i]	  = vblank_when[i - 1];
		i--;
	}
	vblank_actions[i] = fu;
	vblank_when[i]	  = when;
	num_vblank_actions += 1;
}

void VideoController::removeVBlankAction(VBlankAction* fu)
{
	locker();

	for (uint i = 0; i < num_vblank_actions; i++)
	{
		if (vblank_actions[i] == fu)
		{
			while (++i < num_vblank_actions)
			{
				vblank_actions[i - 1] = vblank_actions[i];
				vblank_when[i - 1]	  = vblank_when[i];
			}
			num_vblank_actions -= 1;
			return;
		}
	}
}


} // namespace kio::Video


/*




































*/
