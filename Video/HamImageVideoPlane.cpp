// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HamImageVideoPlane.h"
#include "ScanlineRenderFu.h"
#include "VideoBackend.h"
#include "VideoController.h"
#include "common/cdefs.h"
#include <hardware/interp.h>

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define RAM		 __attribute__((section(".time_critical." XWRAP(__LINE__)))) // general ram
#if !defined VIDEO_HAM_RENDER_FUNCTION_IN_XRAM || VIDEO_HAM_RENDER_FUNCTION_IN_XRAM
  #define XRAM __attribute__((section(".scratch_x." XWRAP(__LINE__)))) // the 4k page with the core1 stack
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

static inline const Color* next_color(interp_hw_t* interp)
{
	return reinterpret_cast<const Color*>(interp_pop_lane_result(interp, lane1));
}


HamImageVideoPlane::HamImageVideoPlane(const Pixmap* pm, const ColorMap* cm, uint first_rel_code) :
	pixmap(pm),
	colormap(cm),
	first_rel_code(first_rel_code)
{
	if (pm->width & 1) throw "ham image: odd row offset not supported";
}

void HamImageVideoPlane::_setup(int w, int h, uint n)
{
	first_rel_code = n;
	image_width	   = w & 0xfffc; // for renderScanline() optimizations
	image_height   = h;
	row_offset	   = w;

	top_border	 = max(0, (vga_height - h) / 2);
	left_border	 = max(0, (vga_width - w) / 2) & 0xfffe;
	right_border = max(0, vga_width - w - left_border);
}

void HamImageVideoPlane::setup(coord width)
{
	assert_eq(width, vga_mode.width);

	vga_width  = vga_mode.width;
	vga_height = vga_mode.height;

	_setup(pixmap->width, pixmap->height, first_rel_code);

	setupScanlineRenderer<colormode_i8>(colormap->colors);
	HamImageVideoPlane::vblank();
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
	top_border = vga_height; // prevent further display
	VideoController::getRef().addOneTimeAction([=, this]() { _setup(width, height, first_rel_code); });
}

void HamImageVideoPlane::teardown() noexcept { teardownScanlineRenderer<colormode_i8>(); }

void RAM HamImageVideoPlane::vblank() noexcept
{
	pixels		= pixmap->pixmap;
	next_row	= 0;
	first_color = black;
}

inline Color operator+(Color a, Color b)
{
	return Color(a.raw + b.raw); //
}

void XRAM HamImageVideoPlane::renderScanline(int current_row, uint32* framebuffer) noexcept
{
	// increment row and catch up when we missed some rows
	while (unlikely(++next_row <= current_row))
	{
		uint8 code	= *pixels;
		Color color = colormap->colors[code];
		first_color = code >= first_rel_code ? first_color + color : color;
		if (next_row > top_border) pixels += row_offset;
	}

	if (uint(current_row - top_border) < uint(image_height))
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
		for (int i = 0; i < vga_width; i++) *dest++ = border_color;
	}
}


} // namespace kio::Video


/*








































*/
