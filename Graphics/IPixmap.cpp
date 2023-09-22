// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "IPixmap.h"


namespace kio::Graphics
{


IPixmap::IPixmap(coord w, coord h, ColorMode CM, AttrHeight AH, bool allocated) noexcept :
	width(w),
	height(h),
	colormode(CM),
	attrheight(AH),
	allocated(allocated) {};

void IPixmap::draw_hline(coord x1, coord y1, coord w, uint color, uint ink) noexcept
{
	constexpr coord h = 1;
	assert(x1 >= 0 && x1 + w <= width);
	assert(y1 >= 0 && y1 + h <= height);

	while (--w >= 0) set_pixel(x1++, y1, color, ink);
}

void IPixmap::draw_vline(coord x1, coord y1, coord h, uint color, uint ink) noexcept
{
	constexpr coord w = 1;
	assert(x1 >= 0 && x1 + w <= width);
	assert(y1 >= 0 && y1 + h <= height);

	while (--h >= 0) set_pixel(x1, y1++, color, ink);
}

void IPixmap::fill_rect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	assert(x1 >= 0 && x1 + w <= width);
	assert(y1 >= 0 && y1 + h <= height);

	while (--h >= 0) draw_hline(x1, y1++, w, color, ink);
}

void IPixmap::xor_rect(coord zx, coord zy, coord w, coord h, uint xor_color) noexcept
{
	assert(zx >= 0 && zx + w <= width);
	assert(zy >= 0 && zy + h <= height);

	while (--h >= 0)
	{
		for (coord i = 0; i < w; i++)
		{
			uint ink, color = get_pixel(zx + i, zy++, &ink);
			set_pixel(zx + i, zy++, color ^ xor_color, ink);
		}
	}
}

void IPixmap::copy_rect(coord zx, coord zy, const IPixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(zx >= 0 && zx + w <= width);
	assert(zy >= 0 && zy + h <= height);
	assert(qx >= 0 && qx + w <= q.width);
	assert(qy >= 0 && qy + h <= q.height);

	while (--h >= 0)
	{
		for (coord i = 0; i < w; i++)
		{
			uint ink, color = get_pixel(qx + i, qy++, &ink);
			set_pixel(zx + i, zy++, color, ink);
		}
	}
}

void IPixmap::copy_rect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(zx >= 0 && zx + w <= width);
	assert(zy >= 0 && zy + h <= height);
	assert(qx >= 0 && qx + w <= width);
	assert(qy >= 0 && qy + h <= height);

	if (zy > qy || (zy == qy && zx < qx))
	{
		while (--h >= 0)
		{
			for (coord i = 0; i < w; i++)
			{
				uint ink, color = get_pixel(qx + i, qy++, &ink);
				set_pixel(zx + i, zy++, color, ink);
			}
		}
	}
	else
	{
		while (--h >= 0)
		{
			for (coord i = w; --i >= 0;)
			{
				uint ink, color = get_pixel(qx + i, qy + h, &ink);
				set_pixel(zx + i, zy + h, color, ink);
			}
		}
	}
}

void IPixmap::read_hline_bmp(coord x, coord y, coord w, uint8* z, uint color, bool set) noexcept
{
	// helper:
	// read & convert horizontal line from pixmap to bitmap line
	// for attributed pixmaps the bitmap is constructed from the attribute colors, not just from the pixmap pixels.

	constexpr coord h = 1;
	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + h <= height);

	while (w > 0)
	{
		uint8 byte = set ? 0 : 0xff;
		for (uint m = 0; m < 0x100; m = m << 1)
		{
			if (get_color(x++, y) == color) byte ^= m;
		}

		w -= 8;
		if (w < 0) byte &= 0xff >> -w;
		*z++ = byte;
	}
}

void IPixmap::draw_hline_bmp(coord x, coord y, coord w, const uint8* q, uint color, uint ink) noexcept
{
	// helper:
	// convert & write horizontal line from a bitmap
	// draws only the set bits

	constexpr coord h = 1;
	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + h <= height);

	while (w > 0)
	{
		w -= 8;
		uint8 byte = *q++;
		if (w < 0) byte &= 0xff >> -w;

		for (uint m = 0; m < 0x100; m = m << 1)
		{
			if (byte & m) set_pixel(x, y, color, ink);
			x++;
		}
	}
}

void IPixmap::draw_bmp(
	coord x, coord y, const uint8* bmp, int row_offs, coord w, coord h, uint color, uint ink) noexcept
{
	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + h <= height);

	while (--h >= 0)
	{
		draw_hline_bmp(x, y++, w, bmp, color, ink);
		bmp += row_offs;
	}
}

void IPixmap::draw_char(coord x, coord y, const uint8* q, coord h, uint color, uint ink) noexcept
{
	// possibly optimized version of draw_bmp:
	//   row_offset is always 1
	//   width is always 8

	constexpr coord w = 8;
	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + h <= height);

	while (--h >= 0) //
	{
		draw_hline_bmp(x, y++, 8, q++, color, ink);
	}
}

void IPixmap::read_bmp(coord x, coord y, uint8* bmp, int row_offset, coord w, coord h, uint color, bool set) noexcept
{
	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + h <= height);

	while (--h >= 0)
	{
		read_hline_bmp(x, y++, w, bmp, color, set);
		bmp += row_offset;
	}
}


// #########################################################


void IPixmap::setPixel(coord x, coord y, uint color, uint ink) noexcept
{
	if (is_inside(x, y)) set_pixel(x, y, color, ink);
}

uint IPixmap::getPixel(coord x, coord y, uint* ink) const noexcept
{
	return is_inside(x, y) ? get_pixel(x, y, ink) : (*ink = 0);
}

uint IPixmap::getInk(coord x, coord y) const noexcept { return is_inside(x, y) ? get_ink(x, y) : 0; }

uint IPixmap::getColor(coord x, coord y) const noexcept { return is_inside(x, y) ? get_color(x, y) : 0; }

void IPixmap::drawHLine(coord x, coord y, coord w, uint color, uint ink) noexcept
{
	// draw horizontal line
	// draws nothing if w <= 0

	if (uint(y) >= uint(height)) return;

	x = max(x, 0);
	w = min(w, width - x);

	draw_hline(x, y, w, color, ink);
}

void IPixmap::drawVLine(coord x, coord y, coord h, uint color, uint ink) noexcept
{
	// draw vertical line
	// draws nothing if h <= 0

	if (uint(x) >= uint(width)) return;

	y = max(y, 0);
	h = min(h, height - y);

	draw_vline(x, y, h, color, ink);
}

void IPixmap::fillRect(coord x, coord y, coord w, coord h, uint color, uint ink) noexcept
{
	// draw filled rectangle.
	// nothing is drawn for empty rect!

	x = max(x, 0);
	w = min(w, width - x);

	y = max(y, 0);
	h = min(h, height - y);

	fill_rect(x, y, w, h, color, ink);
}

void IPixmap::clear(uint color, uint ink) noexcept { fill_rect(0, 0, width, height, color, ink); }

void IPixmap::copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy the pixels from a rectangular area within the same pixmap.
	// note: especially slow for attributed pixmaps. try to copy pixels and attributes separately.
	// overlapping areas are handled safely.
	// if w<0 or h<0 then nothing is drawn.

	if (qx < 0)
	{
		w += qx;
		zx -= qx;
		qx -= qx;
	}
	if (zx < 0)
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (qy < 0)
	{
		h += qy;
		zy -= qy;
		qy -= qy;
	}
	if (zy < 0)
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}

	w = min(w, width - zx, width - qx);
	h = min(h, height - zy, height - qy);

	copy_rect(zx, zy, qx, qy, w, h);
}

void IPixmap::copyRect(coord zx, coord zy, const IPixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy and convert a rectangular area from another pixmap.
	// works poorly / makes no sense with attributed pixmaps.
	// for attributed pixmaps the user will probably copy pixels and attributes separately.
	// if w<0 or h<0 then nothing is drawn.

	if (qx < 0)
	{
		w += qx;
		zx -= qx;
		qx -= qx;
	}
	if (zx < 0)
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (qy < 0)
	{
		h += qy;
		zy -= qy;
		qy -= qy;
	}
	if (zy < 0)
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}
	w = min(w, q.width - qx, width - zx);
	h = min(h, q.height - qy, height - zy);

	copy_rect(zx, zy, q, qx, qy, w, h);
}

void IPixmap::copyRect(coord zx, coord zy, const IPixmap& q) noexcept
{
	// convert & draw another pixmap.
	// works poorly / makes no sense with attributed pixmaps.
	// for attributed pixmaps the user will probably copy pixels and attributes separately.

	copyRect(zx, zy, q, 0, 0, q.width, q.height);
}

void IPixmap::drawBmp(
	coord zx, coord zy, const uint8* bmp, int row_offset, coord w, coord h, uint color, uint ink) noexcept
{
	if (zx < 0)
	{
		w += zx;
		bmp -= zx / 8; // TODO: need x0 offset in draw_hline_bmp()
		zx -= zx;
	}
	if (zy < 0)
	{
		h += zy;
		bmp -= zy * row_offset;
		zy -= zy;
	}
	w = min(w, width - zx);
	h = min(h, height - zy);

	draw_bmp(zx, zy, bmp, row_offset, w, h, color, ink);

	while (--h >= 0)
	{
		draw_hline_bmp(zx, zy++, w, bmp, color, ink);
		bmp += row_offset;
	}
}

void IPixmap::drawChar(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink) noexcept
{
	// optimized version of drawBmp:
	//   row_offset is always 1
	//   width is always 8
	//   x must be a multiple of 8

	if (unlikely(x < 0 || x > width - 8)) return;

	if (y < 0)
	{
		h += y;
		bmp -= y;
		y -= y;
	}
	h = min(h, height - y);

	while (--h >= 0) { draw_hline_bmp(x, y++, 8, bmp++, color, ink); }
}

void IPixmap::readBmp(coord zx, coord zy, uint8* bmp, int row_offset, coord w, coord h, uint color, bool set) noexcept
{
	if (zx < 0)
	{
		w += zx;
		bmp -= zx / 8; // TODO: need x0 offset in draw_hline_bmp()
		zx -= zx;
	}
	if (zy < 0)
	{
		h += zy;
		bmp -= zy * row_offset;
		zy -= zy;
	}
	w = min(w, width - zx);
	h = min(h, height - zy);

	read_bmp(zx, zy, bmp, row_offset, w, h, color, set);
}


} // namespace kio::Graphics


/*
  
























*/
