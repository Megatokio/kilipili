// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Pixmap.h"


namespace kio::Graphics
{

template<ColorMode CM>
void DirectColorPixmap::fillRect(coord x1, coord y1, coord w, coord h, uint color, uint /*ink*/) noexcept
{
	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);

	fill_rect(x1, y1, x2 - x1, y2 - y1, color);
}

template<ColorMode CM>
void DirectColorPixmap::xorRect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);

	xor_rect(x1, y1, x2 - x1, y2 - y1, xor_color);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	if (unlikely(qx < 0))
	{
		w += qx;
		zx -= qx;
		qx -= qx;
	}
	if (unlikely(qy < 0))
	{
		h += qy;
		zy -= qy;
		qy -= qy;
	}
	if (unlikely(zx < 0))
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}
	w = min(w, width - zx, q.width - qx);
	h = min(h, height - zy, q.height - qy);

	if (w > 0 && h > 0)
		bitblit::copy_rect<colordepth>(
			pixmap + zy * row_offset, row_offset, zx,		//
			q.pixmap + qy * q.row_offset, q.row_offset, qx, //
			w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(coord zx, coord zy, const Pixmap& q) noexcept
{
	coord qx = 0;
	coord qy = 0;
	coord w	 = q.width;
	coord h	 = q.height;

	if (unlikely(zx < 0))
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}
	w = min(w, width - zx);
	h = min(h, height - zy);

	if (w > 0 && h > 0)
		bitblit::copy_rect<colordepth>(
			pixmap + zy * row_offset, row_offset, zx,		//
			q.pixmap + qy * q.row_offset, q.row_offset, qx, //
			w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	copyRect(zx, zy, *this, qx, qy, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(coord zx, coord zy, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(CM == q.colormode); // must be same type
	copyRect(zx, zy, static_cast<const Pixmap&>(q), qx, qy, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& z, const Pixmap& q) noexcept
{
	copyRect(z.x, z.y, q, 0, 0, q.width, q.height);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& zp, const Pixmap& pm, const Point& qp, const Size& size) noexcept
{
	copyRect(zp.x, zp.y, pm, qp.x, qp.y, size.width, size.height);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& zp, const Pixmap& pm, const Rect& qr) noexcept
{
	copyRect(zp.x, zp.y, pm, qr.left(), qr.top(), qr.width(), qr.height());
}

template<ColorMode CM>
void DirectColorPixmap::drawBmp(
	coord zx, coord zy, const uint8* bmp, int bmp_row_offset, int w, int h, uint color, uint /*ink*/) noexcept
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
		bitblit::draw_bitmap<colordepth>(
			pixmap + zy * row_offset, // start of the first row in destination Pixmap
			row_offset,				  // row offset in destination Pixmap measured in bytes
			zx,						  // x offset from zp in pixels
			bmp,					  // start of the first byte of source Bitmap
			bmp_row_offset,			  // row offset in source Bitmap measured in bytes
			w,						  // width in pixels
			h,						  // height in pixels
			color);					  // color for drawing the bitmap
}

template<ColorMode CM>
void DirectColorPixmap::drawBmp(coord zx, coord zy, const Bitmap& bmp, uint color, uint /*ink*/) noexcept
{
	drawBmp(zx, zy, bmp.pixmap, bmp.row_offset, bmp.width, bmp.height, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawChar(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint /*ink*/) noexcept
{
	// optimized version of drawBmp:
	//   row_offset = 1
	//   width = 8
	//   x = N * 8

	if (unlikely(zx < 0 || zx >= width) || (zx & 7) != 0) return drawBmp(zx, zy, bmp, 1, 8, h, color);

	if (unlikely(zy < 0))
	{
		h += zy;
		bmp -= zy;
		zy -= zy;
	}
	h = min(h, height - zy);

	if (h > 0) bitblit::draw_char<colordepth>(pixmap + zy * row_offset, row_offset, zx, bmp, h, color);
}


// instantiate everything.
// the linker will know what we need:

template class Pixmap<colormode_i1>;
template class Pixmap<colormode_i2>;
template class Pixmap<colormode_i4>;
template class Pixmap<colormode_i8>;
#if VIDEO_COLOR_PIN_COUNT > 8
template class Pixmap<colormode_rgb>;
#endif


} // namespace kio::Graphics
