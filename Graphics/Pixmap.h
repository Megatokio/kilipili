// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BitBlit.h"
#include "ColorMap.h"
#include "geometry.h"


#ifndef PIXMAP_ALIGN_TO_INT32
  #define PIXMAP_ALIGN_TO_INT32 0
#endif


namespace kio::Graphics
{

/*
	Template class Pixmap defines a pixmap / pixmap with attributes for ColorMode CM.

	The base class defines PixMaps for direct color modes (without attributes) and
	a subclass defines PixMaps for modes with color attributes.
	The PixMaps know nothing about colors. They just handle pixels and color attributes of varying sizes.

	The PixMaps provide basic drawing methods:
	- Direct color PixMaps:
	  - drawing with a `color` .
	  - compatibility methods with `pixel value` and `color` which ignore the pixel value.

	- Attribute color PixMaps:
	  - drawing with a `pixel value` and a `color`.
		The `pixel value` goes into the pixels[] while the `color` goes into the attributes[].
	  - compatibility methods without `color` which draw into the pixmap[] only.
*/
template<ColorMode CM, typename = void>
class Pixmap;

// handy names:
using Bitmap	 = Pixmap<colormode_i1>;
using Pixmap_i1	 = Pixmap<colormode_i1>;
using Pixmap_i2	 = Pixmap<colormode_i2>;
using Pixmap_i4	 = Pixmap<colormode_i4>;
using Pixmap_i8	 = Pixmap<colormode_i8>;
using Pixmap_rgb = Pixmap<colormode_rgb>;


// how ugly can it be?
#define DirectColorPixmap Pixmap<CM, typename std::enable_if_t<is_direct_color(CM)>>


/***************************************************************************
				Template for the direct color PixMaps
************************************************************************** */

template<ColorMode CM>
class DirectColorPixmap
{
public:
	union
	{
		const Size size;
		struct
		{
			const coord width, height;
		};
	};

	const int	 row_offset; // in pixmap[]
	uint8* const pixmap;
	const bool	 allocated; // whether pixmap[] was allocated and will be deleted in dtor

	static constexpr ColorDepth CD			   = get_colordepth(CM); // 0 .. 4  log2 of bits per color
	static constexpr AttrMode	AM			   = attrmode_none;		 // 0 .. 2  log2 of bits per color (dummy)
	static constexpr AttrWidth	AW			   = attrwidth_none;	 // 0 .. 3  log2 of width of tiles (dummy)
	static constexpr int		bits_per_color = 1 << CD; // bits per color in pixmap[] (wAttr: in attributes[])
	static constexpr int		bits_per_pixel = 1 << CD; // bits per pixel in pixmap[]

	static constexpr ColorDepth colordepth = CD;

	// for compatibility with Pixmap_wAttr:
	static constexpr AttrMode	attrmode   = attrmode_none;
	static constexpr AttrWidth	attrwidth  = attrwidth_none;
	static constexpr AttrHeight attrheight = attrheight_none;


	// allocating ctor:
	Pixmap(const Size& size, AttrHeight = attrheight_none) throws;
	Pixmap(coord w, coord h, AttrHeight = attrheight_none) throws;

	// not allocating: wrap existing pixels:
	Pixmap(const Size& size, uint8* pixels, int row_offset) noexcept;
	Pixmap(coord w, coord h, uint8* pixels, int row_offset) noexcept;

	// window into other pixmap:
	Pixmap(Pixmap& q, const Rect& r) noexcept;
	Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept;
	Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept;

	// create Bitmap from Pixmap
	//   set = true:  bits in Bitmap are set  if pixel matches (foreground) color
	//   set = false: bits in Bitmap are null if pixel matches (background) color
	template<ColorMode QCM>
	Pixmap(typename std::enable_if_t<CM == colormode_i1, const Pixmap<QCM>>& q, uint color, bool set) noexcept;

	~Pixmap() noexcept;

	bool operator==(const Pixmap& other) const noexcept;

	// basic drawing methods:

	uint getPixel(coord x, coord y) const noexcept;
	uint getPixel(const Point& p) const noexcept;

	uint getColor(coord x, coord y) const noexcept { return getPixel(x, y); }
	uint getColor(const Point& p) const noexcept { return getPixel(p); }

	void setPixel(coord x, coord y, uint color, uint = 0) noexcept;
	void setPixel(const Point& p, uint color, uint = 0) noexcept;

	void drawHorLine(coord x, coord y, coord x2, uint color, uint = 0) noexcept;
	void drawHorLine(const Point& p, coord x2, uint color, uint = 0) noexcept;

	void drawVertLine(coord x, coord y, coord y2, uint color, uint = 0) noexcept;
	void drawVertLine(const Point& p, coord y2, uint color, uint = 0) noexcept;

	void clear(uint color) noexcept; // whole pixmap
	void fillRect(coord x, coord y, coord w, coord h, uint color, uint = 0) noexcept;
	void fillRect(const Rect& r, uint color, uint = 0) noexcept;

	void copy(const Pixmap& source) noexcept;
	void copyRect(coord x, coord y, const Pixmap& q) noexcept;
	void copyRect(coord x, coord y, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(const Point& z, const Pixmap& q) noexcept;
	void copyRect(const Point& z, const Pixmap& q, const Point& p, const Size& s) noexcept;
	void copyRect(const Point& z, const Pixmap& q, const Rect& r) noexcept;
	void copyRect(const Point& z, const Rect& r) noexcept;
	void copyRect(const Point& z, const Point& q, const Size& s) noexcept;

	void drawBmp(coord x, coord y, const Bitmap& bmp, uint color, uint = 0) noexcept;
	void drawBmp(const Point& z, const Bitmap& bmp, uint color, uint = 0) noexcept;
	void drawChar(coord x, coord y, const uint8* bmp, int h, uint color, uint = 0) noexcept;
	void drawChar(const Point& z, const uint8* bmp, int h, uint color, uint = 0) noexcept;

	void calcRectForAttr(Rect& r) noexcept; // declared but never defined. only the PixmapWithAttr version is.

	// helper:
	bool is_inside(coord x, coord y) const noexcept { return uint(x) < uint(width) && (uint(y) < uint(height)); }

	// no checking and ordered coordinates only:
	uint get_pixel(coord x, coord y) const noexcept;
	uint get_color(coord x, coord y) const noexcept { return get_pixel(x, y); }
	void set_pixel(coord x, coord y, uint color) noexcept;
	void set_pixel(coord x, coord y, uint color, uint /*pixel*/) noexcept { set_pixel(x, y, color); }
	void draw_hline(coord x, coord y, coord x2, uint color) noexcept;
	void draw_hline(coord x, coord y, coord x2, uint color, uint /*pixel*/) noexcept { draw_hline(x, y, x2, color); }
	void draw_vline(coord x, coord y, coord y2, uint color) noexcept;
	void draw_vline(coord x, coord y, coord y2, uint color, uint /*pixel*/) noexcept { draw_vline(x, y, y2, color); }
	void fill_rect(coord x, coord y, coord w, coord h, uint color) noexcept;
	void fill_rect(coord x, coord y, coord w, coord h, uint color, uint) noexcept { fill_rect(x, y, w, h, color); }
	void copy_rect(coord x, coord y, const Pixmap& source) noexcept;
	void copy_rect(coord x, coord y, const Pixmap& source, coord qx, coord qy, coord w, coord h) noexcept;
	void draw_bmp(coord x, coord y, const Bitmap&, uint color) noexcept;
	void draw_bmp(coord x, coord y, const Bitmap& bmp, uint color, uint) noexcept { draw_bmp(x, y, bmp, color); }
	void draw_bmp(coord x, coord y, const uint8* bmp, int bmp_row_offset, int w, int h, uint color) noexcept;
	void draw_bmp(coord x, coord y, const uint8* bmp, int bmp_ro, int w, int h, uint color, uint /*pixel*/) noexcept
	{
		draw_bmp(x, y, bmp, bmp_ro, w, h, color);
	}
	void draw_char(coord x, coord y, const uint8* bmp, int height, uint color) noexcept;
	void draw_char(coord x, coord y, const uint8* bmp, int height, uint color, uint /*pixel*/) noexcept
	{
		draw_char(x, y, bmp, height, color);
	}
};


// #############################################################
// IMPLEMENTATIONS: ctor, dtor

inline constexpr int bits_for_pixels(ColorDepth CD, int w) noexcept // ctor helper
{
	return w << CD;
}

inline constexpr int calc_row_offset(ColorDepth CD, int w) noexcept // ctor helper
{
	if (PIXMAP_ALIGN_TO_INT32) return (bits_for_pixels(CD, w) + 31) >> 5 << 2;
	if ((w << CD) >= (80 << 3)) return (bits_for_pixels(CD, w) + 31) >> 5 << 2;
	else return (bits_for_pixels(CD, w) + 7) >> 3;
}

// allocating, throws:
template<ColorMode CM>
DirectColorPixmap::Pixmap(const Size& size, AttrHeight) throws : Pixmap(size.width, size.height)
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(coord w, coord h, AttrHeight) throws :
	width(w),
	height(h),
	row_offset(calc_row_offset(CD, w)),
	pixmap(new uint8[uint(h * row_offset)]),
	allocated(true)
{}

// not allocating: wrap existing pixels:
template<ColorMode CM>
DirectColorPixmap::Pixmap(const Size& size, uint8* pixels, int row_offset) noexcept :
	Pixmap(size.width, size.height, pixels, row_offset)
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(coord w, coord h, uint8* pixels, int row_offset) noexcept :
	width(w),
	height(h),
	row_offset(row_offset),
	pixmap(pixels),
	allocated(false)
{}

// window into other pixmap:
template<ColorMode CM>
DirectColorPixmap::Pixmap(Pixmap& q, const Rect& r) noexcept : Pixmap(q, r.left(), r.top(), r.width(), r.height())
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept :
	Pixmap(q, p.x, p.y, size.width, size.height)
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept :
	size(w, h),
	row_offset(q.row_offset),
	pixmap(q.pixmap + y * q.row_offset + (bits_for_pixels(CD, x) >> 3)),
	allocated(false)
{
	assert(x >= 0 && w >= 0 && x + w <= q.width);
	assert(y >= 0 && h >= 0 && y + h <= q.height);
	assert(bits_for_pixels(CD, x) % 8 == 0);
}

// create Bitmap from Pixmap:
template<ColorMode CM>
template<ColorMode QCM>
DirectColorPixmap::Pixmap(
	typename std::enable_if_t<CM == colormode_i1, const Pixmap<QCM>>& q, uint color, bool set) noexcept :
	Pixmap(q.width, q.height)
{
	bitblit::copy_rect_as_bitmap<ColorDepth(QCM)>(
		pixmap, row_offset, q.pixmap, q.row_offset, q.width, q.height, color, set);
}

template<ColorMode CM>
DirectColorPixmap::~Pixmap() noexcept
{
	if (allocated) delete[] pixmap;
}


// #############################################################
// IMPLEMENTATIONS: setPixel(), getPixel()

template<ColorMode CM>
uint DirectColorPixmap::get_pixel(coord x, coord y) const noexcept
{
	return bitblit::get_pixel<colordepth>(pixmap + y * row_offset, x);
}

template<ColorMode CM>
uint DirectColorPixmap::getPixel(coord x, coord y) const noexcept
{
	return is_inside(x, y) ? get_pixel(x, y) : 0;
}

template<ColorMode CM>
uint DirectColorPixmap::getPixel(const Point& p) const noexcept
{
	return getPixel(p.x, p.y);
}

template<ColorMode CM>
void DirectColorPixmap::set_pixel(coord x, coord y, uint color) noexcept
{
	bitblit::set_pixel<colordepth>(pixmap + y * row_offset, x, color);
}

template<ColorMode CM>
void DirectColorPixmap::setPixel(coord x, coord y, uint color, uint) noexcept
{
	if (is_inside(x, y)) set_pixel(x, y, color);
}

template<ColorMode CM>
void DirectColorPixmap::setPixel(const Point& p, uint color, uint) noexcept
{
	setPixel(p.x, p.y, color);
}


// #############################################################
// IMPLEMENTATIONS: drawHorLine(), drawVertLine()

template<ColorMode CM>
void DirectColorPixmap::draw_hline(coord x, coord y, coord x2, uint color) noexcept
{
	// draws nothing if x1 == x2

	assert(uint(y) < uint(height));
	assert(0 <= x && x <= x2 && x2 <= width);

	if (x2 > x) bitblit::draw_hline<colordepth>(pixmap + y * row_offset, x, x2 - x, color);
}

template<ColorMode CM>
void DirectColorPixmap::draw_vline(coord x, coord y, coord y2, uint color) noexcept
{
	// draws nothing if y1 == y2

	assert(uint(x) < uint(width));
	assert(0 <= y && y <= y2 && y2 <= height);

	bitblit::draw_vline<colordepth>(pixmap + y * row_offset, row_offset, x, y2 - y, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawHorLine(int x, int y, int x2, uint color, uint) noexcept
{
	// draw hor line between x1 and x2.
	// draws abs(x2-x1) pixels.
	// draws nothing if x1==x2.

	if (uint(y) >= uint(height)) return;
	sort(x, x2);
	x  = max(x, 0);
	x2 = min(x2, width);
	if (x2 > x) bitblit::draw_hline<colordepth>(pixmap + y * row_offset, x, x2 - x, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawHorLine(const Point& p, coord x2, uint color, uint) noexcept
{
	drawHorLine(p.x, p.y, x2, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawVertLine(coord x, coord y, coord y2, uint color, uint) noexcept
{
	// draw vert line between y1 and y2.
	// draws abs(y2-y1) pixels.
	// draws nothing if y1==y2.

	if (uint(x) >= uint(width)) return;
	sort(y, y2);
	y  = max(y, 0);
	y2 = min(y2, height);
	bitblit::draw_vline<colordepth>(pixmap + y * row_offset, row_offset, x, y2 - y, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawVertLine(const Point& p, coord y2, uint color, uint) noexcept
{
	drawVertLine(p.x, p.y, y2, color);
}


// #############################################################
// IMPLEMENTATIONS: clear()

template<ColorMode CM>
void DirectColorPixmap::clear(uint color) noexcept
{
	bitblit::clear_rect_of_bits(
		pixmap,			 // start of top row
		0,				 // x-offset measured in bits
		row_offset,		 // row offset
		row_offset << 3, // width measured in bits
		height,			 // height measured in rows
		flood_filled_color<colordepth>(color));
}

template<ColorMode CM>
void DirectColorPixmap::fill_rect(int x, int y, int w, int h, uint color) noexcept
{
	// draws nothing for empty rect!

	assert(0 <= x && w > 0 && x + w <= width);
	assert(0 <= y && h > 0 && y + h <= height);

	bitblit::clear_rect_of_bits(
		pixmap + y * row_offset, // start of top row
		x << colordepth,		 // x-offset measured in bits
		row_offset,				 // row offset
		w << colordepth,		 // width measured in bits
		h,						 // height measured in rows
		flood_filled_color<colordepth>(color));
}

template<ColorMode CM>
void DirectColorPixmap::fillRect(int x, int y, int w, int h, uint color, uint) noexcept
{
	// draw filled rectangle.
	// nothing is drawn for empty rect!

	x = max(x, 0);
	w = min(w, width - x);
	y = max(y, 0);
	h = min(h, height - y);

	if (w > 0 && h > 0)
		bitblit::clear_rect_of_bits(
			pixmap + y * row_offset, // start of top row
			x << colordepth,		 // x-offset measured in bits
			row_offset,				 // row offset
			w << colordepth,		 // width measured in bits
			h,						 // height measured in rows
			flood_filled_color<colordepth>(color));
}

template<ColorMode CM>
void DirectColorPixmap::fillRect(const Rect& r, uint color, uint) noexcept
{
	fillRect(r.left(), r.top(), r.width(), r.height(), color);
}


// #############################################################
// IMPLEMENTATIONS: copy()

template<ColorMode CM>
void DirectColorPixmap::copy(const Pixmap& q) noexcept
{
	assert(width == q.width); // TODO: crop? scale? ignore?
	assert(height == q.height);

	bitblit::copy_rect<CD>(pixmap, row_offset, 0, q.pixmap, q.row_offset, 0, width, height);
}

template<ColorMode CM>
void DirectColorPixmap::copy_rect(coord x, coord y, const Pixmap& q) noexcept
{
	assert(x >= 0 && q.width > 0 && x + q.width <= width);
	assert(y >= 0 && q.height > 0 && y + q.height <= height);

	bitblit::copy_rect<CD>(pixmap + y * row_offset, row_offset, x, q.pixmap, q.row_offset, 0, q.width, q.height);
}

template<ColorMode CM>
void DirectColorPixmap::copy_rect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(w > 0 && h > 0);
	assert(0 <= qx && qx + w <= q.width);
	assert(0 <= qy && qy + h <= q.height);
	assert(zx >= 0 && zx + w <= width);
	assert(zy >= 0 && zy + h <= height);

	bitblit::copy_rect<CD>(
		pixmap + zy * row_offset, row_offset, zx, q.pixmap + qy * q.row_offset, q.row_offset, qx, w, h);
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
		qx -= zx;
		w += zx;
		zx = 0;
	}
	if (unlikely(zy < 0))
	{
		qy -= zy;
		h += zy;
		zy = 0;
	}

	if (unlikely(zx + w > width)) { w = width - qx; }
	if (unlikely(zy + h > height)) { h = height - qy; }

	if (w > 0 && h > 0)
		bitblit::copy_rect<CD>(
			pixmap + zy * row_offset, row_offset, zx, q.pixmap + qy * q.row_offset, q.row_offset, qx, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	if (unlikely(qx < 0))
	{
		zx -= qx;
		w += qx;
		qx = 0;
	}
	if (unlikely(qy < 0))
	{
		zy -= qy;
		h += qy;
		qy = 0;
	}
	if (unlikely(zx < 0))
	{
		qx -= zx;
		w += zx;
		zx = 0;
	}
	if (unlikely(zy < 0))
	{
		qy -= zy;
		h += zy;
		zy = 0;
	}

	if (unlikely(qx + w > q.width)) { w = q.width - qx; }
	if (unlikely(qy + h > q.height)) { h = q.height - qy; }
	if (unlikely(zx + w > width)) { w = width - zx; }
	if (unlikely(zy + h > height)) { h = height - zy; }

	if (w > 0 && h > 0)
		bitblit::copy_rect<CD>(
			pixmap + zy * row_offset, row_offset, zx, q.pixmap + qy * q.row_offset, q.row_offset, qx, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& z, const Pixmap& q) noexcept
{
	copyRect(z.x, z.y, q);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& z, const Pixmap& q, const Point& p, const Size& s) noexcept
{
	copyRect(z.x, z.y, q, p.x, p.y, s.width, s.height);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& z, const Pixmap& q, const Rect& r) noexcept
{
	copyRect(z.x, z.y, q, r.left(), r.top(), r.width(), r.height());
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& z, const Rect& r) noexcept
{
	copyRect(z.x, z.y, *this, r.left(), r.top(), r.width(), r.height());
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& z, const Point& q, const Size& s) noexcept
{
	copyRect(z.x, z.y, *this, q.x, q.y, s.width, s.height);
}


// #############################################################
// IMPLEMENTATIONS: draw_bmp(), draw_char()

template<ColorMode CM>
void DirectColorPixmap::draw_bmp(
	coord x, coord y, const uint8* bmp, int bmp_row_offset, int w, int h, uint color) noexcept
{
	bitblit::draw_bitmap<colordepth>(pixmap + y * row_offset, row_offset, x, bmp, bmp_row_offset, w, h, color);
}

template<ColorMode CM>
void DirectColorPixmap::draw_bmp(coord x, coord y, const Bitmap& bmp, uint color) noexcept
{
	bitblit::draw_bitmap<colordepth>(
		pixmap + y * row_offset, row_offset, x, bmp.pixmap, bmp.row_offset, bmp.width, bmp.height, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawBmp(coord x, coord y, const Bitmap& q, uint color, uint) noexcept
{
	const uint8* qp = q.pixmap;
	coord		 w	= q.width;
	coord		 h	= q.height;

	if (unlikely(w > width - x)) { w = width - x; }
	if (unlikely(h > height - y)) { h = height - y; }

	if (unlikely(x < 0))
	{
		assert((x & 7) == 0);
		qp -= x >> 3;
		w += x;
		x = 0;
	}
	if (unlikely(y < 0))
	{
		qp -= y * q.row_offset;
		h += y;
		y = 0;
	}

	if (w > 0 && h > 0)
		bitblit::draw_bitmap<colordepth>(pixmap + y * row_offset, row_offset, x, qp, q.row_offset, w, h, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawBmp(const Point& z, const Bitmap& bmp, uint color, uint) noexcept
{
	drawBmp(z.x, z.y, bmp, color);
}

template<ColorMode CM>
void DirectColorPixmap::draw_char(coord x, coord y, const uint8* bmp, int height, uint color) noexcept
{
	bitblit::draw_char<colordepth>(pixmap + y * row_offset, row_offset, x, bmp, height, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawChar(coord x, coord y, const uint8* bmp, int h, uint color, uint) noexcept
{
	constexpr int bmp_row_offs = 1;
	constexpr int w			   = 8;

	assert(h > 0);

	if (unlikely(x < 0 || x > width - w))
	{
		drawBmp(x, y, Bitmap(w, h, const_cast<uint8*>(bmp), bmp_row_offs), color);
		return;
	}

	if (unlikely(h > height - y)) { h = height - y; }
	if (unlikely(y < 0))
	{
		bmp -= y * bmp_row_offs;
		h += y;
		y = 0;
	}

	bitblit::draw_char<colordepth>(pixmap + y * row_offset, row_offset, x, bmp, height, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawChar(const Point& z, const uint8* bmp, int h, uint color, uint) noexcept
{
	drawChar(z.x, z.y, bmp, h, color);
}


// #############################################################
// IMPLEMENTATIONS: operator ==

template<ColorMode CM>
bool DirectColorPixmap::operator==(const Pixmap& other) const noexcept
{
	assert(size == other.size);

	const uint8* q = pixmap;
	const uint8* z = other.pixmap;
	for (int y = 0; y < height; y++)
	{
		if (bitblit::compare_row<CD>(q, z, width) != 0) return false;
		q += row_offset;
		z += other.row_offset;
	}
	return true;
}


} // namespace kio::Graphics
