// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VideoPlane.h"

#define XRAM __attribute__((section(".scratch_x.VP" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.VP" __XSTRING(__LINE__)))) // general ram

namespace kio::Video
{

extern volatile bool locked_out; // in VideoController.cpp


void RAM VideoPlane::do_vblank(VideoPlane* vp) noexcept
{
	if (!locked_out) vp->vblank();
}

void RAM VideoPlane::do_render(VideoPlane* vp, int row, int width, uint32* buffer) noexcept
{
	if (!locked_out) vp->renderScanline(row, width, buffer);
}


} // namespace kio::Video
