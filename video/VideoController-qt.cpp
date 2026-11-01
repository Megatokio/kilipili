// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

//#include "VideoController.h"
#include "VideoPlane.h"
#include "common/standard_types.h"
#include "common/timing.h"
#include "internal/VideoBackend.h"

// symbols in VideoController.cpp:
extern volatile bool lockout_requested;


namespace kilipili
{

// symbols in VideoController.cpp:
enum State : bool {
	STOPPED,
	RUNNING,
};
extern volatile State requested_state;
extern uint			  num_planes;
extern VideoPlanePtr  planes[];
extern void			  call_vblank_actions() noexcept;
extern uint			  scanlines_missed;
extern volatile bool  locked_out;

// symbols in video backend emulation:
extern uint32* get_scanline(int row);


void video_runner()
{
	assert(!locked_out);

	uint32 cc_max_ahead		= 16 * cc_per_scanline;
	int	   row0				= line_at_frame_start; // rolling number
	uint32 cc_at_line_start = time_cc_at_frame_start + uint(vga_mode.height) * cc_per_scanline - cc_max_ahead;

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

			for (uint i = 0; i < num_planes; i++)
			{
				VideoPlane* vp = planes[i];
				vp->vblank_fu(vp);
			}

			cc_at_line_start -= uint(row) * cc_per_scanline;
			cc_at_line_start += cc_per_frame;

			row0 += vga_mode.height;
			row = 0;
		}

		while (int(time_cc_32() - cc_at_line_start) < 0)
		{
			idle_start();
			sleep_us(50);

			if (lockout_requested != locked_out)
			{
				locked_out = lockout_requested;
				__sev();
			}
		}
		idle_end();

		uint32* scanline = get_scanline(row);
		for (uint i = 0; i < num_planes; i++)
		{
			VideoPlane* vp = planes[i];
			vp->render_fu(vp, row, vga_mode.width, scanline);
		}
	}
}

} // namespace kilipili


/*













































*/
