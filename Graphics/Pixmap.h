// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BitBlit.h"
#include "Canvas.h"
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
class DirectColorPixmap : public Canvas
{
public:
	const int	 row_offset; // in pixmap[]
	uint8* const pixmap;

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

	virtual ~Pixmap() noexcept override;

	// -----------------------------------------------

	bool operator==(const Pixmap& other) const noexcept;


	// overrides for Canvas virtual drawing methods:

	virtual void set_pixel(coord x, coord y, uint color, uint ink = 0) noexcept override;
	virtual uint get_pixel(coord x, coord y, uint* ink) const noexcept override;
	virtual uint get_color(coord x, coord y) const noexcept override;
	virtual uint get_ink(coord x, coord y) const noexcept override;

	virtual void draw_hline(coord x, coord y, coord w, uint color, uint ink = 0) noexcept override;
	virtual void draw_vline(coord x, coord y, coord h, uint color, uint ink = 0) noexcept override;
	virtual void fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink = 0) noexcept override;
	virtual void copy_rect(coord x, coord y, coord qx, coord qy, coord w, coord h) noexcept override;
	virtual void copy_rect(coord x, coord y, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept override;
	//virtual void read_bmp(coord x, coord y, uint8*, int roffs, coord w, coord h, uint c, uint = 0) noexcept override;
	virtual void draw_bmp(coord x, coord y, const uint8*, int ro, coord w, coord h, uint c, uint = 0) noexcept override;
	virtual void draw_char(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink = 0) noexcept override;
	virtual void xor_rect(coord x, coord y, coord w, coord h, uint xor_color) noexcept override;

	// non-overrides:

	void copy_rect(coord x, coord y, const Pixmap& src) noexcept;
	void copy_rect(coord x, coord y, const Pixmap& src, coord qx, coord qy, coord w, coord h) noexcept;
	void draw_bmp(coord x, coord y, const Bitmap& src, uint color, uint /*ink*/ = 0) noexcept;

	void copyRect(coord x, coord y, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src) noexcept;

	void copyRect(coord x, coord y, const Pixmap& src, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Rect& q) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Point& q, const Size&) noexcept;

	void drawBmp(coord x, coord y, const Bitmap& q, uint color, uint /*ink*/ = 0) noexcept;
	void drawBmp(const Point& z, const Bitmap& q, uint color, uint /*ink*/ = 0) noexcept;

protected:
	Pixmap(coord w, coord h, ColorMode, AttrHeight) throws;
	Pixmap(coord w, coord h, ColorMode, AttrHeight, uint8* pixels, int row_offset) noexcept;
};


// ####################################################################
//							Implementations
// ####################################################################


// __________________________________________________________________
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
DirectColorPixmap::Pixmap(coord w, coord h, ColorMode cm, AttrHeight ah) throws :
	Canvas(w, h, cm, ah, true /*allocated*/),
	row_offset(calc_row_offset(CD, w)),
	pixmap(new uint8[uint(h * row_offset)])
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(coord w, coord h, AttrHeight ah) throws : Pixmap(w, h, CM, attrheight_none)
{
	// attrheight argument is only for signature compatibility with Pixmap_wAttr version:
	assert(ah == attrheight_none);
}

template<ColorMode CM>
DirectColorPixmap::Pixmap(const Size& sz, AttrHeight ah) throws : Pixmap(sz.width, sz.height, CM, attrheight_none)
{
	// attrheight argument is only for signature compatibility with Pixmap_wAttr version:
	assert(ah == attrheight_none);
}


// not allocating: wrap existing pixels:
template<ColorMode CM>
DirectColorPixmap::Pixmap(coord w, coord h, ColorMode cm, AttrHeight ah, uint8* pixels, int row_offset) noexcept :
	Canvas(w, h, cm, ah, false /*not allocated*/),
	row_offset(row_offset),
	pixmap(pixels)
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(coord w, coord h, uint8* pixels, int row_offset) noexcept :
	Pixmap(w, h, CM, attrheight_none, pixels, row_offset)
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(const Size& sz, uint8* pixels, int row_offset) noexcept :
	Pixmap(sz.width, sz.height, CM, attrheight_none, pixels, row_offset)
{}

// window into other pixmap:
template<ColorMode CM>
DirectColorPixmap::Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept :
	Canvas(w, h, q.Canvas::colormode, q.Canvas::attrheight, false /*not allocated*/),
	row_offset(q.row_offset),
	pixmap(q.pixmap + y * q.row_offset + (bits_for_pixels(CD, x) >> 3))
{
	assert(x >= 0 && w >= 0 && x + w <= q.width);
	assert(y >= 0 && h >= 0 && y + h <= q.height);
	assert(bits_for_pixels(CD, x) % 8 == 0);
}

template<ColorMode CM>
DirectColorPixmap::Pixmap(Pixmap& q, const Rect& r) noexcept : //
	Pixmap(q, r.left(), r.top(), r.width(), r.height())
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept :
	Pixmap(q, p.x, p.y, size.width, size.height)
{}

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


// __________________________________________________________________
// IMPLEMENTATIONS: operator ==

template<ColorMode CM>
bool DirectColorPixmap::operator==(const Pixmap& other) const noexcept
{
	assert(size == other.size);

	const uint8* q = pixmap;
	const uint8* z = other.pixmap;
	for (int y = 0; y < height; y++)
	{
		if (bitblit::compare_row<colordepth>(q, z, width) != 0) return false;
		q += row_offset;
		z += other.row_offset;
	}
	return true;
}


// __________________________________________________________________
// IMPLEMENTATIONS: set pixel, get pixel

template<ColorMode CM>
void DirectColorPixmap::set_pixel(coord x, coord y, uint color, uint /*ink*/) noexcept
{
	// set pixels[] and attributes[]
	//set_ink(x, y, color);
	assert(is_inside(x, y));
	bitblit::set_pixel<colordepth>(pixmap + y * row_offset, x, color);
}

template<ColorMode CM>
uint DirectColorPixmap::get_ink(coord x, coord y) const noexcept
{
	// get value from pixels[]
	assert(is_inside(x, y));
	return bitblit::get_pixel<colordepth>(pixmap + y * row_offset, x);
}

template<ColorMode CM>
uint DirectColorPixmap::get_color(coord x, coord y) const noexcept
{
	// get value from attributes[]
	return get_ink(x, y);
}

template<ColorMode CM>
uint DirectColorPixmap::get_pixel(coord x, coord y, uint* ink) const noexcept
{
	// get color and ink
	return *ink = get_ink(x, y);
}


// __________________________________________________________________
// IMPLEMENTATIONS: draw line, fill rect

template<ColorMode CM>
void DirectColorPixmap::draw_hline(coord x, coord y, coord w, uint color, uint /*ink*/) noexcept
{
	// draw nothing if w <= 0  (draw left-to-right only)

	assert(is_inside(x, y));
	assert(x + w <= width);

	if (w > 0) bitblit::draw_hline<colordepth>(pixmap + y * row_offset, x, w, color);
}

template<ColorMode CM>
void DirectColorPixmap::draw_vline(coord x, coord y, coord h, uint color, uint /*ink*/) noexcept
{
	// draw nothing if h <= 0  (draw top-down only)

	assert(is_inside(x, y));
	assert(y + h <= height);

	if (h > 0) bitblit::draw_vline<colordepth>(pixmap + y * row_offset, row_offset, x, h, color);
}

template<ColorMode CM>
void DirectColorPixmap::fill_rect(coord x, coord y, coord w, coord h, uint color, uint /*ink*/) noexcept
{
	// draw nothing if w <= 0 or h <= 0!

	if (unlikely(w <= 0 || h <= 0)) return;

	assert(is_inside(x, y));
	assert(is_inside(x + w - 1, y + h - 1));

	bitblit::clear_rect_of_bits(
		pixmap + y * row_offset, // start of top row
		x << colordepth,		 // x-offset measured in bits
		row_offset,				 // row offset
		w << colordepth,		 // width measured in bits
		h,						 // height measured in rows
		flood_filled_color<colordepth>(color));
}

template<ColorMode CM>
void DirectColorPixmap::xor_rect(coord x, coord y, coord w, coord h, uint xor_color) noexcept
{
	// draw nothing if w <= 0 or h <= 0!

	if (unlikely(w <= 0 || h <= 0)) return;

	assert(is_inside(x, y));
	assert(is_inside(x + w - 1, y + h - 1));

	bitblit::xor_rect_of_bits(
		pixmap + y * row_offset, // start of top row
		x << colordepth,		 // x-offset measured in bits
		row_offset,				 // row offset
		w << colordepth,		 // width measured in bits
		h,						 // height measured in rows
		flood_filled_color<colordepth>(xor_color));
}

//template<ColorMode CM>
//void DirectColorPixmap::clear(uint color, uint /*ink*/) noexcept
//{
//	bitblit::clear_rect_of_bits(
//		pixmap,			 // start of top row
//		0,				 // x-offset measured in bits
//		row_offset,		 // row offset
//		row_offset << 3, // width measured in bits
//		height,			 // height measured in rows
//		flood_filled_color<colordepth>(color));
//}


// __________________________________________________________________
// IMPLEMENTATION: copy rect areas

template<ColorMode CM>
void DirectColorPixmap::copy_rect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	if (unlikely(w <= 0 || h <= 0)) return;

	assert(is_inside(zx, zy));
	assert(is_inside(zx + w - 1, zy + h - 1));
	assert(is_inside(qx, qy));
	assert(is_inside(qx + w - 1, qy + h - 1));

	bitblit::copy_rect<colordepth>(
		pixmap + zy * row_offset, row_offset, zx, pixmap + qy * row_offset, row_offset, qx, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copy_rect(coord x, coord y, const Pixmap& q) noexcept
{
	const coord w = q.width, h = q.height;

	assert(w > 0 && w > 0);
	assert(is_inside(x, y));
	assert(is_inside(x + w - 1, y + h - 1));

	bitblit::copy_rect<colordepth>(pixmap + y * row_offset, row_offset, x, q.pixmap, q.row_offset, 0, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copy_rect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	if (unlikely(w <= 0 || h <= 0)) return;

	assert(is_inside(zx, zy));
	assert(is_inside(zx + w - 1, zy + h - 1));
	assert(q.is_inside(qx, qy));
	assert(q.is_inside(qx + w - 1, qy + h - 1));

	bitblit::copy_rect<colordepth>(
		pixmap + zy * row_offset, row_offset, zx, q.pixmap + qy * q.row_offset, q.row_offset, qx, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copy_rect(coord zx, coord zy, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(CM == q.colormode); // must be same type
	copy_rect(zx, zy, static_cast<const Pixmap&>(q), qx, qy, w, h);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& z, const Pixmap& pm) noexcept
{
	copyRect(z.x, z.y, pm);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& zpos, const Pixmap& pm, const Point& qpos, const Size& size) noexcept
{
	copyRect(zpos.x, zpos.y, pm, qpos.x, qpos.y, size.width, size.height);
}

template<ColorMode CM>
void DirectColorPixmap::copyRect(const Point& zpos, const Pixmap& pm, const Rect& qrect) noexcept
{
	copyRect(zpos.x, zpos.y, pm, qrect.left(), qrect.top(), qrect.width(), qrect.height());
}


// __________________________________________________________________
// IMPLEMENTATION: draw bitmap & char

template<ColorMode CM>
void DirectColorPixmap::draw_bmp(
	coord x, coord y, const uint8* bmp, int bmp_row_offset, int w, int h, uint color, uint /*ink*/) noexcept
{
	bitblit::draw_bitmap<colordepth>(pixmap + y * row_offset, row_offset, x, bmp, bmp_row_offset, w, h, color);
}

template<ColorMode CM>
void DirectColorPixmap::draw_bmp(coord x, coord y, const Bitmap& bmp, uint color, uint /*ink*/) noexcept
{
	bitblit::draw_bitmap<colordepth>(
		pixmap + y * row_offset, row_offset, x, bmp.pixmap, bmp.row_offset, bmp.width, bmp.height, color);
}

template<ColorMode CM>
void DirectColorPixmap::drawBmp(const Point& z, const Bitmap& bmp, uint color, uint /*ink*/) noexcept
{
	drawBmp(z.x, z.y, bmp, color);
}

template<ColorMode CM>
void DirectColorPixmap::draw_char(coord x, coord y, const uint8* bmp, coord h, uint color, uint /*ink*/) noexcept
{
	bitblit::draw_char<colordepth>(pixmap + y * row_offset, row_offset, x, bmp, h, color);
}


extern template class Pixmap<colormode_i1>;
extern template class Pixmap<colormode_i2>;
extern template class Pixmap<colormode_i4>;
extern template class Pixmap<colormode_i8>;
extern template class Pixmap<colormode_rgb>;

} // namespace kio::Graphics


/*





























*/
