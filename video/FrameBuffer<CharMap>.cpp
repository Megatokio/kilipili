// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer<CharMap>.h"
#include "common/Logger.h"
#include "common/cdefs.h"
#include "common/memory.h"

namespace kilipili
{

FrameBufferCharMapBase::FrameBufferCharMapBase(CharMapBase* charmap, scanline_render_fu* fu) noexcept :
	VideoPlane(&do_vblank, &do_render),
	charmap(charmap),
	font1(charmap->fonts[0]),
	font2(charmap->fonts[1]),
	scanline_renderer(fu)
{
	assert(font1->char_width == 8);
	assert(font2->char_width == 8);
	assert(font1->char_height == font2->char_height);

	if (size_t(fu) >= flash_start() && size_t(fu) < flash_end())
	{
		logline("WARNING: scanline render function is in flash!");
	}
}

static constexpr uint32 mul = sizeof(Color) == 1 ? 0x01010101 : sizeof(Color) == 2 ? 0x00010001 : 1;

//static
void __section(RAM ".FB_CM") FrameBufferCharMapBase::do_vblank(VideoPlane* vp) noexcept
{
	auto* me	  = static_cast<FrameBufferCharMapBase*>(vp);
	auto* charmap = me->charmap.ptr();

	// reste state:
	me->raster_line = 0;
	me->last_y		= 0;
	me->row_ptr		= charmap->data; // source ptr

	// live update of font changes:
	// in a race condition the .rc may be messed up, but due to the way RCPtr::operator=() works
	// this should be impossible to happen here. But still better don't switch fonts during vblank.
	me->font1 = charmap->fonts[0]; // unsafe!
	me->font2 = charmap->fonts[1]; // unsafe!

	// live update of color changes:
	uint32* fgcolors = reinterpret_cast<uint32*>(me + 1);
	uint32* bgcolors = fgcolors + (1 << charmap->fg_bits);
	for (int i = 0; i < 1 << charmap->fg_bits; i++) fgcolors[i] = charmap->fgcolors->colors[i] * mul;
	for (int i = 0; i < 1 << charmap->bg_bits; i++) bgcolors[i] = charmap->bgcolors->colors[i] * mul;

	// live update of cursor position;
	me->cursor_ptr = charmap->cursor_ptr;

	if (me->cursor_ptr)
	{
		int	   fg_ss   = charmap->bg_bits;
		int	   bg_ss   = 0;
		uint8  attr	   = me->cursor_ptr[0] >> 8;
		uint32 fgc	   = fgcolors[(attr & charmap->fg_mask) >> fg_ss];
		uint32 bgc	   = bgcolors[(attr & charmap->bg_mask) >> bg_ss];
		me->cursor_xor = fgc ^ bgc;
	}
}

//static
void __section(RAM ".FB_CM") FrameBufferCharMapBase:: //
	do_render(VideoPlane* vp, int y, int width, uint32* buffer) noexcept
{
	auto* me = static_cast<FrameBufferCharMapBase*>(vp);

	static_assert(char_width == 8);
	assert_lt(y, me->charmap->rows * me->font1->char_height);
	assert_le(width, me->charmap->cols * 8);
	assert(width % 8 == 0);

	while (me->last_y < y)
	{
		me->last_y++;
		if (++me->raster_line >= me->font1->char_height)
		{
			me->raster_line = 0;
			me->row_ptr += me->charmap->cols;
		}
	}

	int cols = width >> 3; // if char_width = 8

	me->scanline_renderer(me, buffer, me->row_ptr, cols);

	if unlikely (size_t(me->cursor_ptr - me->row_ptr) < unsigned(cols))
	{
		uint32* z  = buffer + size_t(me->cursor_ptr - me->row_ptr) * (8 * sizeof(Color) / sizeof(uint32));
		uint32	xc = me->cursor_xor;

		*z++ ^= xc;
		*z++ ^= xc;
		if (sizeof(Color) >= 2) *z++ ^= xc;
		if (sizeof(Color) >= 2) *z++ ^= xc;
		if (sizeof(Color) == 4) *z++ ^= xc;
		if (sizeof(Color) == 4) *z++ ^= xc;
		if (sizeof(Color) == 4) *z++ ^= xc;
		if (sizeof(Color) == 4) *z++ ^= xc;
	}
}


// ______________________________________________________________________________
// scanline renderers go into XRAM
// vblank callbacks go into normal RAM
// you can override this by providing the instantiation in your program!

#define define template void __section

define(XRAM ".FB_CM1")
	FrameBuffer<CharMap<0, 0, 1, 1, 1>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM2")
	FrameBuffer<CharMap<4, 4, 0, 0, 0>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM3")
	FrameBuffer<CharMap<4, 3, 1, 0, 0>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM4")
	FrameBuffer<CharMap<4, 3, 0, 0, 1>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM5")
	FrameBuffer<CharMap<4, 2, 1, 1, 0>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM6")
	FrameBuffer<CharMap<4, 2, 1, 0, 1>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM7")
	FrameBuffer<CharMap<3, 3, 1, 1, 0>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM8")
	FrameBuffer<CharMap<3, 3, 1, 0, 1>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM9")
	FrameBuffer<CharMap<2, 2, 1, 1, 1>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;
define(XRAM ".FB_CM10")
	FrameBuffer<CharMap<3, 2, 1, 1, 1>>::render_scanline(VideoPlane*, uint32*, const uint16*, int) noexcept;


} // namespace kilipili


/*





































*/
