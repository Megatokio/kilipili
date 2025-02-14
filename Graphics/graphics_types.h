// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "basic_math.h"
#include "standard_types.h"

/*	This file defines some enum types used in the Pixmap and graphics engine.
	- ColorDepth: enumeration for 1, 2, 4, 8 and 16 bit per pixel
	- AttrMode:   enumeration for direct pixels (no attributes) and 1 and 2 bit per pixel
	- AttrWidth:  enumeration for 1, 2, 4 and 8 pixel wide attribute cells
	- AttrHeight: enumeration for 1 to 16 pixel high attribute cells
	- ColorMode:  enumeration of the supported combinations of those

Info: What are attributes?
	It's the 'ZX Spectrum' way to disguise lack of memory while still providing colorful graphics.
	The Pixmap is divided into a grid of tiles each with a little colormap with 2 or 4 colors.
	These are the 'attributes'.
	The Pixmap itself uses 1 or 2 bit per pixel and accordingly the attributes have 2 or 4 colors.
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
	colordepth_1bpp	 = 0,									// 1 bit/pixel probably indexed color
	colordepth_2bpp	 = 1,									// 2 bit/pixel probably indexed color
	colordepth_4bpp	 = 2,									// 4 bit/pixel probably indexed color
	colordepth_8bpp	 = 3,									// 8 bit/pixel probably indexed color
	colordepth_16bpp = 4,									// 16 bit/pixel always true color
	colordepth_rgb	 = msbit(VIDEO_COLOR_PIN_COUNT * 2 - 1) // colordepth_1bpp .. colordepth_16bpp
};


/* color attribute mode:
*/
enum AttrMode : int8 {
	attrmode_none = -1, // no attributes: direct colors
	attrmode_1bpp = 0,	// 1 bit / pixel => 2 colors / attribute
	attrmode_2bpp = 1,	// 2 bit / pixel => 4 colors / attribute
};


/* width of attribute cells:
*/
enum AttrWidth : uint8 {
	attrwidth_none = 0, // if used with attrmode_none
	attrwidth_1px  = 0, // 1<<0 = 1 pixel/attr
	attrwidth_2px  = 1, // 1<<1 = 2 pixel/attr
	attrwidth_4px  = 2, // 1<<2 = 4 pixel/attr
	attrwidth_8px  = 3, // 1<<3 = 8 pixel/attr
};


/* height of attribute cells:
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
	colormode_rgb, // 1..16 bit true color

	// color attributes assign colors to a rectangular cell of pixels.
	// color attributes are used to disguise the fact that we don't have enough memory
	// to store an individual color for each pixel.
	// then the pixels are still high-res but the color isn't.
	// the pixels can be 1 bit or 2 bit per pixel, allowing 2 or 4 colors per attribute cell.

	// attribute mode wth 1 bit per pixel to provide 2 colors per attribute cell:
	// attribute width 1..4 pixel may be used for proportional text and horizontal scrolling.
	colormode_a1w1, // 1 bit/pixel, 1 pixel/attr, 4..16 bit true color. very cpu intensive! deprecated!
	colormode_a1w2, // 1 bit/pixel, 2 pixel/attr, 4..16 bit true color. very cpu intensive!
	colormode_a1w4, // 1 bit/pixel, 4 pixel/attr, 4..16 bit true color. very cpu intensive!
	colormode_a1w8, // 1 bit/pixel, 8 pixel/attr, 4..16 bit true color. THIS IS THE RECOMMENDED ATTRIBUTE MODE!

	// attribute mode wth 2 bit per pixel to provide 4 colors per attribute cell:
	colormode_a2w1, // 2 bit/pixel, 1 pixel/attr, 4..16 bit true color. very cpu intensive!
	colormode_a2w2, // 2 bit/pixel, 2 pixel/attr, 4..16 bit true color. very cpu intensive!
	colormode_a2w4, // 2 bit/pixel, 4 pixel/attr, 4..16 bit true color. very cpu intensive!
	colormode_a2w8, // 2 bit/pixel, 8 pixel/attr, 4..16 bit true color. very cpu intensive!
};

constexpr uint		num_colormodes	   = colormode_a2w8 + 1;
constexpr ColorMode colormode_a1w8_rgb = colormode_a1w8; // old name


constexpr AttrMode get_attrmode(ColorMode cm) noexcept
{
	return cm <= colormode_rgb ? attrmode_none : cm <= colormode_a1w8 ? attrmode_1bpp : attrmode_2bpp;
}

constexpr AttrWidth get_attrwidth(ColorMode cm) noexcept
{
	return cm <= colormode_rgb ? attrwidth_none : AttrWidth((cm - colormode_a1w1) & 3);
}

constexpr ColorDepth get_colordepth(ColorMode cm) noexcept
{
	return cm < colormode_rgb		  ? ColorDepth(cm) :
		   cm == colormode_rgb		  ? colordepth_rgb :
		   VIDEO_COLOR_PIN_COUNT <= 4 ? colordepth_4bpp : // min. bits in attr
										colordepth_rgb;
}

constexpr bool is_direct_color(ColorMode cm) noexcept { return cm <= colormode_rgb; }
constexpr bool is_attribute_mode(ColorMode cm) noexcept { return cm > colormode_rgb; }
constexpr bool is_true_color(ColorMode cm) noexcept { return cm >= colormode_rgb; }
constexpr bool is_indexed_color(ColorMode cm) noexcept { return cm < colormode_rgb; }


static_assert(get_attrmode(colormode_i4) == attrmode_none);
static_assert(get_attrmode(colormode_a1w1) == attrmode_1bpp);
static_assert(get_attrmode(colormode_a2w4) == attrmode_2bpp);
static_assert(get_attrmode(colormode_a1w8) == attrmode_1bpp);

static_assert(get_attrwidth(colormode_i4) == attrwidth_none);
static_assert(get_attrwidth(colormode_rgb) == attrwidth_none);
static_assert(get_attrwidth(colormode_a1w1) == attrwidth_1px);
static_assert(get_attrwidth(colormode_a1w4) == attrwidth_4px);
static_assert(get_attrwidth(colormode_a2w4) == attrwidth_4px);
static_assert(get_attrwidth(colormode_a2w8) == attrwidth_8px);

static_assert(get_colordepth(colormode_i1) == colordepth_1bpp);
static_assert(get_colordepth(colormode_i8) == colordepth_8bpp);
static_assert(get_colordepth(colormode_rgb) == colordepth_rgb);
static_assert(get_colordepth(colormode_a1w1) == VIDEO_COLOR_PIN_COUNT < 4 ? colordepth_4bpp : colordepth_rgb);
static_assert(get_colordepth(colormode_a2w4) == VIDEO_COLOR_PIN_COUNT < 4 ? colordepth_4bpp : colordepth_rgb);
static_assert(get_colordepth(colormode_a1w8) == VIDEO_COLOR_PIN_COUNT < 4 ? colordepth_4bpp : colordepth_rgb);

} // namespace kio::Graphics


inline cstr tostr(kio::Graphics::ColorDepth cd)
{
	static constexpr char id[][4] = {"i1", "i2", "i4", "i8", "i16"};
	return id[cd];
}

inline cstr tostr(kio::Graphics::ColorMode cm)
{
	// clang-format off
	static constexpr char id[kio::Graphics::num_colormodes][9] = {
		"i1", "i2", "i4", "i8", "rgb",		
		"a1w1", "a1w2", "a1w4", "a1w8", 
		"a2w1", "a2w2", "a2w4", "a2w8", 
	};
	// clang-format on
	return id[cm];
}


/*




























*/
