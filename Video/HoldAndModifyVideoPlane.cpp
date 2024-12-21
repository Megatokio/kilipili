// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HoldAndModifyVideoPlane.h"
#include "ScanlineRenderFu.h"
#include "VideoBackend.h"
#include <hardware/interp.h>


#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x." XWRAP(__LINE__))))	 // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical." XWRAP(__LINE__)))) // general ram


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


HoldAndModifyVideoPlane::HoldAndModifyVideoPlane(Pixmap* pm, const ColorMap cm, uint first_rel_code) :
	pixmap(pm),
	colormap(cm),
	first_rel_code(first_rel_code)
{}

void HoldAndModifyVideoPlane::setup(coord width)
{
	assert(width == vga_mode.width);

	vga_width	 = vga_mode.width;
	vga_height	 = vga_mode.height;
	image_width	 = pixmap->width & 0xfffc; // for renderScanline() optimizations
	image_height = pixmap->height;

	top_border	 = max(0, (vga_height - image_height) / 2);
	left_border	 = max(0, (vga_width - image_width) / 2) & 0xfffe;
	right_border = max(0, vga_width - image_width - left_border);

	setupScanlineRenderer<colormode_i8>(colormap);
	HoldAndModifyVideoPlane::vblank();
}

void HoldAndModifyVideoPlane::teardown() noexcept { teardownScanlineRenderer<colormode_i8>(); }

void RAM HoldAndModifyVideoPlane::vblank() noexcept
{
	pixels		= pixmap->pixmap;
	next_row	= 0;
	first_color = black;
}

inline Color operator+(Color a, Color b)
{
	return Color(a.raw + b.raw); //
}

void RAM HoldAndModifyVideoPlane::renderScanline(int current_row, uint32* framebuffer) noexcept
{
	// increment row and catch up when we missed some rows
	while (unlikely(++next_row <= current_row))
	{
		uint8 code	= *pixels;
		Color color = colormap[code];
		first_color = code >= first_rel_code ? first_color + color : color;
		if (next_row > top_border) pixels += pixmap->row_offset;
	}

	if (uint(current_row - top_border) < uint(image_height))
	{
		const Color*  first_rel_color = &colormap[first_rel_code];
		Color		  current_color	  = first_color;
		const uint16* pixels		  = reinterpret_cast<const uint16*>(this->pixels);
		this->pixels += pixmap->row_offset;

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
