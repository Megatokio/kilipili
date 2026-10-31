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


namespace kilipili
{


/* bits per color: (in the pixmap or attributes)
   bits/pixel = 1<<CD
   num colors = 1<<(1<<CD)
*/
enum ColorDepth : uint8 {
	colordepth_1bpp	 = 0,		// 1 bit/pixel probably indexed color
	colordepth_2bpp	 = 1,		// 2 bit/pixel probably indexed color
	colordepth_4bpp	 = 2,		// 4 bit/pixel probably indexed color
	colordepth_8bpp	 = 3,		// 8 bit/pixel probably indexed color
	colordepth_16bpp = 4,		// 16 bit/pixel always true color
	colordepth_32bpp = 5,		// 32 bit/pixel always true color
	colordepth_rgb	 = color_ss // colordepth_1bpp .. colordepth_32bpp
};


/* color attribute mode:
*/
enum AttrMode : uint8 {
	attrmode_1bpp = 0, // 1 bit / pixel => 2 colors / attribute
	attrmode_2bpp = 1, // 2 bit / pixel => 4 colors / attribute
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


/*
	enumeration of supported combinations of ColorDepth, AttrMode and AttrWidth:
*/
enum ColorMode : uint8 {
	// direct color modes store the color (or color index) in each pixel:
	colormode_i1,
	colormode_i2,
	colormode_i4,
	colormode_i8,
	colormode_rgb, // 1..32 bit true color

	// attribute modes:
	// see class Pixmap_wAttr for description.
	colormode_a1w1,
	colormode_a1w2,
	colormode_a1w4,
	colormode_a1w8,
	colormode_a2w1,
	colormode_a2w2,
	colormode_a2w4,
	colormode_a2w8,
};

constexpr uint num_colormodes = colormode_a2w8 + 1;


} // namespace kilipili


inline cstr tostr(kilipili::ColorDepth cd)
{
	static constexpr char id[][4] = {"i1", "i2", "i4", "i8", "i16", "i32"};
	return id[cd];
}

inline cstr tostr(kilipili::ColorMode cm)
{
	// clang-format off
	static constexpr char id[kilipili::num_colormodes][9] = {
		"i1", "i2", "i4", "i8", "rgb",		
		"a1w1", "a1w2", "a1w4", "a1w8", 
		"a2w1", "a2w2", "a2w4", "a2w8", 
	};
	// clang-format on
	return id[cm];
}


/*




























*/
