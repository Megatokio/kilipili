// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FrameBuffer.h"
#include "Graphics/CharMap.h"
#include "Graphics/Color.h"
#include "internal/Interp.h"


namespace kilipili::Video
{
/*	Implementation of template `FrameBuffer<>` for template character array `CharMap<>`.

	template class FrameBuffer<CharMap> uses one attribute per character.
	The attribute follows immediately after the character code.

	Attributes are configurable per template arguments to `CharMap<>`:
	template parameters:
		fgbits:    foreground color bits
		bgbits:    background color bits
		bold:      enable bold attribute
		underline: enable underline attribute
		graphics:  enable secondary font (default: graphics)
	The total number of bits must not exceed 8.

	Font and Colors are taken from the CharMap and updated every frame.
	The FrameBuffer provides a hardware cursor (inverted character).

	Because gcc has a long standing bug in handling attributes in template instantiations,
	you should not rely on automatic instantiation of the FrameBuffer<> for your CharMap<> of choice,
	because then gcc will then put the callback functions into flash. (except you are fine with this.)

	Instead you must instantiate the callback functions explicitly with the desired section attribute.
	For convenience the most commonly used are predefined at the end of this file. 
	To can override them if your XRAM overflows by providing your own instantiation using RAM in your program.

	To simplify this the #define DEFINE_FB_CM(fgbits,bgbits,bold,ul,gra,section) is provided:
	use:
		DEFINE_FB_CM(2, 2, 1, 1, 0, XRAM)   // RAM or XRAM are defined in common/cdefs.h
*/

template<class CharMap>
class FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>> final : public VideoPlane
{
	using Color = Graphics::Color;

	static constexpr int   fg_bits	 = CharMap::fg_bits;
	static constexpr int   bg_bits	 = CharMap::bg_bits;
	static constexpr uint8 bold		 = CharMap::bold;
	static constexpr uint8 underline = CharMap::underline;
	static constexpr uint8 graphics	 = CharMap::graphics;
	static constexpr int   fg_ss	 = CharMap::fg_ss;
	static constexpr int   bg_ss	 = CharMap::bg_ss;
	static constexpr uchar fg_mask	 = CharMap::fg_mask;
	static constexpr uchar bg_mask	 = CharMap::bg_mask;

	static_assert(fg_bits + bg_bits + !!bold + !!underline + !!graphics <= 8);
	static_assert(uint(fg_bits <= 8));
	static_assert(uint(bg_bits <= 8));

public:
	FrameBuffer(CharMap*) noexcept;
	NO_COPY_MOVE(FrameBuffer);

	static constexpr int char_width = 8;

	RCPtr<CharMap>				charmap;
	RCPtr<const Graphics::Font> font1;
	RCPtr<const Graphics::Font> font2;

	const uchar* row_ptr	 = nullptr; // source row in charmap.data[]
	const uchar* cursor_ptr	 = nullptr; //
	int			 raster_line = 0;		// line within character
	int			 last_y		 = 0;

private:
	static void do_vblank(VideoPlane*) noexcept;
	static void do_render(VideoPlane*, int row, int width, uint32* buffer) noexcept;
};


//
// *****************************************************************************
//					I M P L E M E N T A T I O N S
// *****************************************************************************
//

#define XRAM_DATA __attribute__((section(".scratch_x.FB_CM_DATA")))

// all permutations of mask for four 1-byte Colors in a uint32:
static constexpr XRAM_DATA uint32 mask16x1[16] = {
	0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, 0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff, //
	0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff, 0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,
};

// all permutations of mask for two 2-byte Colors in a uint32:
static constexpr XRAM_DATA uint32 mask4x2[4] = {0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff};

#undef XRAM_DATA


template<class CharMap>
FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::FrameBuffer(CharMap* charmap) noexcept :
	VideoPlane(&do_vblank, &do_render),
	charmap(charmap),
	font1(charmap->font1),
	font2(charmap->font2)
{
	assert(font1->char_width == 8);
	assert(font2->char_width == 8);
	assert(font1->char_height == font2->char_height);
}

template<class CharMap>
void FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::do_vblank(VideoPlane* vp) noexcept
{
	auto* me		= static_cast<FrameBuffer*>(vp);
	me->raster_line = 0;
	me->last_y		= 0;
	me->row_ptr		= cuptr(me->charmap->data);			  // source ptr
	me->cursor_ptr	= cuptr(me->charmap->cursor_ptr) + 2; //

	me->font1 = me->charmap->font1;
	me->font2 = me->charmap->font2;
}

template<class CharMap>
void FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::do_render(
	VideoPlane* vp, int y, int width, uint32* buffer) noexcept
{
	auto* me = static_cast<FrameBuffer*>(vp);

	static_assert(char_width == 8);
	assert_lt(y, me->charmap->rows * me->font1->char_height);
	assert_le(width, me->charmap->cols * char_width);
	assert_eq(width % char_width, 0);

	while (me->last_y < y)
	{
		me->last_y++;
		if (++me->raster_line >= me->font1->char_height)
		{
			me->raster_line = 0;
			me->row_ptr += me->charmap->cols * 2;
		}
	}

	constexpr InterpMode ip		= InterpMode::i2u32;
	Interp* const		 interp = &interp0[ipi<ip>];
	if constexpr (sizeof(Color) == 2) interp->setup(ip);
	if constexpr (sizeof(Color) == 2) interp->set_color_base(mask4x2);

	int			 cols = width >> (3 - 1); // if char_width = 8
	const uchar* font = me->font1->data + me->raster_line * me->font1->row_offset - me->font1->first_glyph;

	const uchar*  font2;	// only if graphics = true
	const uint32* fgcolors; // only if fg_bits > 0
	const uint32* bgcolors; // only if bg_bits > 0
	uint32		  fgc;		// only if fg_bits = 0
	uint32		  bgc;		// only if bg_bits = 0
	uint8		  ul;		// only if underline = true

	if constexpr (graphics) font2 = me->font2->data + me->raster_line * me->font2->row_offset - me->font2->first_glyph;
	if constexpr (underline) ul = me->raster_line == 10 ? underline : 0; // if char_height = 12
	if constexpr (fg_bits != 0) fgcolors = me->charmap->fgcolors;
	if constexpr (bg_bits != 0) bgcolors = me->charmap->bgcolors;
	if constexpr (fg_bits == 0) fgc = *me->charmap->fgcolors;
	if constexpr (bg_bits == 0) bgc = *me->charmap->bgcolors;

	for (cuptr p = me->row_ptr, bu_end = p + cols; p < bu_end;)
	{
		uchar c	   = p[0];
		uint8 attr = p[1];
		uint8 byte = (graphics && (attr & graphics) ? font2 : font)[c];

		cuptr end = bu_end;
		if unlikely (me->cursor_ptr >= p && me->cursor_ptr < end)
		{
			if (p == me->cursor_ptr) byte = uint8(~byte);
			else end = me->cursor_ptr;
		}

		while (p < end)
		{
			uint32 bg;

			if unlikely (underline && (attr & ul))
			{
				bg = fg_bits ? fgcolors[(attr & fg_mask) >> fg_ss] : fgc;
				goto space;
			}

			bg = bg_bits ? bgcolors[(attr & bg_mask) >> bg_ss] : bgc;

			if (byte)
			{
				if (bold && (attr & bold)) byte |= uint8(byte << 1);
				uint32 fg = fg_bits ? fgcolors[(attr & fg_mask) >> fg_ss] : fgc;

				if constexpr (sizeof(Color) == 1)
				{
					fg = fg ^ bg;

					*buffer++ = bg ^ (fg & mask16x1[byte & 0x0f]);
					*buffer++ = bg ^ (fg & mask16x1[byte >> 4]);
				}
				else if constexpr (sizeof(Color) == 2)
				{
					fg = fg ^ bg;

					interp->set_pixels(byte, 2);
					*buffer++ = bg ^ (fg & *interp->next_color<uint32>());
					*buffer++ = bg ^ (fg & *interp->next_color<uint32>());
					*buffer++ = bg ^ (fg & *interp->next_color<uint32>());
					*buffer++ = bg ^ (fg & *interp->next_color<uint32>());
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
			space:
				*buffer++ = bg;
				*buffer++ = bg;

				if constexpr (sizeof(Color) >= 2) *buffer++ = bg;
				if constexpr (sizeof(Color) >= 2) *buffer++ = bg;

				if constexpr (sizeof(Color) == 4) *buffer++ = bg;
				if constexpr (sizeof(Color) == 4) *buffer++ = bg;
				if constexpr (sizeof(Color) == 4) *buffer++ = bg;
				if constexpr (sizeof(Color) == 4) *buffer++ = bg;
			}

			p += 2;
			c	 = p[0];
			attr = p[1];
			byte = (graphics && (attr & graphics) ? font2 : font)[c];
		}
	}
	if constexpr (sizeof(Color) == 2) cleanup_if_needed<ip>();
}


// _______________________________________
// pre-define some for a quick start:
// scanline renderers go into XRAM.
// you can override this by providing an instantiation in your program!

// clang-format off
#define DEFINE_FB_CM(a,b,c,d,e) \
extern template void FrameBuffer<Graphics::CharMap<a,b,c,d,e>>::do_render(VideoPlane*, int, int, uint32*) noexcept; \
extern template void FrameBuffer<Graphics::CharMap<a,b,c,d,e>>::do_vblank(VideoPlane*) noexcept;

DEFINE_FB_CM(0, 0, 1, 1, 1) // up to 1280x768, monochrome + bold + underline + graphics

DEFINE_FB_CM(4, 4, 0, 0, 0) // up to 800x600, 16 fg and 16 bg colors

DEFINE_FB_CM(4, 3, 1, 0, 0) // up to 800x600 + bold
DEFINE_FB_CM(4, 3, 0, 0, 1) // up to 800x600 + graphics

DEFINE_FB_CM(4, 2, 1, 1, 0) // up to 800x600 + bold + underline
DEFINE_FB_CM(4, 2, 1, 0, 1) // up to 800x600 + bold + graphics

DEFINE_FB_CM(3, 3, 1, 1, 0) // up to 800x600 + bold + underline
DEFINE_FB_CM(3, 3, 1, 0, 1) // up to 800x600 + bold + graphics

DEFINE_FB_CM(3, 2, 1, 1, 1) // up to 800x600 + bold + underline + graphics
DEFINE_FB_CM(2, 2, 1, 1, 1) // up to 800x600 + bold + underline + graphics
#undef DEFINE_FB_CM


// _______________________________________
// use DEFINE_FB_CM(..) if you need other variants:

#define DEFINE_FB_CM(a,b,c,d,e,SECTION) namespace kilipili::Video{ \
template void __section(RAM ".FB_CM") FrameBuffer<kilipili::Graphics::CharMap<a,b,c,d,e>>::do_vblank(VideoPlane*) noexcept; \
template void __section(SECTION ".FB_CM") FrameBuffer<kilipili::Graphics::CharMap<a,b,c,d,e>>::do_render(VideoPlane*, int, int, uint32*) noexcept; }
// clang-format on


} // namespace kilipili::Video


/*





































*/
