// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "kipili_cdefs.h"

/*	This file defines some enum types used in the Pixmap and graphics engine.
	- ColorDepth: enumeration for 1, 2, 4, 8 and 16 bit per pixel
	- AttrMode:   enumeration for direct pixels (no attributes) and 1 and 2 bit per pixel
	- AttrWidth:  enumeration for 1, 2, 4 and 8 pixel wide attribute cells
	- AttrHeight: enumeration for 1 to 16 pixel high attribute cells
	- ColorMode:  enumeration of the supported combinations of those

Info: What are attributes?
	It's the 'ZX Spectrum' way to disguise lack of memory while still providing colorful graphics.
	The Pixmap is divided into a grid of tiles for which each there is a colormap.
	These are the 'attributes'.
	The Pixmap itself uses 1 or 2 bit per pixel and accordingly the attributes contain 2 or 4 colors each.
	If you want to display text then you best match the attribute cell size with the character size.

	Of course there is nothing to substitute memory, except more memory.
	So what are the disadvantages?
	Whoever knew the ZX Spectrum knows there is a problem named 'color clash'.
	- If you draw a pink line through a b&w image then all 'set' pixels in the attribute cell become pink.
	This graphics library remedies this a little bit by that you can also use a 2 bit per pixel mode.
	- If you scroll the image then rows (or columns) of pixels move from one attribute cell to another.
	If you don't scroll with a multiple of the cell height or width then pixels get wrong colors.
	This graphics library remedies this a little bit by allowing 1 or 2 pixel wide or high cells.
	- A similar problem arises with 'software' sprites most prominently when they overlap each other.
	The video engine provides 'hardware' sprites which are independent of the frame buffer.
*/


namespace kio::Graphics
{


/* bits per color: (in the pixmap or attributes)
   bits/pixel = 1<<CD
   num colors = 1<<(1<<CD)
*/
enum ColorDepth : uint8 {
	colordepth_1bpp	 = 0, // 1 bit/pixel indexed color
	colordepth_2bpp	 = 1,
	colordepth_4bpp	 = 2,
	colordepth_8bpp	 = 3,
	colordepth_16bpp = 4, // 16 bit/pixel true color
};


/* color attribute mode:
*/
enum AttrMode : int8 {
	attrmode_none = -1, // no attributes: direct colors
	attrmode_1bpp = 0,	// 1 bit / pixel => 2 colors / attribute
	attrmode_2bpp = 1,	// 2 bit / pixel => 4 colors / attribute
};


/* width of color attribute cells:
*/
enum AttrWidth : uint8 {
	attrwidth_none = 0, // if used with attrmode_none
	attrwidth_1px  = 0, // 1<<0 = 1 pixel/attr
	attrwidth_2px  = 1, // 1<<1 = 2 pixel/attr
	attrwidth_4px  = 2,
	attrwidth_8px  = 3,
};


/* height of color attribute cells:
*/
enum AttrHeight : uint8 {
	attrheight_none = 0,
	attrheight_1px	= 1,
	attrheight_2px,
	attrheight_3px,
	attrheight_4px,
	attrheight_5px,
	attrheight_6px,
	attrheight_7px,
	attrheight_8px,
	attrheight_9px,
	attrheight_10px,
	attrheight_11px,
	attrheight_12px,
	attrheight_13px,
	attrheight_14px,
	attrheight_15px,
	attrheight_16px,
	// etc.
};


/* supported combinations of ColorDepth, AttrMode and AttrWidth:
*/
enum ColorMode : uint8 {
	// direct color modes store the color for each pixel:
	// this may be an indexed color thou.
	colormode_i1,
	colormode_i2,
	colormode_i4,
	colormode_i8,
	colormode_rgb,

	// color attributes assign colors to a rectangular cell of pixels.
	// color attributes are used to disguise the fact that we don't have enough memory
	// to store an individual color for each pixel.
	// then the pixels are still high-res but the color isn't.
	// the pixels can be 1 bit or 2 bit per pixel, allowing 2 or 4 colors per attribute cell.
	// attribute modes are cpu intensive; the narrower the cell the higher the required system clock!

	// attribute width 1 pixel to best support proportional text and horizontal scrollers:
	// very cpu intensive! not recommended!
	colormode_a1w1_i4,	// 1 bit/pixel, 1 pixel/attr, 4 bit indexed colors
	colormode_a1w1_i8,	// 1 bit/pixel, 1 pixel/attr, 8 bit indexed colors
	colormode_a1w1_rgb, // 1 bit/pixel, 1 pixel/attr, 16 bit true color

	// attribute width 2 pixel to support proportional text and horizontal scrollers:
	// very cpu intensive!
	colormode_a1w2_i4,	// 1 bit/pixel, 2 pixel/attr, 4 bit indexed colors
	colormode_a1w2_i8,	// 1 bit/pixel, 2 pixel/attr, 8 bit indexed colors
	colormode_a1w2_rgb, // 1 bit/pixel, 2 pixel/attr, 16 bit true color

	// attribute width 4 pixel still allows good horizontal scrolling at 4 pixel/frame:
	// very cpu intensive!
	colormode_a1w4_i4,	// 1 bit/pixel, 4 pixel/attr, 4 bit indexed colors
	colormode_a1w4_i8,	// 1 bit/pixel, 4 pixel/attr, 8 bit indexed colors
	colormode_a1w4_rgb, // 1 bit/pixel, 4 pixel/attr, 16 bit true color

	// attribute width 8 pixel matches character cell width of 8x12 pixel terminal font:
	// the ZX Spectrum used 8x8 pixel attribute cells.
	// THESE ARE THE RECOMMENDED ATTRIBUTE MODES!
	colormode_a1w8_i4,	// 1 bit/pixel, 8 pixel/attr, 4 bit indexed colors
	colormode_a1w8_i8,	// 1 bit/pixel, 8 pixel/attr, 8 bit indexed colors
	colormode_a1w8_rgb, // 1 bit/pixel, 8 pixel/attr, 16 bit true color

	// attributes width 2 bit per pixel to provide attribute cells with a total of 4 different colors:
	// very cpu intensive!

	colormode_a2w1_i4,	// 2 bit/pixel, 1 pixel/attr, 4 bit indexed colors
	colormode_a2w1_i8,	// 2 bit/pixel, 1 pixel/attr, 8 bit indexed colors
	colormode_a2w1_rgb, // 2 bit/pixel, 1 pixel/attr, 16 bit true color

	colormode_a2w2_i4,	// 2 bit/pixel, 2 pixel/attr, 4 bit indexed colors
	colormode_a2w2_i8,	// 2 bit/pixel, 2 pixel/attr, 8 bit indexed colors
	colormode_a2w2_rgb, // 2 bit/pixel, 2 pixel/attr, 16 bit true color

	colormode_a2w4_i4,	// 2 bit/pixel, 4 pixel/attr, 4 bit indexed colors
	colormode_a2w4_i8,	// 2 bit/pixel, 4 pixel/attr, 8 bit indexed colors
	colormode_a2w4_rgb, // 2 bit/pixel, 4 pixel/attr, 16 bit true color

	colormode_a2w8_i4,	// 2 bit/pixel, 8 pixel/attr, 4 bit indexed colors
	colormode_a2w8_i8,	// 2 bit/pixel, 8 pixel/attr, 8 bit indexed colors
	colormode_a2w8_rgb, // 2 bit/pixel, 8 pixel/attr, 16 bit true color
};

constexpr uint num_colormodes = colormode_a2w8_rgb + 1;


constexpr ColorMode calc_colormode(AttrMode am, AttrWidth aw, ColorDepth cd) noexcept
{
	return am == attrmode_none ? ColorMode(cd) :
								 ColorMode(colormode_a1w1_i4 + am * 3 * 4 + aw * 3 + cd - colordepth_4bpp);
}

constexpr AttrMode get_attrmode(ColorMode cm) noexcept
{
	return cm <= colormode_rgb ? attrmode_none : cm <= colormode_a1w8_rgb ? attrmode_1bpp : attrmode_2bpp;
}

constexpr AttrWidth get_attrwidth(ColorMode cm) noexcept
{
	return cm <= colormode_rgb ? attrwidth_none : AttrWidth(((cm - colormode_a1w1_i4) / 3) & 3);
}

constexpr ColorDepth get_colordepth(ColorMode cm) noexcept
{
	return cm <= colormode_rgb ? ColorDepth(cm) : ColorDepth(colordepth_4bpp + (cm - colormode_i4) % 3);
}

constexpr bool is_direct_color(ColorMode cm) noexcept { return cm <= colormode_rgb; }
constexpr bool is_indexed_color(ColorDepth cd) noexcept { return cd != colordepth_16bpp; }
constexpr bool is_indexed_color(ColorMode cm) noexcept { return is_indexed_color(get_colordepth(cm)); }
constexpr bool is_true_color(ColorMode cm) noexcept { return !is_indexed_color(cm); }
constexpr bool is_attribute_mode(ColorMode cm) noexcept { return !is_direct_color(cm); }

inline void split_colormode(ColorMode cm, AttrMode* am, AttrWidth* aw, ColorDepth* cd) noexcept
{
	*am = get_attrmode(cm);
	*aw = get_attrwidth(cm);
	*cd = get_colordepth(cm);
	assert(cm == calc_colormode(*am, *aw, *cd));
}

static_assert(calc_colormode(attrmode_none, attrwidth_none, colordepth_2bpp) == colormode_i2);
static_assert(calc_colormode(attrmode_1bpp, attrwidth_1px, colordepth_4bpp) == colormode_a1w1_i4);
static_assert(calc_colormode(attrmode_1bpp, attrwidth_2px, colordepth_4bpp) == colormode_a1w2_i4);
static_assert(calc_colormode(attrmode_1bpp, attrwidth_2px, colordepth_8bpp) == colormode_a1w2_i8);
static_assert(calc_colormode(attrmode_2bpp, attrwidth_2px, colordepth_8bpp) == colormode_a2w2_i8);

static_assert(get_attrmode(colormode_i4) == attrmode_none);
static_assert(get_attrmode(colormode_a1w1_i4) == attrmode_1bpp);
static_assert(get_attrmode(colormode_a2w4_i4) == attrmode_2bpp);
static_assert(get_attrmode(colormode_a1w1_rgb) == attrmode_1bpp);

static_assert(get_attrwidth(colormode_i4) == attrwidth_none);
static_assert(get_attrwidth(colormode_a1w1_i4) == attrwidth_1px);
static_assert(get_attrwidth(colormode_a1w4_i4) == attrwidth_4px);
static_assert(get_attrwidth(colormode_a1w4_i8) == attrwidth_4px);
static_assert(get_attrwidth(colormode_a2w4_rgb) == attrwidth_4px);

static_assert(get_colordepth(colormode_rgb) == colordepth_16bpp);
static_assert(get_colordepth(colormode_a1w1_i4) == colordepth_4bpp);
static_assert(get_colordepth(colormode_a1w4_i4) == colordepth_4bpp);
static_assert(get_colordepth(colormode_a1w4_i8) == colordepth_8bpp);

} // namespace kio::Graphics


inline cstr tostr(kio::Graphics::ColorDepth cd)
{
	static constexpr char id[][4] = {"i1", "i2", "i4", "i8", "rgb"};
	return id[cd];
}
