// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#include "VideoController.h"
#include "ScanlineBuffer.h"
#include "VideoBackend.h"
#include "VideoPlane.h"
#include "cdefs.h"
#include "tempmem.h"
#include "utilities/LoadSensor.h"
#include "utilities/Trace.h"
#include "utilities/stack_guard.h"
#include "utilities/utilities.h"
#include <cstdio>
#include <hardware/exception.h>
#include <pico/multicore.h>


#define XRAM __attribute__((section(".scratch_x.VC" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.VC" __XSTRING(__LINE__)))) // general ram


using namespace kio::Video;


static volatile bool lockout_requested = false;
static volatile bool locked_out		   = false;

__weak_symbol void suspend_core1() noexcept
{
	assert(get_core_num() == 0);
	assert(lockout_requested == false);

	while (locked_out) kio::wfe(); // because we don't wait in resume_core1()
	lockout_requested = true;
	__sev();
	while (!locked_out) kio::wfe();
}

__weak_symbol void resume_core1() noexcept
{
	assert(get_core_num() == 0);
	assert(lockout_requested == true);
	assert(locked_out == true);

	lockout_requested = false;
	__sev();
}

__noreturn RAM static void hard_fault_handler() noexcept
{
	if (!locked_out) kio::panic("HARDFAULT_EXCEPTION");

	if constexpr (1) { for (;;); }
	else
	{
		// core0 = short ON, long off
		// core1 = long ON, short off

		static constexpr uint pin  = PICO_DEFAULT_LED_PIN;
		uint				  mask = 1 << pin;
		gpio_set_mask(mask);
		for (uint n = get_core_num();;)
		{
			for (uint end = time_us_32() + 150 * 1000; time_us_32() != end;) {}
			if ((++n & 3) <= 1) gpio_xor_mask(mask);
		}
	}
}


namespace kio::Video
{

uint scanlines_missed = 0;

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
	VideoBackend::initialize();
	spinlock		= spin_lock_init(uint(spin_lock_claim_unused(true)));
	requested_state = STOPPED;
	multicore_launch_core1([] {
		exception_set_exclusive_handler(
			HARDFAULT_EXCEPTION,
			hard_fault_handler);	 // contraire to documentation, this sets the handler for both cores
		assert(get_core_num() == 1); // yes we are on core1
		sleep_us(10);				 // static videocontroller's ctor must finish
		getRef().core1_runner();	 // else calling getRef() leads to recursion
	});
}

VideoController& VideoController::getRef() noexcept
{
	// may panic on first call if HW can't be claimed

	static VideoController videocontroller;
	return videocontroller;
}

void VideoController::startVideo(const VgaMode& mode, uint32 system_clock, uint scanline_buffer_count)
{
	assert(get_core_num() == 0);
	assert(state == STOPPED);
	assert(requested_state == STOPPED);
	assert(lockout_requested == false);
	assert(locked_out == false);

	vga_mode = mode;
	scanline_buffer.setup(vga_mode, scanline_buffer_count); // throws
	requested_system_clock = system_clock;
	requested_state		   = RUNNING;
	__sev();
	while (state != RUNNING) { wfe(); }
}

void VideoController::stopVideo() noexcept
{
	assert(get_core_num() == 0);

	requested_state = STOPPED;
	__sev();
	while (state != STOPPED) { wfe(); }
	onetime_action = nullptr; // if planes were added but the VideoController was never started
}

static void __noinline RAM poll_isr(volatile bool& lockout) noexcept
{
	while (lockout) { __wfe(); }
}

void __noreturn VideoController::core1_runner() noexcept
{
	assert(get_core_num() == 1);
	assert(state == STOPPED);
	trace(__func__);

	try
	{
		if (debug) print_core();
		if (debug) print_stack_free();
		init_stack_guard();

		while (true)
		{
			wfe();
			if (lockout_requested)
			{
				locked_out = true;
				__sev();
				poll_isr(lockout_requested);
				locked_out = false;
				__sev();
			}

			if (requested_state == RUNNING)
			{
				VideoBackend::start(vga_mode, requested_system_clock);
				state = RUNNING;
				__sev();

				{ // setup calculations for video_runner() done here in flash to save ram:
					uint32 cc_max_ahead = (scanline_buffer.count - 1) * cc_per_scanline +
										  ((vga_mode.h_total() - vga_mode.h_active()) + 18) * cc_per_px + 50 + 50;
				a:
					int	   row0 = line_at_frame_start; // rolling number
					uint32 cc_at_line_start =
						time_cc_at_frame_start + uint(vga_mode.height) * cc_per_scanline - cc_max_ahead;
					if (row0 != line_at_frame_start) goto a;

					video_runner(row0, cc_at_line_start);
					assert(requested_state == STOPPED);
				}

				VideoBackend::stop();
				while (num_planes)
				{
					planes[--num_planes]->teardown();
					planes[num_planes] = nullptr;
				}
				scanline_buffer.teardown();
				idle_action	   = nullptr;
				vblank_action  = nullptr;
				onetime_action = nullptr;
				state		   = STOPPED;
				purge_tempmem();
				__sev();
			}
		}
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
		panic("core1: unknown exception");
	}
}

__noinline void VideoController::call_vblank_actions() noexcept
{
	if (onetime_action)
	{
		onetime_action();
		onetime_action = nullptr;
		__sev();
	}

	if (vblank_action) { vblank_action(); }
}

__force_inline RAM void VideoController::vblank(VideoPlane* vp) noexcept
{
	if (vp->vblank_fu) vp->vblank_fu(vp);
	else if (!locked_out) vp->vblank();
}

__force_inline RAM void VideoController::render(VideoPlane* vp, int row, uint32* fb) noexcept
{
	if (vp->render_fu) vp->render_fu(vp, row, fb);
	else if (!locked_out) vp->renderScanline(row, fb);
}

void RAM VideoController::video_runner(int row0, uint32 cc_at_line_start)
{
	trace(__func__);
	assert(!locked_out);

	// before rendering scanline `row` we must wait until the video backend no longer displays from
	// scanline_buffer[row]
	// time_cc_32() is 0..cc_per_us cc too low: we need the lower limit though actual cc may be higher
	//			- the calculation adds some delay	 => 50cc
	//			- more delay until the first scanline_renderer() stores it's first pixel => 50cc
	// the pixel dma reads the last data 18 pixels before the end of in-screen area
	//			=> (vga.width-18) * cc_per_pixel
	//			but for half size modes scanlines are repeated and we must wait for the last repetition
	//			=> cc_per_scanline - (px_per_hsync+18) * cc_per_px
	//			=> cc_per_scanline - ((vga.h_total-vga.width)+18) * cc_per_px

	//	uint32 cc_max_ahead =												//
	//		(scanline_buffer.count - 1) * cc_per_scanline					//
	//		+ ((vga_mode.h_total() - vga_mode.h_active()) + 18) * cc_per_px //
	//		+ 50 + 50;
	//a:
	//	int	   row0				= line_at_frame_start; // rolling number
	//	uint32 cc_at_line_start = time_cc_at_frame_start + uint(vga_mode.height) * cc_per_scanline - cc_max_ahead;
	//	if (row0 != line_at_frame_start) goto a;


	// note: VideoPlane::vblank() is guaranteed to be called before VideoPlane::renderScanline()

	for (int row = vga_mode.height; requested_state == RUNNING; row++, cc_at_line_start += cc_per_scanline)
	{
		if unlikely (row0 != line_at_frame_start)
		{
			int missed = vga_mode.height - row;
			scanlines_missed += uint(missed);
			cc_at_line_start += uint(missed) * cc_per_scanline;
			row += missed;
		}

		if unlikely (row >= vga_mode.height) // next frame
		{
			if (!locked_out) call_vblank_actions(); // in rom: only if !lockout

			for (uint i = 0; i < num_planes; i++) { vblank(planes[i]); }

			// the pixel dma starts reading the first pixels of a scanline
			// (8+1)*2 = 18 pixels before the end of the previous line
			// => if the 1st pixels of the 1st line of the next frame are not rendered
			//	  before the last 18 pixels of the last line of the current frame are displayed,
			//	  then the dma reads 18 not-yet-rendered pixels for the first line before it
			//	  is blocked by the pio.
			//    to avoid this (if vblank actions finish quickly) we render the first line immediately.
			// this is a minor glitch and we could ignore this and save some space if we don't bother.

			row0 += vga_mode.height;

			while (int(time_cc_32() - cc_at_line_start) < 0) {}
			for (uint i = 0; i < num_planes; i++) { render(planes[i], 0, scanline_buffer[row0]); }

			cc_at_line_start -= uint(row) * cc_per_scanline;
			cc_at_line_start += cc_per_frame + cc_per_scanline;
			row = 1;
		}

		while (int(time_cc_32() - cc_at_line_start) < 0)
		{
			idle_start();

			if (lockout_requested != locked_out)
			{
				locked_out = lockout_requested;
				__sev();
			}
		}
		idle_end();

		uint32* scanline = scanline_buffer[row0 + row];
		for (uint i = 0; i < num_planes; i++) { render(planes[i], row, scanline); }
	}
}

void VideoController::addPlane(VideoPlanePtr plane)
{
	assert(plane != nullptr);
	assert_lt(num_planes, count_of(planes));

	// plane must be added by core1 because plane->setup() may initialize the hw interp:

	addOneTimeAction([this, plane] {
		locker();
		assert_lt(num_planes, max_planes);
		plane->setup(); // throws
		planes[num_planes++] = plane;
	});
}

void VideoController::removePlane(VideoPlanePtr plane)
{
	// plane must be removed by core1 because plane->teardown() may unclaim the hw interp:
	// note: normally not used because teardown() removes all planes.

	addOneTimeAction([this, plane] {
		locker();
		for (uint i = num_planes; i;)
		{
			if (planes[--i] != plane) continue;

			plane->teardown();
			while (++i < num_planes) std::swap(planes[i - 1], planes[i]);
			planes[--num_planes] = nullptr;
			return;
		}
	});
}

void VideoController::setVBlankAction(const VBlankAction& fu) noexcept
{
	locker();
	vblank_action = fu;
}

void VideoController::addOneTimeAction(const std::function<void()>& fu) noexcept
{
	locker();

	if (!onetime_action)
	{
		onetime_action = fu; //
	}
	else
	{
		OneTimeAction ota = std::move(onetime_action);
		onetime_action	  = [=] {
			   ota();
			   fu();
		};
	}
}


} // namespace kio::Video


/*




































*/
