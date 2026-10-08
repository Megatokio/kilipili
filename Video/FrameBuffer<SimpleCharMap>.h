// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FrameBuffer.h"
#include "Graphics/Color.h"
#include "Graphics/SimpleCharMap.h"

namespace kilipili::Video
{
using Color			= Graphics::Color;
using SimpleCharMap = Graphics::SimpleCharMap;


/*	template class CharacterFrameBuffer is the bare minimum character-based FrameBuffer:
	It supports only one foreground and one background color. No attributes!
	Characters are expected to be 8 pixels wide and 12 pixels high.
	All character codes incl. 0x00 are available.

	The class contains precalculated strips of all 4 or 8 pixel combinations:
	Flag = Small: strips of 4 pixels x 16 versions = 64 Colors   ~ 5cc per pixel
	Flag = Fast:  strips of 8 pixels x 256 versions = 2k Colors  ~ 4cc per pixel

	Inverted colors (or bold characters) may be achieved by modifying character codes 128++.
	--> latin1_256x12[][]  or
	--> ascii_256x12_inverse[][]
*/
using Fast	= int;
using Small = void; // => this is the default

template<typename Flag>
class FrameBuffer<SimpleCharMap, Flag> final : public VideoPlane
{
public:
	static constexpr bool small = std::is_same<Flag, Small>();

	FrameBuffer(SimpleCharMap* charmap) noexcept;
	NO_COPY_MOVE(FrameBuffer);

	static constexpr int char_width = 8;

	RCPtr<SimpleCharMap>		charmap;
	RCPtr<const Graphics::Font> font;

	using ColorStrip = Color[small ? 4 : 8];
	ColorStrip __aligned(4) colorstrips[small ? 16 : 256];

	const uchar* row_ptr	 = nullptr; // charmap row
	int			 raster_line = 0;		// line within character
	int			 last_y		 = 0;
	const uint8* cursor_ptr	 = nullptr;

private:
	static void do_vblank(VideoPlane*) noexcept;
	static void do_render(VideoPlane*, int row, int width, uint32* buffer) noexcept;
};


//
// *****************************************************************************
//					I M P L E M E N T A T I O N S
// *****************************************************************************
//

template<typename Flag>
FrameBuffer<SimpleCharMap, Flag>::FrameBuffer(SimpleCharMap* charmap) noexcept :
	charmap(charmap),
	font(charmap->font1),
	VideoPlane(&do_vblank, &do_render)
{
	assert(font->char_width == 8);
	assert(font->first_glyph == 0);

	// setup_colors in first vblank:
	charmap->bgcolor = ~colorstrips[0][0];
}

template<typename Flag>
void FrameBuffer<SimpleCharMap, Flag>::do_vblank(VideoPlane* vp) noexcept
{
	auto* me = static_cast<FrameBuffer<SimpleCharMap, Flag>*>(vp);

	SimpleCharMap* charmap = me->charmap;

	me->row_ptr		= charmap->data;
	me->raster_line = 0;
	me->last_y		= 0;
	me->cursor_ptr	= charmap->cursor_ptr + 1;
	me->font		= charmap->font1;

	// assert(me->font->char_width == 8);
	// assert(me->font->first_glyph == 0);

	if (charmap->bgcolor != me->colorstrips[0][0] || charmap->fgcolor != me->colorstrips[NELEM(colorstrips) - 1][0])
	{
		// setup_colors:
		Color  colors[2] = {charmap->bgcolor, charmap->fgcolor};
		Color* p		 = &me->colorstrips[0][0];
		for (int byte = 0; byte < (small ? 16 : 256); byte++)
		{
			for (int bit = 0; bit < (small ? 4 : 8); bit++) { *p++ = colors[(byte >> bit) & 1]; }
		}
	}
}

template<typename Flag>
void FrameBuffer<SimpleCharMap, Flag>::do_render(VideoPlane* vp, int y, int width, uint32* buffer) noexcept
{
	auto* me = static_cast<FrameBuffer<SimpleCharMap, Flag>*>(vp);

	static_assert(char_width == 8);
	assert_lt(y, me->charmap->rows * me->font->char_height);
	assert_le(width, me->charmap->cols * 8);
	assert(width % 8 == 0);

	while (me->last_y < y)
	{
		me->last_y++;
		if (++me->raster_line >= me->font->char_height)
		{
			me->raster_line = 0;
			me->row_ptr += me->charmap->cols;
		}
	}

	int		cols = width >> 3;												// if char_width = 8
	cuptr	font = me->font->data + me->raster_line * me->font->row_offset; // if first_char = 0
	uint32* z	 = buffer;

	for (cuptr p = me->row_ptr, end = p + cols; p < end;)
	{
		int byte = font[*p++];
		if unlikely (p == me->cursor_ptr) byte = ~byte;

		if (byte)
		{
			if constexpr (small)
			{
				uint32* colors = reinterpret_cast<uint32*>(me->colorstrips[byte & 0x0f]);

				*z++ = colors[0];
				if (sizeof(Color) >= 2) *z++ = colors[1];
				if (sizeof(Color) == 4) *z++ = colors[2];
				if (sizeof(Color) == 4) *z++ = colors[3];

				colors = reinterpret_cast<uint32*>(me->colorstrips[byte >> 4]);

				*z++ = colors[0];
				if (sizeof(Color) >= 2) *z++ = colors[1];
				if (sizeof(Color) == 4) *z++ = colors[2];
				if (sizeof(Color) == 4) *z++ = colors[3];
			}
			else // fast:
			{
				uint32* colors = reinterpret_cast<uint32*>(me->colorstrips[byte]);

				*z++ = colors[0];
				*z++ = colors[1];
				if (sizeof(Color) >= 2) *z++ = colors[2];
				if (sizeof(Color) >= 2) *z++ = colors[3];
				if (sizeof(Color) == 4) *z++ = colors[4];
				if (sizeof(Color) == 4) *z++ = colors[5];
				if (sizeof(Color) == 4) *z++ = colors[6];
				if (sizeof(Color) == 4) *z++ = colors[7];
			}
		}
		else // byte = 0x00
		{
			uint32 bg = *reinterpret_cast<uint32*>(me->colorstrips[0]);

			*z++ = bg;
			*z++ = bg;
			if (sizeof(Color) >= 2) *z++ = bg;
			if (sizeof(Color) >= 2) *z++ = bg;
			if (sizeof(Color) == 4) *z++ = bg;
			if (sizeof(Color) == 4) *z++ = bg;
			if (sizeof(Color) == 4) *z++ = bg;
			if (sizeof(Color) == 4) *z++ = bg;
		}
	}
}

// rant:
// gcc ignores attributes in templates!
// we must specify the section in every instantiation!
// we cannot just instantiate the class,
// we must instantiate every single function!
// at least there are only 2 versions of the SimpleCharMap...

#ifndef VIDEO_SCANLINE_RENDERER_SECTION
  #define VIDEO_SCANLINE_RENDERER_SECTION XRAM
#endif

template void __section(RAM ".scfb") FrameBuffer<SimpleCharMap, Fast>::do_vblank(VideoPlane*) noexcept;
template void __section(VIDEO_SCANLINE_RENDERER_SECTION
						".scfb") FrameBuffer<SimpleCharMap, Fast>::do_render(VideoPlane*, int, int, uint32*) noexcept;

template void __section(RAM ".scfb") FrameBuffer<SimpleCharMap, Small>::do_vblank(VideoPlane*) noexcept;
template void __section(VIDEO_SCANLINE_RENDERER_SECTION
						".scfb") FrameBuffer<SimpleCharMap, Small>::do_render(VideoPlane*, int, int, uint32*) noexcept;


} // namespace kilipili::Video


/*





































*/
