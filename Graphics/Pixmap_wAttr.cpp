// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "Pixmap_wAttr.h"

namespace kio::Graphics
{


template<ColorMode CM>
void AttrModePixmap::fillRect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);
	w		 = x2 - x1;
	h		 = y2 - y1;

	if (w > 0 && h > 0)
	{
		attr_fill_rect(x1, y1, w, h, color, ink);
		super::fill_rect(x1, y1, w, h, ink);
	}
}

template<ColorMode CM>
void AttrModePixmap::clear(uint color) noexcept
{
	// clear pixmap to:     all inks = 0
	// clear attributes to: all colors = color
	// if this is a window into another Pixmap and if width or height is not a multiple of the attribute cell size
	// then this bleeds the color to full attribute cells.

	coord w = calc_ax(width - 1) + colors_per_attr;
	coord h = calc_ay(height - 1) + 1;

	assert(w <= attributes.width);
	assert(h <= attributes.height);

	attributes.fill_rect(0, 0, w, h, color);  // all colors in all attributes := color
	super::fill_rect(0, 0, width, height, 0); // all pixels: ink := 0
}

template<ColorMode CM>
void AttrModePixmap::xorRect(coord x1, coord y1, coord w, coord h, uint color) noexcept
{
	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);
	w		 = x2 - x1;
	h		 = y2 - y1;

	if (w > 0 && h > 0) attr_xor_rect(x1, y1, w, h, color);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy a rectangular area from another pixmap of the same ColorDepth.

	if (unlikely(qx < 0))
	{
		w += qx;
		zx -= qx;
		qx -= qx;
	}
	if (unlikely(zx < 0))
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (unlikely(qy < 0))
	{
		h += qy;
		zy -= qy;
		qy -= qy;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}
	w = min(w, q.width - qx, width - zx);
	h = min(h, q.height - qy, height - zy);

	if (w > 0 && h > 0)
	{
		assert(zx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
		assert(zy % attrheight == 0);  // y must be a multiple of the attribute cell height
		assert(((zx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

		assert(qx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
		assert(qy % attrheight == 0);  // y must be a multiple of the attribute cell height
		assert(((qx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

		super::copy_rect(zx, zy, q.as_super(), qx, qy, w, h);
		w  = calc_ax(zx + w - 1) + colors_per_attr;
		h  = calc_ay(zy + h - 1) + 1;
		zx = calc_ax(zx);
		zy = calc_ay(zy);
		qx = calc_ax(qx);
		qy = calc_ay(qy);
		w  = w - zx;
		h  = h - zy;
		attributes.copy_rect(zx, zy, q.attributes, qx, qy, w, h);
	}
}

template<ColorMode CM>
void AttrModePixmap::copyRect(coord zx, coord zy, const Pixmap& q) noexcept
{
	copyRect(zx, zy, q, 0, 0, q.width, q.height);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	copyRect(zx, zy, *this, qx, qy, w, h);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(coord zx, coord zy, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(CM == q.colormode); // must be same type
	copyRect(zx, zy, static_cast<const Pixmap&>(q), qx, qy, w, h);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(const Point& z, const Pixmap& q) noexcept
{
	copyRect(z.x, z.y, q, 0, 0, q.width, q.height);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(const Point& zp, const Pixmap& pm, const Point& qp, const Size& size) noexcept
{
	copyRect(zp.x, zp.y, pm, qp.x, qp.y, size.width, size.height);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(const Point& zp, const Pixmap& pm, const Rect& qr) noexcept
{
	copyRect(zp.x, zp.y, pm, qr.left(), qr.top(), qr.width(), qr.height());
}

template<ColorMode CM>
void AttrModePixmap::drawBmp(
	coord zx, coord zy, const uint8* bmp, int bmp_row_offset, int w, int h, uint color, uint ink) noexcept
{
	// draw bitmap into Canvas
	// draw the set bits with `color`, skip the zeros

	if (unlikely(zx < 0))
	{
		w += zx;
		bmp -= zx / 8; // TODO: need x0 offset in bitblit::draw_bitmap()
		zx -= zx;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		bmp -= zy * bmp_row_offset;
		zy -= zy;
	}
	w = min(w, width - zx);
	h = min(h, height - zy);

	if (w > 0 && h > 0)
	{
		attr_fill_rect(zx, zy, w, h, color, ink);
		super::draw_bmp(zx, zy, bmp, bmp_row_offset, w, h, ink);
	}
}

template<ColorMode CM>
void AttrModePixmap::drawBmp(coord zx, coord zy, const Bitmap& bmp, uint color, uint ink) noexcept
{
	drawBmp(zx, zy, bmp.pixmap, bmp.row_offset, bmp.width, bmp.height, color, ink);
}

template<ColorMode CM>
void AttrModePixmap::drawChar(coord zx, coord zy, const uint8* bmp, int h, uint color, uint ink) noexcept
{
	// optimized version of drawBmp:
	//   row_offset = 1
	//   width = 8
	//   x = N * 8

	if (unlikely(zx < 0 || zx >= width) || (zx & 7) != 0) return drawBmp(zx, zy, bmp, 1, 8, h, color, ink);

	if (unlikely(zy < 0))
	{
		h += zy;
		bmp -= zy;
		zy -= zy;
	}
	h = min(h, height - zy);

	if (h > 0)
	{
		attr_fill_rect(zx, zy, 8, h, color, ink);
		super::draw_char(zx, zy, bmp, h, ink);
	}
}


// instantiate everything.
// the linker will know what we need:

template class Pixmap<colormode_a1w1_i4>;
template class Pixmap<colormode_a1w1_i8>;
template class Pixmap<colormode_a1w1_i16>;
template class Pixmap<colormode_a1w2_i4>;
template class Pixmap<colormode_a1w2_i8>;
template class Pixmap<colormode_a1w2_i16>;
template class Pixmap<colormode_a1w4_i4>;
template class Pixmap<colormode_a1w4_i8>;
template class Pixmap<colormode_a1w4_i16>;
template class Pixmap<colormode_a1w8_i4>;
template class Pixmap<colormode_a1w8_i8>;
template class Pixmap<colormode_a1w8_i16>;
template class Pixmap<colormode_a2w1_i4>;
template class Pixmap<colormode_a2w1_i8>;
template class Pixmap<colormode_a2w1_i16>;
template class Pixmap<colormode_a2w2_i4>;
template class Pixmap<colormode_a2w2_i8>;
template class Pixmap<colormode_a2w2_i16>;
template class Pixmap<colormode_a2w4_i4>;
template class Pixmap<colormode_a2w4_i8>;
template class Pixmap<colormode_a2w4_i16>;
template class Pixmap<colormode_a2w8_i4>;
template class Pixmap<colormode_a2w8_i8>;
template class Pixmap<colormode_a2w8_i16>;


} // namespace kio::Graphics
