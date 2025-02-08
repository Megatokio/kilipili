// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer.h"
#include "VideoBackend.h"

#define XRAM __attribute__((section(".scratch_x.FB" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.FB" __XSTRING(__LINE__)))) // general ram


namespace kio::Video
{

void RAM FrameBufferBase::_vblank(VideoPlane* vp) noexcept
{
	FrameBufferBase* fb = reinterpret_cast<FrameBufferBase*>(vp);

	fb->pixels = fb->pixmap;
}

void XRAM FrameBufferBase::_render(VideoPlane* vp, int __unused row, uint32* scanline) noexcept
{
	FrameBufferBase* fb = reinterpret_cast<FrameBufferBase*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we actually miss a scanline then the remainder of the screen is shifted

	fb->render_fu(scanline, uint(vga_mode.width), fb->pixels);
	fb->pixels += fb->row_offset;
}


void RAM FrameBufferBase_wAttr::_vblank(VideoPlane* vp) noexcept
{
	FrameBufferBase_wAttr* fb = reinterpret_cast<FrameBufferBase_wAttr*>(vp);

	fb->pixels	   = fb->pixmap;
	fb->attributes = fb->attrmap;
	fb->arow	   = fb->attrheight;
}

void XRAM FrameBufferBase_wAttr::_render(VideoPlane* vp, int __unused row, uint32* scanline) noexcept
{
	FrameBufferBase_wAttr* fb = reinterpret_cast<FrameBufferBase_wAttr*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we actually miss a scanline then the remainder of the screen is shifted

	fb->render_fu(scanline, uint(vga_mode.width), fb->pixels, fb->attributes);

	fb->pixels += fb->row_offset;

	if unlikely (--fb->arow == 0)
	{
		fb->arow = fb->attrheight;
		fb->attributes += fb->arow_offset;
	}
}

} // namespace kio::Video
