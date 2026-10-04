// Copyright (c) 2025 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VideoPlane.h"
#include "common/Logger.h"
#include "common/memory.h"

#define XRAM __attribute__((section(".scratch_x.VP" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.VP" __XSTRING(__LINE__)))) // general ram

namespace kilipili::Video
{

extern volatile bool locked_out; // in Video.cpp


VideoPlane::VideoPlane(VblankFu* a, RenderFu* b) noexcept : vblank_fu(a), render_fu(b)
{
	if (size_t(a) >= flash_start() && size_t(a) < flash_end())
	{
		logline("WARNING: vblank function is in flash!"); //
	}
	if (size_t(b) >= flash_start() && size_t(b) < flash_end())
	{
		logline("WARNING: scanline render function is in flash!");
	}
}

void RAM VideoPlane::do_vblank(VideoPlane* vp) noexcept
{
	if (!locked_out) vp->vblank();
}

void RAM VideoPlane::do_render(VideoPlane* vp, int row, int width, uint32* buffer) noexcept
{
	if (!locked_out) vp->renderScanline(row, width, buffer);
}


} // namespace kilipili::Video
