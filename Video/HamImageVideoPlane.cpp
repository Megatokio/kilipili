// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HamImageVideoPlane.h"
#include "common/cdefs.h"


#define RAM	 __attribute__((section(".time_critical.HAM" __XSTRING(__LINE__)))) // general ram
#define XRAM __attribute__((section(".scratch_x.HAM" __XSTRING(__LINE__))))		// the 4k page with the core1 stack


namespace kio::Video
{

using namespace Graphics;

HamImageVideoPlane::HamImageVideoPlane(const Pixmap* pm, const ColorMap* cm, uint16 first_rel_code) :
	VideoPlane(&do_vblank, &do_render),
	pixmap(pm),
	colormap(cm),
	scanline_renderer(cm->colors, first_rel_code),
	row_offset(pm->row_offset),
	pixels(pm->pixmap)
{
	if (row_offset & 1) throw "ham image: odd row offset not supported";
	if (pm->width & 3) {} // then up to 3 rightmost pixel at the right border will never  beset
}

void HamImageVideoPlane::setupNextImage(int new_row_offset, uint16 new_first_rel_code)
{
	// set row_offset and first_rel_code.
	// it is assumed that the caller updates the contents of the pixmap and the colormap.
	// it is not neccessary to modify the pixmap width etc., they are not used.
	// prevent display of garbage during image update by setting Passepartout.inner_height to 0.

	if (new_row_offset & 1) throw "ham image: odd row offset not supported";

	row_offset						 = new_row_offset;
	scanline_renderer.first_rel_code = new_first_rel_code;
}

void RAM HamImageVideoPlane::do_vblank(VideoPlane* vp) noexcept
{
	HamImageVideoPlane* me = reinterpret_cast<HamImageVideoPlane*>(vp);
	me->pixels			   = me->pixmap->pixmap;
	me->scanline_renderer.vblank();
}

void XRAM HamImageVideoPlane::do_render(VideoPlane* vp, __unused int row, int width, uint32* fbu) noexcept
{
	HamImageVideoPlane* me = reinterpret_cast<HamImageVideoPlane*>(vp);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we actually miss a scanline then let it be

	const uint8* px = me->pixels;
	me->pixels		= px + me->row_offset;
	me->scanline_renderer.render(fbu, uint(width), px);
}


} // namespace kio::Video


/*








































*/
