// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Canvas.h"


namespace kio::Graphics
{

Canvas::Canvas(coord w, coord h, ColorMode CM, AttrHeight AH, bool allocated) noexcept :
	width(w),
	height(h),
	colormode(CM),
	attrheight(AH),
	allocated(allocated)
{}

void Canvas::draw_hline(coord x1, coord y1, coord w, uint color, uint ink) noexcept
{
	// draw horizontal line
	// no bound test for speed

	assert(x1 >= 0 && x1 + w <= width);
	assert(y1 >= 0 && y1 + 1 <= height);

	while (--w >= 0) set_pixel(x1++, y1, color, ink);
}

void Canvas::drawHLine(coord x1, coord y1, coord w, uint color, uint ink) noexcept
{
	// draw horizontal line

	if (uint(y1) >= uint(height)) return;

	coord x2 = min(x1 + w, width);
	x1		 = max(x1, 0);

	while (x1 < x2) set_pixel(x1++, y1, color, ink);
}

void Canvas::drawVLine(coord x1, coord y1, coord h, uint color, uint ink) noexcept
{
	// draw vertical line

	if (uint(x1) >= uint(width)) return;

	coord y2 = min(y1 + h, height);
	y1		 = max(y1, 0);

	while (y1 < y2) set_pixel(x1, y1++, color, ink);
}

void Canvas::fillRect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	// draw filled rectangle

	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);
	w		 = x2 - x1;

	if (w > 0)
		while (y1 < y2) draw_hline(x1, y1++, w, color, ink);
}

void Canvas::xorRect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	// xor all colors in the rect area with the xorColor

	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);

	for (coord y = y1; y < y2; y++)
	{
		for (coord x = x1; x < x2; x++)
		{
			uint ink, color = get_pixel(x, y, &ink);
			set_pixel(x, y, color ^ xor_color, ink);
		}
	}
}


void Canvas::copyRect(coord zx, coord zy, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy a rectangular area from another pixmap of the same ColorDepth.

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

	while (--h >= 0)
	{
		for (coord i = 0; i < w; i++)
		{
			uint ink, color = get_pixel(qx + i, qy++, &ink);
			set_pixel(zx + i, zy++, color, ink);
		}
	}
}

void Canvas::copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy the pixels from a rectangular area within the same pixmap.
	// overlapping areas are handled safely.

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

	if (zy != qy ? zy < qy : zx < qx) // copy down (towards larger y) -> with incrementing addresses
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
	else // copy up (towards smaller y) -> with decrementing addresses
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

void Canvas::draw_hline_bmp(coord x, coord y, coord w, const uint8* q, uint color, uint ink) noexcept
{
	// helper:
	// draw one line from a bitmap
	// draw the set bits with `color`, skip the zeros

	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + 1 <= height);

	while (w > 0)
	{
		w -= 8;
		uint8 byte = *q++;
		if (w < 0) byte &= 0xff >> -w;

		for (uint m = 1; m < 0x100; m = m << 1)
		{
			if (byte & m) set_pixel(x, y, color, ink);
			x++;
		}
	}
}

void Canvas::drawBmp(
	coord zx, coord zy, const uint8* bmp, int row_offset, coord w, coord h, uint color, uint ink) noexcept
{
	// draw bitmap into Canvas
	// draw the set bits with `color`, skip the zeros

	if (unlikely(zx < 0))
	{
		w += zx;
		bmp -= zx / 8; // TODO: need x0 offset in draw_hline_bmp()
		zx -= zx;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		bmp -= zy * row_offset;
		zy -= zy;
	}
	w = min(w, width - zx);
	h = min(h, height - zy);

	if (w > 0)
		while (--h >= 0)
		{
			draw_hline_bmp(zx, zy++, w, bmp, color, ink);
			bmp += row_offset;
		}
}

void Canvas::drawChar(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint ink) noexcept
{
	// version of drawBmp optimized for:
	//   bmp_row_offset = 1
	//   width = 8
	//   zx = N * 8

	if (unlikely(zx < 0 || zx > width - 8)) return drawBmp(zx, zy, bmp, 1, 8, h, color, ink);

	if (unlikely(zy < 0))
	{
		h += zy;
		bmp -= zy;
		zy -= zy;
	}
	h = min(h, height - zy);

	while (--h >= 0)
	{
		uint8 byte = *bmp++;
		for (int i = 0; byte != 0; byte >>= 1, i++)
		{
			if (byte & 1) set_pixel(zx + i, zy, color, ink);
		}
		zy++;
	}
}

void Canvas::read_hline_bmp(coord x, coord y, coord w, uint8* z, uint color, bool set) noexcept
{
	// helper:
	// read & convert horizontal line to bitmap
	// for attributed pixmaps the bitmap is constructed from the attribute colors, not just from the pixmap pixels.
	// set=0: `color` is a `bgcolor` => clear bit in bmp for pixel == color
	// set=1: `color` is a `fgcolor` => set bit in bmp for pixel == color

	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + 1 <= height);

	while (w > 0)
	{
		uint8 byte = set ? 0 : 0xff;
		for (uint m = 1; m < 0x100; m = m << 1)
		{
			if (get_color(x++, y) == color) byte ^= m;
		}

		w -= 8;
		if (w < 0) byte &= 0xff >> -w;
		*z++ = byte;
	}
}


void Canvas::readBmp(coord zx, coord zy, uint8* bmp, int row_offset, coord w, coord h, uint color, bool set) noexcept
{
	// read bitmap from Canvas.
	// for attributed pixmaps the bitmap is constructed from the attribute colors, not just from the pixmap pixels.
	// set=0: `color` is a `bgcolor` => clear bit in bmp for pixel == color
	// set=1: `color` is a `fgcolor` => set bit in bmp for pixel == color

	if (zx < 0)
	{
		w += zx;
		bmp -= zx / 8; // TODO: need x0 offset in read_hline_bmp()
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

	if (w > 0)
		while (--h >= 0)
		{
			read_hline_bmp(zx, zy++, w, bmp, color, set);
			bmp += row_offset;
		}
}


} // namespace kio::Graphics


/*
  
























*/
