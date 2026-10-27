// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FrameBuffer.h"
#include "Graphics/CharMap.h"
#include "Graphics/Color.h"
#include "internal/Interp.h"


namespace kilipili
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


/*	class FrameBufferCharMapBase is the base class for all FrameBuffer<CharMap<..>>.
	It provides do_vblank() and do_render() which calls function scanline_renderer for the actual rendering.
*/
class FrameBufferCharMapBase : public VideoPlane
{
public:
	NO_COPY_MOVE(FrameBufferCharMapBase);

	using scanline_render_fu = void(VideoPlane* vp, uint32* z, const uint16* q, int cols) noexcept;

	static constexpr int char_width = 8;

	// set in ctor:
	RCPtr<CharMapBase>	charmap;
	RCPtr<const Font>	font1;
	RCPtr<const Font>	font2;
	scanline_render_fu* scanline_renderer = nullptr;

	// set in do_vblank:
	const uint16* cursor_ptr = nullptr; // set in vblank
	uint32		  cursor_xor = 0;		// xor_color for cursor blob

	// reset in do_vblank, updated in do_render:
	const uint16* row_ptr	  = nullptr; // source row in charmap.data[]
	int			  raster_line = 0;		 // line within character
	int			  last_y	  = 0;		 // line within screen

protected:
	FrameBufferCharMapBase(CharMapBase*, scanline_render_fu*) noexcept;
	static void do_vblank(VideoPlane*) noexcept;
	static void do_render(VideoPlane*, int row, int width, uint32* buffer) noexcept;
};


// ===================================================================================

/*	template class FrameBuffer<CharMap<..>> is the concrete class for the FrameBuffer for the actual CharMap<..>.
	It provides function scanline_renderer for the actual rendering specific to CharMap<..>.
*/
template<class CharMap>
class FrameBuffer<CharMap, std::enable_if_t<CharMap::isa_CharMap>> final : public FrameBufferCharMapBase
{
public:
	static constexpr int	fg_bits	  = CharMap::fg_bits;
	static constexpr int	bg_bits	  = CharMap::bg_bits;
	static constexpr uint16 bold	  = CharMap::bold << 8;		 // mask
	static constexpr uint16 underline = CharMap::underline << 8; // mask
	static constexpr uint16 graphics  = CharMap::graphics << 8;	 // mask
	static constexpr int	fg_ss	  = CharMap::fg_ss;
	static constexpr int	bg_ss	  = CharMap::bg_ss;
	static constexpr uchar	fg_mask	  = CharMap::fg_mask;
	static constexpr uchar	bg_mask	  = CharMap::bg_mask;

	static_assert(fg_bits + bg_bits + !!bold + !!underline + !!graphics <= 8);
	static_assert(uint(fg_bits) <= 8);
	static_assert(uint(bg_bits) <= 8);

	FrameBuffer(CharMap* cm) noexcept : FrameBufferCharMapBase(cm, &render_scanline) {}
	virtual ~FrameBuffer() noexcept override = default;
	NO_COPY_MOVE(FrameBuffer);

	uint32 fgcolors[1 << fg_bits];
	uint32 bgcolors[1 << bg_bits];

private:
	static void render_scanline(VideoPlane*, uint32* z, const uint16* p, int cols) noexcept;
};


// all permutations of mask for four 1-byte Colors in a uint32:
constexpr __section(".scratch_x.FB_CM_DATA1") uint32 mask16x1[16] = {
	0x00000000, 0x000000ff, 0x0000ff00, 0x0000ffff, 0x00ff0000, 0x00ff00ff, 0x00ffff00, 0x00ffffff, //
	0xff000000, 0xff0000ff, 0xff00ff00, 0xff00ffff, 0xffff0000, 0xffff00ff, 0xffffff00, 0xffffffff,
};

// all permutations of mask for two 2-byte Colors in a uint32:
constexpr __section(".scratch_x.FB_CM_DATA2") uint32 mask4x2[4] = {0x00000000, 0x0000ffff, 0xffff0000, 0xffffffff};


template<class CharMap>
void FrameBuffer<CharMap, std::enable_if_t<CharMap::isa_CharMap>>:: //
	render_scanline(VideoPlane* vp, uint32* z, const uint16* q, int cols) noexcept
{
	static_assert(char_width == 8);

	auto* me = static_cast<FrameBuffer*>(vp);

	Interp* const interp = interp0;
	if constexpr (sizeof(Color) == 2) interp->setup(InterpMode::i2u32);
	if constexpr (sizeof(Color) == 2) interp->set_color_base(mask4x2);

	if constexpr (bg_bits) interp1->base[0] = uint32(me->bgcolors);
	if constexpr (fg_bits) interp1->base[1] = uint32(me->fgcolors);

	if constexpr (bg_bits) interp1->ctrl[0] = InterpConfig().set_mask(2, 2 + bg_bits - 1).set_shift(8 + bg_ss - 2);
	if constexpr (fg_bits)
		interp1->ctrl[1] = InterpConfig().set_mask(2, 2 + fg_bits - 1).set_shift(8 + fg_ss - 2).set_cross_input();

	const uchar* font = me->font1->data + me->raster_line * me->font1->row_offset - me->font1->first_glyph;
	const uchar* font2; // only if graphics = true
	uint		 ul;	// only if underline = true
	uint32		 bg;
	uint32		 fg;

	if constexpr (graphics) font2 = me->font2->data + me->raster_line * me->font2->row_offset - me->font2->first_glyph;
	if constexpr (underline) ul = me->raster_line == 10 ? underline : 0; // if char_height = 12
	if constexpr (fg_bits == 0) fg = me->fgcolors[0];
	if constexpr (bg_bits == 0) bg = me->bgcolors[0];

	for (const uint16* end = q + cols; q < end;)
	{
		uint attr = *q++;

		if constexpr (fg_bits || bg_bits) interp1->set_accumulator(0, attr);

		if (underline && (attr & ul))
		{
			if constexpr (fg_bits) fg = *interp1->peek<uint32*>(1);

			*z++ = fg;
			*z++ = fg;
			if constexpr (sizeof(Color) >= 2) *z++ = fg;
			if constexpr (sizeof(Color) >= 2) *z++ = fg;
			if constexpr (sizeof(Color) == 4) *z++ = fg;
			if constexpr (sizeof(Color) == 4) *z++ = fg;
			if constexpr (sizeof(Color) == 4) *z++ = fg;
			if constexpr (sizeof(Color) == 4) *z++ = fg;
			continue;
		}

		if constexpr (bg_bits) bg = *interp1->peek<uint32*>(0);

		uint byte = ((attr & graphics) ? font2 : font)[uchar(attr)];

		if (byte)
		{
			if (bold && (attr & bold)) byte |= byte << 1;

			if constexpr (fg_bits) fg = *interp1->peek<uint32*>(1);

			if constexpr (sizeof(Color) == 1)
			{
				*z++ = bg ^ ((fg ^ bg) & mask16x1[byte & 0x0f]);
				*z++ = bg ^ ((fg ^ bg) & mask16x1[(byte >> 4) & 0x0f]);
			}
			else if constexpr (sizeof(Color) == 2)
			{
				interp->set_pixels(byte, 2);
				*z++ = bg ^ ((fg ^ bg) & *interp->next_color<uint32>());
				*z++ = bg ^ ((fg ^ bg) & *interp->next_color<uint32>());
				*z++ = bg ^ ((fg ^ bg) & *interp->next_color<uint32>());
				*z++ = bg ^ ((fg ^ bg) & *interp->next_color<uint32>());
			}
			else // Color = 4 bytes
			{
				*z++ = byte & 1 ? fg : bg;
				*z++ = byte & 2 ? fg : bg;
				*z++ = byte & 4 ? fg : bg;
				*z++ = byte & 8 ? fg : bg;
				*z++ = byte & 0x10 ? fg : bg;
				*z++ = byte & 0x20 ? fg : bg;
				*z++ = byte & 0x40 ? fg : bg;
				*z++ = byte & 0x80 ? fg : bg;
			}
		}
		else // byte = 0x00
		{
			*z++ = bg;
			*z++ = bg;
			if constexpr (sizeof(Color) >= 2) *z++ = bg;
			if constexpr (sizeof(Color) >= 2) *z++ = bg;
			if constexpr (sizeof(Color) == 4) *z++ = bg;
			if constexpr (sizeof(Color) == 4) *z++ = bg;
			if constexpr (sizeof(Color) == 4) *z++ = bg;
			if constexpr (sizeof(Color) == 4) *z++ = bg;
		}
	}
}


// _______________________________________
// pre-define some for a quick start:
// scanline renderers go into XRAM.
// you can override this by providing an instantiation in your program!

// clang-format off
#define DEFINE_FB_CM(a,b,c,d,e) \
extern template void FrameBuffer<CharMap<a,b,c,d,e>>::render_scanline(VideoPlane*,uint32*,const uint16*,int)noexcept;

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

#define DEFINE_FB_CM(a,b,c,d,e,SECTION) namespace kilipili{ \
template void __section(SECTION ".FB_CM") FrameBuffer<CharMap<a,b,c,d,e>>:: \
	render_scanline(VideoPlane*,uint32*,const uint16*,int)noexcept; }
// clang-format on


} // namespace kilipili


/*





































*/
