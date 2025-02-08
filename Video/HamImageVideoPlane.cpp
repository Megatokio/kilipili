// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HamImageVideoPlane.h"
#include "ScanlineRenderFu.h"
#include "VideoBackend.h"
#include "VideoController.h"
#include "common/cdefs.h"
#include <hardware/interp.h>

#define RAM __attribute__((section(".time_critical." __XSTRING(__LINE__)))) // general ram
#if !defined VIDEO_HAM_RENDER_FUNCTION_IN_XRAM || VIDEO_HAM_RENDER_FUNCTION_IN_XRAM
  #define XRAM __attribute__((section(".scratch_x." __XSTRING(__LINE__)))) // the 4k page with the core1 stack
#else
  #define XRAM RAM
#endif


namespace kio::Video
{

using namespace Graphics;
constexpr uint		 lane0 = 0;
constexpr uint		 lane1 = 1;
extern template void setupScanlineRenderer<Graphics::colormode_i8>(const Color* colormap);
extern template void teardownScanlineRenderer<Graphics::colormode_i8>();

static const __force_inline Color* next_color(interp_hw_t* interp)
{
	return reinterpret_cast<const Color*>(interp_pop_lane_result(interp, lane1));
}

static __force_inline Color operator+(Color a, Color b) { return Color(a.raw + b.raw); }


HamImageVideoPlane::HamImageVideoPlane(const Pixmap* pm, const ColorMap* cm, uint first_rel_code) :
	VideoPlane(&do_vblank, &do_render),
	pixmap(pm),
	colormap(cm),
	first_rel_code(first_rel_code)
{
	if (pm->width & 1) throw "ham image: odd row offset not supported";
}

void HamImageVideoPlane::_setup(int w, int h, uint n) noexcept
{
	first_rel_code = n;
	image_width	   = w & 0xfffc; // for renderScanline() optimizations
	image_height   = h;
	row_offset	   = w;

	top_border	 = max(0, (vga_mode.height - h) / 2);
	left_border	 = max(0, (vga_mode.width - w) / 2) & 0xfffe;
	right_border = max(0, vga_mode.width - w - left_border);

	do_vblank(this);
}

void HamImageVideoPlane::setup()
{
	_setup(pixmap->width, pixmap->height, first_rel_code);

	setupScanlineRenderer<colormode_i8>(colormap->colors);
}

void HamImageVideoPlane::setupNextImage(int width, int height, uint first_rel_code)
{
	// set width, height and first_rel_code on the next vblank.
	// it is assumed that the caller updates the contents of the pixmap and the colormap.
	// it is not neccessary to modify the pixmap width etc., they are not used.
	//
	// suggested sequence:
	// 1. setupNextImage
	// 2. copy cmap
	// 3. read pixels from file

	if (width & 1) throw "ham image: odd row offset not supported";
	top_border = vga_mode.height; // prevent further display
	VideoController::getRef().addOneTimeAction([=, this]() { _setup(width, height, first_rel_code); });
}

void HamImageVideoPlane::teardown() noexcept { teardownScanlineRenderer<colormode_i8>(); }

void RAM HamImageVideoPlane::do_vblank(VideoPlane* vp) noexcept
{
	HamImageVideoPlane* me = reinterpret_cast<HamImageVideoPlane*>(vp);
	me->pixels			   = me->pixmap->pixmap;
	me->first_color		   = black;
}

void __force_inline XRAM HamImageVideoPlane::_render(int row, uint32* framebuffer) noexcept
{
	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we actually miss a scanline then let it be

	if (uint(row - top_border) < uint(image_height))
	{
		const Color*  first_rel_color = &colormap->colors[first_rel_code];
		Color		  current_color	  = first_color;
		const uint16* pixels		  = reinterpret_cast<const uint16*>(this->pixels);
		this->pixels += row_offset;

		Color* dest = reinterpret_cast<Color*>(framebuffer);
		for (int i = 0; i < left_border; i++) *dest++ = border_color;
		Color* first_pixel = dest;

		for (int i = 0; i < image_width / 4; i++)
		{
			const Color* color;

			interp_set_accumulator(interp0, lane0, uint(*pixels++) << 1);
			color	= next_color(interp0);
			*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;
			color					= next_color(interp0);
			*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;

			interp_set_accumulator(interp0, lane0, uint(*pixels++) << 1);
			color	= next_color(interp0);
			*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;
			color					= next_color(interp0);
			*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;
		}

		first_color = *first_pixel;
		for (int i = 0; i < right_border; i++) *dest++ = border_color;
	}
	else
	{
		Color* dest = reinterpret_cast<Color*>(framebuffer);
		for (int i = 0; i < vga_mode.width; i++) *dest++ = border_color;
	}
}

void XRAM HamImageVideoPlane::do_render(VideoPlane* vp, int row, uint32* fbu) noexcept
{
	HamImageVideoPlane* me = reinterpret_cast<HamImageVideoPlane*>(vp);
	me->_render(row, fbu);
}


} // namespace kio::Video


/*








































*/
