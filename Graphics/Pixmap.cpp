// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Pixmap.h"


namespace kio::Graphics
{

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
			pixmap + zy * row_offset, row_offset, zx, q.pixmap + qy * q.row_offset, q.row_offset, qx, w, h);
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
			pixmap + zy * row_offset, row_offset, zx, q.pixmap + qy * q.row_offset, q.row_offset, qx, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::drawBmp(coord zx, coord zy, const Bitmap& q, uint color, uint /*ink*/) noexcept
{
	const uint8* qp = q.pixmap;
	coord		 w	= min(q.width, width - zx);
	coord		 h	= min(q.height, height - zy);

	if (unlikely(zx < 0))
	{
		assert((zx & 7) == 0);
		w += zx;
		qp -= zx >> 3;
		zx -= zx;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		qp -= zy * q.row_offset;
		zy -= zy;
	}

	if (w > 0 && h > 0)
		bitblit::draw_bitmap<colordepth>(pixmap + zy * row_offset, row_offset, zx, qp, q.row_offset, w, h, color);
}


// instantiate everything.
// the linker will know what we need:

template class Pixmap<colormode_i1>;
template class Pixmap<colormode_i2>;
template class Pixmap<colormode_i4>;
template class Pixmap<colormode_i8>;
template class Pixmap<colormode_rgb>;


} // namespace kio::Graphics
