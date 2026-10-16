// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer<CharMap>.h"
#include "common/Logger.h"
#include "common/cdefs.h"
#include "common/memory.h"

namespace kilipili
{

FrameBufferCharMapBase::FrameBufferCharMapBase(
	CharMapBase* charmap, scanline_render_fu* fu, uint32* fg, uint32* bg) noexcept :
	VideoPlane(&do_vblank, &do_render),
	charmap(charmap),
	font1(charmap->fonts[0]),
	font2(charmap->fonts[1]),
	scanline_renderer(fu),
	fgcolors(fg),
	bgcolors(bg)
{
	assert(font1->char_width == 8);
	assert(font2->char_width == 8);
	assert(font1->char_height == font2->char_height);

	if (size_t(fu) >= flash_start() && size_t(fu) < flash_end())
	{
		logline("WARNING: scanline render function is in flash!");
	}
}

//static
void __section(RAM ".FB_CM") FrameBufferCharMapBase::do_vblank(VideoPlane* vp) noexcept
{
	auto* me	  = static_cast<FrameBufferCharMapBase*>(vp);
	auto* charmap = me->charmap.ptr();

	me->raster_line = 0;
	me->last_y		= 0;
	me->font1		= charmap->fonts[0];
	me->font2		= charmap->fonts[1];
	me->row_ptr		= charmap->data;	   // source ptr
	me->cursor_ptr	= charmap->cursor_ptr; //

	if (me->cursor_ptr)
	{
		int	   fg_ss   = charmap->bg_bits;
		int	   bg_ss   = 0;
		uint8  attr	   = me->cursor_ptr[0] >> 8;
		uint32 fgc	   = me->fgcolors[(attr & charmap->fg_mask) >> fg_ss];
		uint32 bgc	   = me->bgcolors[(attr & charmap->bg_mask) >> bg_ss];
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
