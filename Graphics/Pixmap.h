// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BitBlit.h"
#include "Canvas.h"
#include "geometry.h"


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
using Pixmap_i16 = Pixmap<colormode_i16>;


// how ugly can it be?
#define DirectColorPixmap Pixmap<CM, typename std::enable_if_t<is_direct_color(CM)>>


/***************************************************************************
				Template for the direct color Pixmaps
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
	static constexpr ColorMode	colormode  = CM;
	static constexpr AttrMode	attrmode   = attrmode_none;
	static constexpr AttrWidth	attrwidth  = attrwidth_none;
	static constexpr AttrHeight attrheight = attrheight_none;

	// helper: calculate (minimum) sizes for the pixels pixmap:
	static constexpr int calc_row_offset(int w) noexcept { return ((w << CD) + 7) >> 3; }

	// allocating ctor:
	Pixmap(const Size& size, AttrHeight = attrheight_none) throws;
	Pixmap(coord w, coord h, AttrHeight = attrheight_none) throws;

	// not allocating: wrap existing pixels:
	constexpr Pixmap(const Size& size, uint8* pixels, int row_offset) noexcept;
	constexpr Pixmap(coord w, coord h, uint8* pixels, int row_offset) noexcept;

	// window into other pixmap:
	Pixmap(Pixmap& q, const Rect& r) noexcept;
	Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept;
	Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept;

	// create Bitmap from Pixmap
	//   set = true:  bits in Bitmap are set  if pixel matches (foreground) color
	//   set = false: bits in Bitmap are null if pixel matches (background) color
	template<ColorMode QCM>
	Pixmap(typename std::enable_if_t<CM == colormode_i1, const Pixmap<QCM>>& q, uint color, bool set) noexcept;

	virtual constexpr ~Pixmap() noexcept override;

	// -----------------------------------------------

	bool operator==(const Pixmap& other) const noexcept;


	// _______________________________________________________________________________________
	// overrides for Canvas virtual drawing methods:

	virtual void set_pixel(coord x, coord y, uint color, uint ink = 0) noexcept override;
	virtual uint get_pixel(coord x, coord y, uint* ink) const noexcept override;
	virtual uint get_color(coord x, coord y) const noexcept override;
	virtual uint get_ink(coord x, coord y) const noexcept override;

	virtual void draw_hline_to(coord x, coord y, coord x2, uint color, uint ink = 0) noexcept override;
	virtual void draw_vline_to(coord x, coord y, coord y2, uint color, uint ink = 0) noexcept override;
	virtual void fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 0) noexcept override;
	virtual void xorRect(coord x, coord y, coord w, coord h, uint xor_color) noexcept override;
	//virtual void clear(uint color) noexcept override;
	virtual void copyRect(coord x, coord y, coord qx, coord qy, coord w, coord h) noexcept override;
	virtual void copyRect(coord x, coord y, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept override;
	//virtual void readBmp(coord x, coord y, uint8*, int roffs, coord w, coord h, uint c, uint = 0) noexcept override;
	virtual void drawBmp(coord x, coord y, const uint8*, int ro, coord w, coord h, uint c, uint = 0) noexcept override;
	virtual void drawChar(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink = 0) noexcept override;

	// _______________________________________________________________________________________
	// non-overrides:

	void fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink = 0) noexcept;
	void xor_rect(coord x, coord y, coord w, coord h, uint xor_color) noexcept;
	void copy_rect(coord x, coord y, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept;
	void draw_bmp(coord x, coord y, const uint8*, int ro, coord w, coord h, uint c, uint = 0) noexcept;
	void draw_char(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink = 0) noexcept;

	using Canvas::drawBmp;
	void drawBmp(coord x, coord y, const Bitmap&, uint c, uint = 0) noexcept;

	using Canvas::copyRect;
	void copyRect(coord x, coord y, const Pixmap& src, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(coord x, coord y, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Rect& q) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Point& q, const Size&) noexcept;

protected:
	constexpr Pixmap(coord w, coord h, ColorMode, AttrHeight) throws;
	constexpr Pixmap(coord w, coord h, ColorMode, AttrHeight, uint8* pixels, int row_offset) noexcept;
};


// ####################################################################
//							Implementations
// ####################################################################


// __________________________________________________________________
// IMPLEMENTATIONS: ctor, dtor

// allocating, throws. for use by subclass:
template<ColorMode CM>
constexpr DirectColorPixmap::Pixmap(coord w, coord h, ColorMode cm, AttrHeight ah) throws :
	Canvas(w, h, cm, ah, true /*allocated*/),
	row_offset(calc_row_offset(w)),
	pixmap(new uint8[uint(h * row_offset)])
{}

template<ColorMode CM>
DirectColorPixmap::Pixmap(coord w, coord h, AttrHeight) throws : Pixmap(w, h, CM, attrheight_none)
{
	// argument attrheight is only for signature compatibility with Pixmap_wAttr version
}

template<ColorMode CM>
DirectColorPixmap::Pixmap(const Size& sz, AttrHeight) throws : Pixmap(sz.width, sz.height, CM, attrheight_none)
{
	// argument attrheight is only for signature compatibility with Pixmap_wAttr version
}


// not allocating: wrap existing pixels. for use by subclass:
template<ColorMode CM>
constexpr DirectColorPixmap::Pixmap(
	coord w, coord h, ColorMode cm, AttrHeight ah, uint8* pixels, int row_offset) noexcept :
	Canvas(w, h, cm, ah, false /*not allocated*/),
	row_offset(row_offset),
	pixmap(pixels)
{}

template<ColorMode CM>
constexpr DirectColorPixmap::Pixmap(coord w, coord h, uint8* pixels, int row_offset) noexcept :
	Pixmap(w, h, CM, attrheight_none, pixels, row_offset)
{}

template<ColorMode CM>
constexpr DirectColorPixmap::Pixmap(const Size& sz, uint8* pixels, int row_offset) noexcept :
	Pixmap(sz.width, sz.height, CM, attrheight_none, pixels, row_offset)
{}

// window into other pixmap:
template<ColorMode CM>
DirectColorPixmap::Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept :
	Canvas(w, h, q.Canvas::colormode, q.Canvas::attrheight, false /*not allocated*/),
	row_offset(q.row_offset),
	pixmap(q.pixmap + y * q.row_offset + ((x << CD) >> 3))
{
	assert(x >= 0 && w >= 0 && x + w <= q.width);
	assert(y >= 0 && h >= 0 && y + h <= q.height);
	assert((x << CD) % 8 == 0);
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
constexpr DirectColorPixmap::~Pixmap() noexcept
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
// IMPLEMENTATIONS: lines, rects, bmps & chars

template<ColorMode CM>
void DirectColorPixmap::draw_hline_to(coord x1, coord y1, coord x2, uint color, uint /*ink*/) noexcept
{
	if (x1 < x2)
	{
		assert(is_inside(x1, y1));
		assert(x2 <= width);

		bitblit::draw_hline<colordepth>(pixmap + y1 * row_offset, x1, x2 - x1, color);
	}
}

template<ColorMode CM>
void DirectColorPixmap::draw_vline_to(coord x1, coord y1, coord y2, uint color, uint) noexcept
{
	if (y1 < y2)
	{
		assert(is_inside(x1, y1));
		assert(y2 <= height);

		bitblit::draw_vline<colordepth>(pixmap + y1 * row_offset, row_offset, x1, y2 - y1, color);
	}
}

template<ColorMode CM>
void DirectColorPixmap::fill_rect(coord x1, coord y1, coord w, coord h, uint color, uint /*ink*/) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));
	}

	bitblit::clear_rect<colordepth>(pixmap + y1 * row_offset, row_offset, x1, w, h, color);
}

template<ColorMode CM>
void DirectColorPixmap::xor_rect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));
	}

	bitblit::xor_rect<colordepth>(pixmap + y1 * row_offset, row_offset, x1, w, h, xor_color);
}

template<ColorMode CM>
void DirectColorPixmap::copy_rect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy the pixels from a rectangular area within the same pixmap.
	// overlapping areas are handled safely.

	if (w > 0 && h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + w - 1, zy + h - 1));
		assert(q.is_inside(qx, qy));
		assert(q.is_inside(qx + w - 1, qy + h - 1));

		bitblit::copy_rect<colordepth>(
			pixmap + zy * row_offset, row_offset, zx,		//
			q.pixmap + qy * q.row_offset, q.row_offset, qx, //
			w, h);
	}
}

template<ColorMode CM>
void DirectColorPixmap::draw_bmp(
	coord zx, coord zy, const uint8* bmp, int bmp_row_offs, coord w, coord h, uint color, uint /*ink*/) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + w - 1, zy + h - 1));

		bitblit::draw_bitmap<colordepth>(
			pixmap + zy * row_offset, // start of the first row in destination Pixmap
			row_offset,				  // row offset in destination Pixmap measured in bytes
			zx,						  // x offset from zp in pixels
			bmp,					  // start of the first byte of source Bitmap
			bmp_row_offs,			  // row offset in source Bitmap measured in bytes
			w,						  // width in pixels
			h,						  // height in pixels
			color);					  // color for drawing the bitmap
	}
}

template<ColorMode CM>
void DirectColorPixmap::draw_char(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint /*ink*/) noexcept
{
	if (h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + 8 - 1, zy + h - 1));
		assert((zx & 7) == 0);

		bitblit::draw_char<colordepth>(pixmap + zy * row_offset, row_offset, zx, bmp, h, color);
	}
}


extern template class Pixmap<colormode_i1>;
extern template class Pixmap<colormode_i2>;
extern template class Pixmap<colormode_i4>;
extern template class Pixmap<colormode_i8>;
extern template class Pixmap<colormode_i16>;

} // namespace kio::Graphics


/*





























*/
