// Copyright (c) 2025 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer<Pixmap>.h"

#define RENDER __section(RAM ".fb")
#define VBLANK __section(RAM ".fb")


namespace kilipili::Video
{
using namespace Graphics;

void VBLANK FrameBuffer<Pixmap_rgb>::vblank(VideoPlane* vp) noexcept
{
	auto* fb   = reinterpret_cast<FrameBuffer*>(vp);
	fb->pixels = fb->pixmap->pixmap;
}

void RENDER FrameBuffer<Pixmap_rgb>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
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

void VBLANK FrameBuffer<Pixmap_i1>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
	//fb->scanline_renderer.vblank();	nop
}

void RENDER FrameBuffer<Pixmap_i1>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->scanline_renderer.render(scanline, uint(width), fb->pixels);
	fb->pixels += fb->row_offset;
}


//	_____________________________________________________________________________________

void VBLANK FrameBuffer<Pixmap_i2>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
}

void RENDER FrameBuffer<Pixmap_i2>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->scanline_renderer.render(scanline, uint(width), fb->pixels);
	fb->pixels += fb->row_offset;
}


//	_____________________________________________________________________________________

void VBLANK FrameBuffer<Pixmap_i4>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
}

void RENDER FrameBuffer<Pixmap_i4>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->scanline_renderer.render(scanline, uint(width), fb->pixels);
	fb->pixels += fb->row_offset;
}


//	_____________________________________________________________________________________

void VBLANK FrameBuffer<Pixmap_i8>::vblank(VideoPlane* vp) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	fb->pixels = fb->pixmap->pixmap;
}

void RENDER FrameBuffer<Pixmap_i8>::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBuffer* fb = reinterpret_cast<FrameBuffer*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->scanline_renderer.render(scanline, uint(width), fb->pixels);
	fb->pixels += fb->row_offset;
}


//	_____________________________________________________________________________________

void VBLANK FrameBufferBase_wAttr::vblank(VideoPlane* vp) noexcept
{
	FrameBufferBase_wAttr* fb = reinterpret_cast<FrameBufferBase_wAttr*>(vp);

	fb->pixels	   = fb->pixmap;
	fb->attributes = fb->attrmap;
	fb->arow	   = fb->attrheight;
}

void RENDER FrameBufferBase_wAttr::render(VideoPlane* vp, int __unused row, int width, uint32* scanline) noexcept
{
	FrameBufferBase_wAttr* fb = reinterpret_cast<FrameBufferBase_wAttr*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we miss a scanline then the remainder of the screen is shifted

	fb->render_fu(scanline, uint(width), fb->pixels, fb->attributes);

	fb->pixels += fb->row_offset;

	if unlikely (--fb->arow == 0)
	{
		fb->arow = fb->attrheight;
		fb->attributes += fb->arow_offset;
	}
}


// =========================================================================
// define them all, the linker will know what we need:

template class FrameBuffer<Pixmap_i1>;
template class FrameBuffer<Pixmap_i2>;
template class FrameBuffer<Pixmap_i4>;
template class FrameBuffer<Pixmap_i8>;
template class FrameBuffer<Pixmap_rgb>;
template class FrameBuffer<Pixmap_a1w1>;
template class FrameBuffer<Pixmap_a1w2>;
template class FrameBuffer<Pixmap_a1w4>;
template class FrameBuffer<Pixmap_a1w8>;
template class FrameBuffer<Pixmap_a2w1>;
template class FrameBuffer<Pixmap_a2w2>;
template class FrameBuffer<Pixmap_a2w4>;
template class FrameBuffer<Pixmap_a2w8>;

} // namespace kilipili::Video

/*
































*/
