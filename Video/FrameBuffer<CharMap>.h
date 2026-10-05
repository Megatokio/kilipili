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

	Because gcc has a long outstanding bug in handling attributes in template instantiations,
	you should not simply instantiate the FrameBuffer<> for your CharMap<> of choice,
	because then gcc will then put the callback functions into flash. (except you are fine with this.)
	Instead you must instantiate the callback functions explicitly with the desired section attribute.
	To simplify this the #define DEFINE_FB_CB(...) is provided.
	Use DEFINE_FB_CM(fgbits,bgbits,bold,ul,gra,section) to define the callbacks for the FrameBuffer you actually use:
	e.g.:
		  namespace kilipili::Video{
		  DEFINE_FB_CM(2, 2, 1, 1, 0, XRAM)   // XRAM = ".scratch_x" is defined in cdefs.h
	    }

	There are some variants predefined for a quick start anyway:
		DEFINE_FB_CM(0, 0, 1, 1, 1, VIDEO_SCANLINE_RENDERER_SECTION) // works safely up to 1024x768
		DEFINE_FB_CM(3, 3, 1, 1, 0, VIDEO_SCANLINE_RENDERER_SECTION) // up to 800x600
		DEFINE_FB_CM(3, 2, 1, 1, 1, VIDEO_SCANLINE_RENDERER_SECTION) // up to 800x600
*/

template<class CharMap>
class FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>> final : public VideoPlane
{
	using Color = Graphics::Color;

	static constexpr int   fg_bits		  = CharMap::fg_bits;
	static constexpr int   bg_bits		  = CharMap::bg_bits;
	static constexpr bool  bold			  = CharMap::bold;
	static constexpr bool  underline	  = CharMap::underline;
	static constexpr bool  graphics		  = CharMap::graphics;
	static constexpr int   fg_ss		  = CharMap::fg_ss;
	static constexpr int   bg_ss		  = CharMap::bg_ss;
	static constexpr uchar fg_mask		  = CharMap::fg_mask;
	static constexpr uchar bg_mask		  = CharMap::bg_mask;
	static constexpr uchar bold_mask	  = CharMap::bold_mask;
	static constexpr uchar underline_mask = CharMap::underline_mask;
	static constexpr uchar graphics_mask  = CharMap::graphics_mask;

	static_assert(fg_bits + bg_bits + bold + underline + graphics <= 8);
	static_assert(uint(fg_bits <= 8));
	static_assert(uint(bg_bits <= 8));

public:
	FrameBuffer(CharMap*) noexcept;
	NO_COPY_MOVE(FrameBuffer);

	static constexpr int char_width	 = 8;
	static constexpr int char_height = 12;

	RCPtr<CharMap> charmap;
	const uchar*   row_ptr	   = nullptr; // charmap row
	int			   raster_line = 0;		  // line within character
	int			   last_y	   = 0;

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
	charmap(charmap)
{}

template<class CharMap>
void FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::do_vblank(VideoPlane* vp) noexcept
{
	auto* me		= static_cast<FrameBuffer*>(vp);
	me->raster_line = 0;
	me->last_y		= 0;
	me->row_ptr		= cuptr(me->charmap->data);
}

template<class CharMap>
void FrameBuffer<CharMap, std::enable_if_t<CharMap::fg_bits >= 0>>::do_render(
	VideoPlane* vp, int y, int width, uint32* buffer) noexcept
{
	auto* me = static_cast<FrameBuffer*>(vp);

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

	constexpr InterpMode ip		= InterpMode::i2u32;
	Interp* const		 interp = &interp0[ipi<ip>];
	if constexpr (sizeof(Color) == 2) interp->setup(ip);
	if constexpr (sizeof(Color) == 2) interp->set_color_base(mask4x2);

	int			 cols = width >> 3;								   // assumes char_width = 8
	const uchar* font = me->charmap->font + me->raster_line * 256; // TODO: this assumes 256 glyphs in font

	const uchar*  font2;	// only if graphics = true
	const uint32* fgcolors; // only if fg_bits > 0
	const uint32* bgcolors; // only if bg_bits > 0
	uint32		  fgc;		// only if fg_bits = 0
	uint32		  bgc;		// only if bg_bits = 0
	uint8		  ul;		// only if underline = true

	if constexpr (graphics) font2 = me->charmap->font2 + me->raster_line * 256;
	if constexpr (underline) ul = me->raster_line == 10 ? underline_mask : 0;
	if constexpr (fg_bits != 0) fgcolors = me->charmap->fgcolors;
	if constexpr (bg_bits != 0) bgcolors = me->charmap->bgcolors;
	if constexpr (fg_bits == 0) fgc = *me->charmap->fgcolors;
	if constexpr (bg_bits == 0) bgc = *me->charmap->bgcolors;

	for (cuptr p = me->row_ptr, end = p + 2 * cols; p < end;)
	{
		uchar  c	= *p++;
		uint8  attr = *p++;
		uint32 bg;
		uchar  byte;

		if unlikely (underline && (attr & ul))
		{
			bg = fg_bits ? fgcolors[(attr & fg_mask) >> fg_ss] : fgc;
			goto space;
		}

		bg	 = bg_bits ? bgcolors[(attr & bg_mask) >> bg_ss] : bgc;
		byte = (graphics && (attr & graphics_mask) ? font2 : font)[c];

		if (byte)
		{
			if (bold && (attr & bold_mask)) byte |= byte << 1;
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
	}

	if constexpr (sizeof(Color) == 2) cleanup_if_needed<ip>();
}

// clang-format off
#define DEFINE_FB_CM(a,b,c,d,e,SECTION)\
template void __section(SECTION ".FB_CM") FrameBuffer<Graphics::CharMap<a,b,c,d,e>>::do_render(VideoPlane*, int, int, uint32*) noexcept;\
template void __section(RAM ".FB_CM") FrameBuffer<Graphics::CharMap<a,b,c,d,e>>::do_vblank(VideoPlane*) noexcept;
// clang-format on

#ifndef VIDEO_SCANLINE_RENDERER_SECTION
  #define VIDEO_SCANLINE_RENDERER_SECTION XRAM
#endif

// pre-define some for a quick start:
DEFINE_FB_CM(0, 0, 1, 1, 1, VIDEO_SCANLINE_RENDERER_SECTION) // works safely up to 1024x768
DEFINE_FB_CM(3, 3, 1, 1, 0, VIDEO_SCANLINE_RENDERER_SECTION) // up to 800x600
DEFINE_FB_CM(3, 2, 1, 1, 1, VIDEO_SCANLINE_RENDERER_SECTION) // up to 800x600
#undef DEFAULT


} // namespace kilipili::Video


/*





































*/
