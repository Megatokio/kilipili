// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer.h"
#include "Pixmap_wAttr.h"

#define XRAM __attribute__((section(".scratch_x.FB" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.FB" __XSTRING(__LINE__)))) // general ram


namespace kio::Video
{
using namespace Graphics;

void RAM FrameBuffer<ColorMode::colormode_rgb>::_vblank(VideoPlane* vp) noexcept
{
	auto* fb   = reinterpret_cast<FrameBuffer*>(vp);
	fb->pixels = fb->pixmap->pixmap;
	ScanlineRenderer<CM>::vblank();
}

void XRAM FrameBuffer<colormode_rgb>::_render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	auto*  fb  = reinterpret_cast<FrameBuffer*>(vp);
	uint8* px  = fb->pixels;
	fb->pixels = px + fb->row_offset;
	ScanlineRenderer<CM>::render(scanline, uint(width), px);
}


// =========================================================================

template<ColorMode CM>
void RAM FrameBuffer<CM, std::enable_if_t<is_indexed_color(CM)>>::_vblank(VideoPlane* vp) noexcept
{
	auto* fb   = reinterpret_cast<FrameBuffer*>(vp);
	fb->pixels = fb->pixmap->pixmap;
	fb->scanline_renderer.vblank();
}

template<ColorMode CM>
void XRAM FrameBuffer<CM, std::enable_if_t<is_indexed_color(CM)>>::_render(
	VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	auto*  fb  = reinterpret_cast<FrameBuffer*>(vp);
	uint8* px  = fb->pixels;
	fb->pixels = px + fb->row_offset;
	fb->scanline_renderer.render(scanline, uint(width), px);
}


// =========================================================================

template<ColorMode CM>
void RAM FrameBuffer<CM, std::enable_if_t<is_attribute_mode(CM)>>::_vblank(VideoPlane* vp) noexcept
{
	auto* fb	   = reinterpret_cast<FrameBuffer<CM>*>(vp);
	fb->pixels	   = fb->pixmap->pixmap;
	fb->attributes = fb->attrmap;
	fb->arow	   = fb->attrheight;
	ScanlineRenderer<CM>::vblank();
}

template<ColorMode CM>
void XRAM FrameBuffer<CM, std::enable_if_t<is_attribute_mode(CM)>>::_render(
	VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	auto* fb = reinterpret_cast<FrameBuffer<CM>*>(vp);

	const uint8* at = fb->attributes;
	if unlikely (--fb->arow == 0)
	{
		fb->arow	   = fb->attrheight;
		fb->attributes = at + fb->arow_offset;
	}

	const uint8* px = fb->pixels;
	fb->pixels		= px + fb->row_offset;

	ScanlineRenderer<CM>::render(scanline, uint(width), px, at);
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
