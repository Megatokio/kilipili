// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer.h"
#include "Pixmap_wAttr.h"
#include <hardware/gpio.h>

#define XRAM __attribute__((section(".scratch_x.FB" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.FB" __XSTRING(__LINE__)))) // general ram


namespace kio::Video
{
using namespace Graphics;

void RAM FrameBuffer<ColorMode::colormode_rgb>::vblank(VideoPlane* vp) noexcept
{
	auto* fb   = reinterpret_cast<FrameBuffer*>(vp);
	fb->pixels = fb->pixmap->pixmap;
}

void XRAM FrameBuffer<colormode_rgb>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	auto*  fb  = reinterpret_cast<FrameBuffer*>(vp);
	uint8* px  = fb->pixels;
	fb->pixels = px + fb->row_offset;
	ScanlineRenderer_rgb(scanline, uint(width), px);
}


//	_____________________________________________________________________________________

void RAM FrameBuffer<colormode_i1>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
	//fb->scanline_renderer.vblank();	nop
}

void XRAM FrameBuffer<colormode_i1>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	//gpio_set_mask(1 << PICO_DEFAULT_LED_PIN);
	fb->scanline_renderer.render(scanline, uint(width), fb->pixels);
	fb->pixels += fb->row_offset;
	//gpio_clr_mask(1 << PICO_DEFAULT_LED_PIN);
}


//	_____________________________________________________________________________________

void RAM FrameBuffer<colormode_i2>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
}

void XRAM FrameBuffer<colormode_i2>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->scanline_renderer.render(scanline, uint(width), fb->pixels);
	fb->pixels += fb->row_offset;
}


//	_____________________________________________________________________________________

void RAM FrameBuffer<colormode_i4>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
}

void XRAM FrameBuffer<colormode_i4>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->scanline_renderer.render(scanline, uint(width), fb->pixels);
	fb->pixels += fb->row_offset;
}


//	_____________________________________________________________________________________

void RAM FrameBuffer<colormode_i8>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
}

void XRAM FrameBuffer<colormode_i8>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->scanline_renderer.render(scanline, uint(width), fb->pixels); // *** NOT HERE
	fb->pixels += fb->row_offset;
}


//	_____________________________________________________________________________________

void RAM FrameBufferBase_wAttr::vblank(VideoPlane* vp) noexcept
{
	FrameBufferBase_wAttr* fb = reinterpret_cast<FrameBufferBase_wAttr*>(vp);

	fb->pixels	   = fb->pixmap;
	fb->attributes = fb->attrmap;
	fb->arow	   = fb->attrheight;
}

void XRAM FrameBufferBase_wAttr::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBufferBase_wAttr* fb = reinterpret_cast<FrameBufferBase_wAttr*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	//gpio_set_mask(1 << PICO_DEFAULT_LED_PIN);
	fb->render_fu(scanline, uint(width), fb->pixels, fb->attributes); // *** NOT HERE
	//gpio_clr_mask(1 << PICO_DEFAULT_LED_PIN);

	fb->pixels += fb->row_offset;

	if unlikely (--fb->arow == 0)
	{
		fb->arow = fb->attrheight;
		fb->attributes += fb->arow_offset;
	}
}


// =========================================================================
// define them all, the linker will know what we need:

template class FrameBuffer<colormode_i1>;
template class FrameBuffer<colormode_i2>;
template class FrameBuffer<colormode_i4>;
template class FrameBuffer<colormode_i8>;
template class FrameBuffer<colormode_rgb>;
template class FrameBuffer<colormode_a1w1>;
template class FrameBuffer<colormode_a1w2>;
template class FrameBuffer<colormode_a1w4>;
template class FrameBuffer<colormode_a1w8>;
template class FrameBuffer<colormode_a2w1>;
template class FrameBuffer<colormode_a2w2>;
template class FrameBuffer<colormode_a2w4>;
template class FrameBuffer<colormode_a2w8>;

} // namespace kio::Video

/*
































*/
