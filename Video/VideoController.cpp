/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * SPDX-License-Identifier: BSD-3-Clause
 * Copyright (c) 2022 - 2023 kio@little-bat.de
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "VideoController.h"
#include "ScanlineSM.h"
#include "StackInfo.h"
#include "TimingSM.h"
#include "VideoPlane.h"
#include "VideoQueue.h"
#include "composable_scanline.h"
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

constexpr uint32 DMA_CHANNELS_MASK = (1u << PICO_SCANVIDEO_TIMING_DMA_CHANNEL) |
									 (1u << PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL1) |
									 ((PICO_SCANVIDEO_PLANE_COUNT >= 2) << PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL2) |
									 ((PICO_SCANVIDEO_PLANE_COUNT >= 3) << PICO_SCANVIDEO_SCANLINE_DMA_CHANNEL3);

constexpr uint32 SM_MASK = (1u << PICO_SCANVIDEO_SCANLINE_SM1) | (1u << PICO_SCANVIDEO_TIMING_SM) |
						   ((PICO_SCANVIDEO_PLANE_COUNT >= 2) << PICO_SCANVIDEO_SCANLINE_SM2) |
						   ((PICO_SCANVIDEO_PLANE_COUNT >= 3) << PICO_SCANVIDEO_SCANLINE_SM3);

#define RAM __attribute__((section(".time_critical.Scanvideo")))


using namespace Graphics;

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

VideoController::VideoController() noexcept
{
	pio_claim_sm_mask(video_pio, 0x0f); // claim all because we clear instruction memory
	dma_claim_mask(DMA_CHANNELS_MASK);
	if (!spinlock) spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
}

VideoController& VideoController::getRef() noexcept
{
	// may panic on first call if HW can't be claimed

	static VideoController scanvideo;
	return scanvideo;
}

Error VideoController::setup(const VgaMode* mode, const VgaTiming* timing)
{
	assert(get_core_num() == 0);
	assert(!video_output_enabled);
	assert(num_planes == 0);
	assert(num_vblank_actions == 0);
	assert(idle_action == nullptr);

	size.width	= mode->width;
	size.height = mode->height;

	pio_set_sm_mask_enabled(video_pio, 0x0f, false); // stop all 4 state machines
	pio_clear_instruction_memory(video_pio);

	if (Error e = scanline_sm.setup(mode, timing)) return e;
	if (Error e = timing_sm.setup(mode, timing)) return e;

	is_initialized = true;
	return NO_ERROR;
}

void VideoController::teardown() noexcept
{
	assert(get_core_num() == 0);
	assert(!video_output_enabled);

	idle_action		   = nullptr;
	num_vblank_actions = 0;

	while (num_onetime_actions)
	{
		onetime_actions[--num_onetime_actions] = [] {};
	}

	while (num_planes)
	{
		num_planes--;
		planes[num_planes]->teardown(num_planes, video_queue);
	}

	is_initialized = false;
}

void VideoController::addOneTimeAction(std::function<void()> fu) noexcept
{
	assert(is_initialized);
	assert(num_onetime_actions < max_onetime_actions);
	locker();
	onetime_actions[num_onetime_actions++] = std::move(fu);
}

void VideoController::addPlane(VideoPlane* plane)
{
	assert(is_initialized);
	assert(plane != nullptr);
	assert(num_planes < max_planes);

	// plane must be added by core1 because plane->setup() may initialize the hw interp:

	addOneTimeAction([this, plane] {
		assert(num_planes < max_planes);
		planes[num_planes] = plane;
		plane->setup(num_planes, width(), video_queue); // throws
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
	plane->teardown(--num_planes, video_queue);
}

void VideoController::addVBlankAction(VBlankAction* fu, uint8 when) noexcept
{
	assert(is_initialized);
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

void VideoController::startVideo()
{
	assert(get_core_num() == 0);
	assert(is_initialized);
	if (video_output_enabled) return;

	video_queue.reset();
	scanline_sm.start();
	sleep_ms(1);
	timing_sm.start();
	pio_clkdiv_restart_sm_mask(video_pio, SM_MASK); // synchronize fractional divider

	video_output_enabled = true;
	multicore_launch_core1(core1_runner);
}

void VideoController::stopVideo()
{
	assert(get_core_num() == 0);
	if (!video_output_enabled) return;

	video_output_enabled = false;
	while (video_output_running) wfe();
	multicore_reset_core1();

	timing_sm.stop();
	scanline_sm.stop();
}

//static
void VideoController::core1_runner() noexcept
{
	//debuginfo();

	try
	{
		print_core();
		print_stack_free();
		init_stack_guard();

		auto& scanvideo				   = getRef();
		scanvideo.video_output_running = true;
		scanvideo.video_runner();
		scanvideo.video_output_running = false;
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

void RAM VideoController::video_runner()
{
	debuginfo();

	int row = 9999;

	while (1)
	{
	a:
		Scanline* scanline = scanline_sm.getScanlineForGenerating();
		if (unlikely(!scanline))
		{
			idle_start();
			if (auto* fu = idle_action) fu();
			else __wfe();
			idle_end();
			goto a;
		}

		if (unlikely(++row != scanline->id.scanline)) // next frame or missed row
		{
			if (row > scanline->id.scanline) // next frame
			{
				if (unlikely(!video_output_enabled)) break;
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

			row = scanline->id.scanline;
		}

		for (uint i = 0; i < num_planes; i++)
		{
			auto& data = scanline->data[i];
			data.used  = uint16(planes[i]->renderScanline(row, data.data));
			assert(data.used <= data.max);
		}

		scanline_sm.pushGeneratedScanline();
	}
}

bool VideoController::in_hblank() noexcept { return scanline_sm.in_hblank(); }
void VideoController::waitForVBlank() noexcept { scanline_sm.waitForVBlank(); }
void VideoController::waitForScanline(ScanlineID n) noexcept { scanline_sm.waitForScanline(n); }


} // namespace kio::Video


/*




































*/
