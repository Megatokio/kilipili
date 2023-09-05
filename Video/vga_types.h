// Copyright (c) 2022 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Scanvideo.h"
#include "graphics_types.h"


namespace kipili::Video
{

using ColorDepth = Graphics::ColorDepth;	// bits per color (in the pixmap or attributes)
using AttrMode = Graphics::AttrMode;		// color attribute mode
using AttrWidth = Graphics::AttrWidth;		// width of color attribute cells
using AttrHeight = Graphics::AttrHeight;  	// height of color attribute cells
using ColorMode = Graphics::ColorMode;		// combines ColorDepth, AttrMode and AttrWidth

struct VideoMode
{
	ScreenSize screensize : 3;
	ColorDepth colordepth   : 3;
	AttrMode   attrmode     : 2;
	AttrWidth  attrwidth    : 2;
	AttrHeight attrheight   : 6;

	constexpr VideoMode() = default;

	constexpr VideoMode(ScreenSize ss, ColorDepth cd, AttrMode am = AttrMode::attrmode_none,
						AttrWidth aw = AttrWidth::attrwidth_none, AttrHeight ah = AttrHeight::attrheight_none) :
		screensize(ss),colordepth(cd),attrmode(am),attrwidth(aw),attrheight(ah)
	{
		assert((am?1:!aw) && !am == !ah); // note: aw_none == aw_1px therefore only 'half way' test of aw
	}

	constexpr VideoMode(ScreenSize ss, ColorMode cm, AttrHeight ah = AttrHeight::attrheight_none) :
		VideoMode(ss, get_colordepth(cm), get_attrmode(cm), get_attrwidth(cm), ah)
	{
		assert((attrmode==AttrMode::attrmode_none) == (ah==AttrHeight::attrheight_none));
	}

	constexpr ColorMode get_colormode() const noexcept { return calc_colormode(attrmode, attrwidth, colordepth); }
	constexpr uint get_bitsperpixel() const noexcept { return attrmode==AttrMode::attrmode_none ? 1 << colordepth : 1 << attrmode; }
};

static_assert (sizeof(VideoMode) == 2);


} // namespace





















