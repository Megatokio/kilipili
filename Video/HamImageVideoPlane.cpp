// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HamImageVideoPlane.h"
#include "internal/Interp.h"


#define RAM	 __attribute__((section(".time_critical.HAM" __XSTRING(__LINE__)))) // general ram
#define XRAM __attribute__((section(".scratch_x.HAM" __XSTRING(__LINE__))))		// the 4k page with the core1 stack


namespace kilipili::Video
{

using namespace Graphics;

HamImageVideoPlane::HamImageVideoPlane(const Pixmap* pm, const ColorMap* cm, uint16 first_rel_code) :
	VideoPlane(&do_vblank, &do_render),
	pixmap(pm),
	colormap(cm),
	row_offset(pm->row_offset),
	pixels(pm->pixmap),
	first_rel_code(first_rel_code)
{
	if (row_offset & 1) throw "ham image: odd row offset not supported";
	if (pm->width & 3) {} // then up to 3 rightmost pixel at the right border will never be set
}

void HamImageVideoPlane::setupNextImage(int new_row_offset, uint16 new_first_rel_code)
{
	// set row_offset and first_rel_code.
	// it is assumed that the caller updates the contents of the pixmap and the colormap.
	// it is not neccessary to modify the pixmap width etc., they are not used.
	// avoid display of garbage during image update by setting Passepartout.inner_height to 0.

	if (new_row_offset & 1) throw "ham image: odd row offset not supported";

	row_offset	   = new_row_offset;
	first_rel_code = new_first_rel_code;
}

void RAM HamImageVideoPlane::do_vblank(VideoPlane* vp) noexcept
{
	HamImageVideoPlane* me = static_cast<HamImageVideoPlane*>(vp);
	me->pixels			   = me->pixmap->pixmap;
	me->first_color		   = Graphics::black;
}

static __force_inline Color operator+(Color a, Color b) { return Color(a.raw + b.raw); }

void XRAM HamImageVideoPlane::do_render(VideoPlane* vp, int /*row*/, int width, uint32* framebuffer) noexcept
{
	HamImageVideoPlane* me = static_cast<HamImageVideoPlane*>(vp);

	const uint8* px = me->pixels;
	me->pixels		= px + me->row_offset;

	constexpr InterpMode ip_mode = InterpMode::i8;
	setup_if_needed<ip_mode>();
	Interp* interp = &interp0[ipi<ip_mode>];
	interp->set_color_base(me->colormap->colors);

	const Color*  first_rel_color = &me->colormap->colors[me->first_rel_code];
	Color		  current_color	  = me->first_color;
	const uint16* pixels		  = reinterpret_cast<const uint16*>(px);
	Color*		  dest			  = reinterpret_cast<Color*>(framebuffer);
	Color*		  first_pixel	  = dest;

	for (uint i = 0; i < uint(width) / 4; i++)
	{
		const Color* color;

		interp->set_pixels(*pixels++);

		color	= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;

		color	= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;

		interp->set_pixels(*pixels++);

		color	= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;

		color	= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;
	}

	me->first_color = *first_pixel;
	cleanup_if_needed<ip_mode>();
}


} // namespace kilipili::Video


/*








































*/
