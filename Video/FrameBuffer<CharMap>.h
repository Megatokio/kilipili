// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FrameBuffer.h"
#include "Graphics/Color.h"
#include "VideoPlane.h"


namespace kilipili::Video
{
using Color = Graphics::Color;


/*	Implementation of template `FrameBuffer<>` for template character array `CharMap<>`.

	template class FrameBuffer<CharMap> uses one attribute per character.
	The attribute follows immediately after the character code.

	Attributes are configurable per template arguments to `CharMap<>`:
	template parameters:
		fgbits:    foreground color bits
		bgbits:    background color bits
		bold:      enable bold attribute
		underline: enable underline attribute
	The total number of bits must not exceed 8.
*/

template<class CharMap>
class FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>> final : public VideoPlane
{
	static constexpr int   fg_bits		  = CharMap::fg_bits;
	static constexpr int   bg_bits		  = CharMap::bg_bits;
	static constexpr bool  bold			  = CharMap::bold;
	static constexpr bool  underline	  = CharMap::underline;
	static constexpr int   fg_ss		  = CharMap::fg_ss;
	static constexpr int   bg_ss		  = CharMap::bg_ss;
	static constexpr uchar fg_mask		  = CharMap::fg_mask;
	static constexpr uchar bg_mask		  = CharMap::bg_mask;
	static constexpr uchar bold_mask	  = CharMap::bold_mask;
	static constexpr uchar underline_mask = CharMap::underline_mask;

	static_assert(fg_bits + bg_bits + bold + underline <= 8);
	static_assert(uint(fg_bits <= 8));
	static_assert(uint(bg_bits <= 8));

public:
	FrameBuffer(CharMap*) noexcept;
	NO_COPY_MOVE(FrameBuffer);

	static constexpr int char_width	 = 8;
	static constexpr int char_height = 12;

	RCPtr<CharMap> charmap;

	const uchar* font		 = nullptr;
	const uchar* row_ptr	 = nullptr; // charmap row
	int			 raster_line = 0;		// line within character
	int			 last_y		 = 0;

	uint32 fgcolors[1 << fg_bits]; // floodfilled color
	uint32 bgcolors[1 << bg_bits];

private:
	void		setup_colors() noexcept;
	static void do_vblank(VideoPlane*) noexcept;
	static void do_render(VideoPlane*, int row, int width, uint32* buffer) noexcept;
};


//
// *****************************************************************************
//					I M P L E M E N T A T I O N S
// *****************************************************************************
//

#define XRAM __attribute__((section(".scratch_x.CFBuA" __XSTRING(__LINE__))))	  // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.CFBuA" __XSTRING(__LINE__)))) // general ram

// all permutations of mask for 4 1-char Colors in a uint32:
static constexpr XRAM uint32 mask16x1[16] = {
	0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, //
	0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff, //
	0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff, //
	0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,
};

// all permutations of mask for 2 2-char Colors in a uint32:
static constexpr XRAM uint32 mask4x2[4] = //
	{0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff};


template<class CharMap>
FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::FrameBuffer(CharMap* charmap) noexcept :
	VideoPlane(&do_vblank, &do_render),
	charmap(charmap)
{
	//setup_colors();
}

template<class CharMap>
inline void RAM FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::setup_colors() noexcept
{
	constexpr uint32 mul = sizeof(Color) == 1 ? 0x01010101 : sizeof(Color) == 2 ? 0x00010001 : 1;

	for (int i = 0; i < 1 << bg_bits; i++) { bgcolors[i] = charmap->bgcolors[i] * mul; }
	for (int i = 0; i < 1 << fg_bits; i++) { fgcolors[i] = charmap->fgcolors[i] * mul; }
}

template<class CharMap>
void XRAM FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::do_vblank(VideoPlane* vp) noexcept
{
	auto* me		= static_cast<FrameBuffer<CharMap>*>(vp);
	me->raster_line = 0;
	me->last_y		= 0;
	me->row_ptr		= cuptr(me->charmap->data);
	me->font		= me->charmap->font; // ATTN: this may be in flash!

	me->setup_colors();
}

template<class CharMap>
void RAM FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::do_render( //
	VideoPlane* vp, int y, int width, uint32* buffer) noexcept
{
	auto* me = static_cast<FrameBuffer<CharMap>*>(vp);

	assert(char_width == 8);
	assert(char_height == 12);
	assert(y < me->charmap->rows * char_height);
	assert_le(width, me->charmap->cols * char_width);
	assert(width % char_width == 0);

	while (me->last_y < y)
	{
		me->last_y++;
		if (++me->raster_line >= char_height)
		{
			me->raster_line = 0;
			me->row_ptr += me->charmap->cols * 2;
		}
	}

	int			 cols = width >> 3;						  // assumes char_width = 8
	const uchar* font = me->font + me->raster_line * 256; // TODO: this assumes 256 glyphs in font

	for (cuptr p = me->row_ptr, end = p + 2 * cols; p < end;)
	{
		uchar byte = font[*p++];
		int	  attr = *p++;
		if ((attr & underline_mask) && me->raster_line == 10) { byte = 0xff; }
		uint32 bg = me->bgcolors[(attr & bg_mask) >> bg_ss];

		if (byte)
		{
			if (attr & bold_mask) byte |= byte << 1;
			uint32 fg = me->fgcolors[(attr & fg_mask) >> fg_ss];
			if constexpr (sizeof(Color) <= 2) fg = fg ^ bg; // xor instead of fg color

			if constexpr (sizeof(Color) == 1)
			{
				*buffer++ = bg ^ (fg & mask16x1[byte & 0x0f]);
				*buffer++ = bg ^ (fg & mask16x1[byte >> 4]);
			}
			else if constexpr (sizeof(Color) == 2)
			{
				*buffer++ = bg ^ (fg & mask4x2[byte & 3]);
				*buffer++ = bg ^ (fg & mask4x2[(byte >> 2) & 3]);
				*buffer++ = bg ^ (fg & mask4x2[(byte >> 4) & 3]);
				*buffer++ = bg ^ (fg & mask4x2[byte >> 6]);
			}
			else // Color = 4 bytes
			{
				*buffer++ = byte & 1 ? fg : bg;
				*buffer++ = byte & 2 ? fg : bg;
				*buffer++ = byte & 4 ? fg : bg;
				*buffer++ = byte & 8 ? fg : bg;
				*buffer++ = byte & 0x10 ? fg : bg;
				*buffer++ = byte & 0x20 ? fg : bg;
				*buffer++ = byte & 0x40 ? fg : bg;
				*buffer++ = byte & 0x80 ? fg : bg;
			}
		}
		else // byte = 0x00
		{
			*buffer++ = bg;
			*buffer++ = bg;

			if constexpr (sizeof(Color) >= 2) *buffer++ = bg;
			if constexpr (sizeof(Color) >= 2) *buffer++ = bg;

			if constexpr (sizeof(Color) == 4) *buffer++ = bg;
			if constexpr (sizeof(Color) == 4) *buffer++ = bg;
			if constexpr (sizeof(Color) == 4) *buffer++ = bg;
			if constexpr (sizeof(Color) == 4) *buffer++ = bg;
		}
	}
}

#undef RAM
#undef XRAM


} // namespace kilipili::Video


/*





































*/
