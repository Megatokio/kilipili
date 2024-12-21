// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HoldAndModifyVideoPlane.h"
#include "ScanlineRenderFu.h"
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
	this->width = width;
	setupScanlineRenderer<colormode_i8>(colormap);
	HoldAndModifyVideoPlane::vblank();
}

void HoldAndModifyVideoPlane::teardown() noexcept { teardownScanlineRenderer<colormode_i8>(); }

void RAM HoldAndModifyVideoPlane::vblank() noexcept
{
	pixels		= pixmap->pixmap;
	row			= 0;
	first_color = black;
}

inline Color operator+(Color a, Color b)
{
	return Color(a.raw + b.raw); //
}

void RAM HoldAndModifyVideoPlane::renderScanline(int row, uint32* framebuffer) noexcept
{
	// increment row and catch up when we missed some rows
	while (unlikely(++this->row <= row))
	{
		uint8 code	= *pixels;
		Color color = colormap[code];
		first_color = code >= first_rel_code ? first_color + color : color;
		pixels += pixmap->row_offset;
	}

	if constexpr (0)
	{
		const uint8* pixels		   = this->pixels;
		Color*		 dest		   = reinterpret_cast<Color*>(framebuffer);
		Color		 current_color = first_color;
		const Color* colormap	   = this->colormap;

		for (int i = 0; i < width / 4; i++)
		{
			Color color;
			uint8 code;

			code	= *pixels++;
			color	= colormap[code];
			*dest++ = current_color = code >= first_rel_code ? current_color + color : color;

			code	= *pixels++;
			color	= colormap[code];
			*dest++ = current_color = code >= first_rel_code ? current_color + color : color;

			code	= *pixels++;
			color	= colormap[code];
			*dest++ = current_color = code >= first_rel_code ? current_color + color : color;

			code	= *pixels++;
			color	= colormap[code];
			*dest++ = current_color = code >= first_rel_code ? current_color + color : color;
		}
	}
	else
	{
		const uint16* pixels = reinterpret_cast<const uint16*>(this->pixels);
		uint16*		  dest	 = reinterpret_cast<uint16*>(framebuffer);

		const Color* first_rel_color = &colormap[first_rel_code];
		Color		 current_color	 = first_color;

		for (int i = 0; i < width / 4; i++)
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
	}

	pixels += pixmap->row_offset;
	first_color = *reinterpret_cast<Color*>(framebuffer);
}


} // namespace kio::Video


/*








































*/
