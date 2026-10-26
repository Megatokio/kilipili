// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BitBlit.h"
#include "Canvas.h"
#include "geometry.h"


namespace kilipili
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
template<int bpp, int aw, bool ic>
class Pixmap;

// handy names:
using Bitmap	 = Pixmap<1, 0, true>;
using Pixmap_i1	 = Pixmap<1, 0, true>;
using Pixmap_i2	 = Pixmap<2, 0, true>;
using Pixmap_i4	 = Pixmap<4, 0, true>;
using Pixmap_i8	 = Pixmap<8, 0, true>;
using Pixmap_rgb = Pixmap<color_bits, 0, false>;


/***************************************************************************
				Template for the direct color Pixmaps
************************************************************************** */

template<int bpp, bool ic>
class Pixmap<bpp, 0, ic> : public Canvas
{
	static_assert(8 % bpp == 0 || bpp == color_bits);

public:
	static constexpr ColorDepth CD				 = ColorDepth(msbit(bpp)); // 0 .. 5  log2 of bits per pixel
	static constexpr bool		is_attr_mode	 = false;
	static constexpr bool		is_indexed_color = ic;
	static constexpr bool		is_colormode_rgb = ic == false && bpp == color_bits;
	static constexpr ColorMode	colormode		 = is_colormode_rgb ? colormode_rgb : ColorMode(CD);

	const int			 row_offset; // in pixmap[]
	uint8* const		 pixmap;	 //
	RCPtr<ColorMap<bpp>> colormap;	 // if indexed color


	// helper: calculate (minimum) sizes for the pixels pixmap:
	static constexpr int calc_row_offset(int w) noexcept { return ((w << CD) + 7) >> 3; }

	// allocating ctor:
	Pixmap(const Size& size, ColorMap<bpp>* = nullptr) throws;
	Pixmap(coord w, coord h, ColorMap<bpp>* = nullptr) throws;

	// not allocating: wrap existing pixels:
	constexpr Pixmap(const Size& size, uint8* pixels, int row_offset, ColorMap<bpp>* = nullptr) noexcept;
	constexpr Pixmap(coord w, coord h, uint8* pixels, int row_offset, ColorMap<bpp>* = nullptr) noexcept;

	// window into other pixmap:
	Pixmap(Pixmap& q, const Rect& r) noexcept;
	Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept;
	Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept;

	// create Bitmap from Pixmap
	//   set = true:  bits in Bitmap are set  if pixel matches (foreground) color
	//   set = false: bits in Bitmap are null if pixel matches (background) color
	template<int a, bool b>
	Pixmap(typename std::enable_if_t<bpp == 1, const Pixmap<a, 0, b>>& q, uint color, bool set) noexcept;

	// window into this pixmap:
	virtual Pixmap* createWindow(coord x, coord y, coord w, coord h) throws override;

	virtual ~Pixmap() noexcept override;

	// -----------------------------------------------

	bool operator==(const Pixmap& other) const noexcept;


	// _______________________________________________________________________________________
	// overrides for Canvas virtual drawing methods:

	virtual void set_pixel(coord x, coord y, uint color, uint ink = 1) noexcept override;
	virtual uint get_pixel(coord x, coord y, uint* ink) const noexcept override;
	virtual uint get_color(coord x, coord y) const noexcept override;
	virtual uint get_ink(coord x, coord y) const noexcept override;

	virtual ColorMap<bpp>* getColorMap() const noexcept override;

	virtual void draw_hline_to(coord x, coord y, coord x2, uint color, uint ink = 1) noexcept override;
	virtual void draw_vline_to(coord x, coord y, coord y2, uint color, uint ink = 1) noexcept override;
	virtual void fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 1) noexcept override;
	virtual void xorRect(coord x, coord y, coord w, coord h, uint xor_color) noexcept override;
	using Canvas::clear; //virtual void clear(uint color) noexcept override;
	virtual void copyRect(coord x, coord y, coord qx, coord qy, coord w, coord h) noexcept override;
	virtual void copyRect(coord x, coord y, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept override;
	//virtual void readBmp(coord x, coord y, uint8* bmp, int row_offs, coord w, coord h, uint c, bool) noexcept override;
	virtual void drawBmp(coord x, coord y, const uint8*, int ro, coord w, coord h, uint c, uint = 1) noexcept override;
	virtual void drawChar(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink = 1) noexcept override;

	// _______________________________________________________________________________________
	// non-overrides:

	void fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink = 1) noexcept;
	void xor_rect(coord x, coord y, coord w, coord h, uint xor_color) noexcept;
	void copy_rect(coord x, coord y, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept;
	void draw_bmp(coord x, coord y, const uint8*, int ro, coord w, coord h, uint c, uint = 1) noexcept;
	void draw_char(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink = 1) noexcept;

	using Canvas::drawBmp;
	void drawBmp(coord x, coord y, const Bitmap&, uint c, uint = 1) noexcept;

	using Canvas::copyRect;
	void copyRect(coord x, coord y, const Pixmap& src, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(coord x, coord y, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Rect& q) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Point& q, const Size&) noexcept;

protected:
	constexpr Pixmap(coord w, coord h, const PmInfo&, ColorMap<bpp>* = nullptr) throws;
	constexpr Pixmap(coord w, coord h, const PmInfo&, uint8* pixels, int row_offset, ColorMap<bpp>* = nullptr) noexcept;
};


// ####################################################################
//							Implementations
// ####################################################################


// __________________________________________________________________
// IMPLEMENTATIONS: dedicated ctor, dtor
// also for use by subclass
// these don't copy the colormap!

// allocating:
template<int bpp, bool ic>
constexpr Pixmap<bpp, 0, ic>::Pixmap(coord w, coord h, const PmInfo& pminfo, ColorMap<bpp>* cmap) throws :
	Canvas(w, h, pminfo, true),
	row_offset(calc_row_offset(w)),
	pixmap(new uint8[uint(h * row_offset)]),
	colormap(cmap)
{}

// not allocating: wrap existing pixels.
template<int bpp, bool ic>
constexpr Pixmap<bpp, 0, ic>::Pixmap(
	coord w, coord h, const PmInfo& pminfo, uint8* pixels, int row_offs, ColorMap<bpp>* cmap) noexcept :
	Canvas(w, h, pminfo, false),
	row_offset(row_offs),
	pixmap(pixels),
	colormap(cmap)
{}

template<int bpp, bool ic>
Pixmap<bpp, 0, ic>::~Pixmap() noexcept
{
	if (is_allocated) delete[] pixmap;
}


// __________________________________________________________________
// IMPLEMENTATIONS: other ctor

template<int bpp, bool ic>
Pixmap<bpp, 0,ic>::Pixmap(coord w, coord h, ColorMap<bpp>* cmap) throws : // allocating
	Pixmap(w, h, PmInfo{msbit(bpp),ic}, cmap)
{}

template<int bpp, bool ic>
Pixmap<bpp, 0,ic>::Pixmap(const Size& sz, ColorMap<bpp>* cmap) throws : // allocating
	Pixmap(sz.width, sz.height, PmInfo{msbit(bpp),ic}, cmap)
{}

template<int bpp, bool ic> // not allocating
constexpr Pixmap<bpp, 0, ic>::Pixmap(coord w, coord h, uint8* pixels, int row_offset, ColorMap<bpp>* cmap) noexcept :
	Pixmap(w, h, PmInfo {msbit(bpp), ic}, pixels, row_offset, cmap)
{}

template<int bpp, bool ic> // not allocating
constexpr Pixmap<bpp, 0, ic>::Pixmap(const Size& sz, uint8* pixels, int row_offset, ColorMap<bpp>* cmap) noexcept :
	Pixmap(sz.width, sz.height, PmInfo {msbit(bpp), ic}, pixels, row_offset, cmap)
{}

template<int bpp, bool ic>
Pixmap<bpp, 0, ic>::Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept : // window into other pixmap
	Pixmap(
		w, h, PmInfo(q.ss_bpp, q.ss_aw, q.attrheight, q.is_indexed_color),
		q.pixmap + y * q.row_offset + ((x << CD) >> 3), q.row_offset, q.colormap)
{
	assert(x >= 0 && w >= 0 && x + w <= q.width);
	assert(y >= 0 && h >= 0 && y + h <= q.height);
	assert((x << CD) % 8 == 0);
}

template<int bpp, bool ic>
Pixmap<bpp, 0, ic>::Pixmap(Pixmap& q, const Rect& r) noexcept : // window into other pixmap
	Pixmap(q, r.left(), r.top(), r.width(), r.height())
{}

template<int bpp, bool ic>
Pixmap<bpp, 0, ic>::Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept : // window into other pixmap
	Pixmap(q, p.x, p.y, size.width, size.height)
{}

template<int bpp, bool ic>
Pixmap<bpp, 0, ic>* Pixmap<bpp, 0, ic>::createWindow(coord x, coord y, coord w, coord h) // window into this pixmap
{
	return new Pixmap(*this, x, y, w, h);
}

template<int bpp, bool ic> // create Bitmap from Pixmap:
template<int a, bool b>
Pixmap<bpp, 0, ic>::Pixmap(typename std::enable_if_t<bpp == 1, const Pixmap<a, 0, b>>& q, uint color, bool set) noexcept
	:
	Pixmap(q.width, q.height, nullptr /*no colormap*/)
{
	bitblit::copy_rect_as_bitmap<q.CD>(pixmap, row_offset, q.pixmap, q.row_offset, q.width, q.height, color, set);
}


// __________________________________________________________________
// IMPLEMENTATIONS: operator ==

template<int bpp, bool ic>
bool Pixmap<bpp, 0, ic>::operator==(const Pixmap& other) const noexcept
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


// __________________________________________________________________
// IMPLEMENTATIONS: set pixel, get pixel, getColorMap:

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::set_pixel(coord x, coord y, uint color, uint /*ink*/) noexcept
{
	// set pixels[] and attributes[]
	//set_ink(x, y, color);
	assert(is_inside(x, y));
	bitblit::set_pixel<CD>(pixmap + y * row_offset, x, color);
}

template<int bpp, bool ic>
uint Pixmap<bpp, 0, ic>::get_ink(coord x, coord y) const noexcept
{
	// get value from pixels[]
	assert(is_inside(x, y));
	return bitblit::get_pixel<CD>(pixmap + y * row_offset, x);
}

template<int bpp, bool ic>
uint Pixmap<bpp, 0, ic>::get_color(coord x, coord y) const noexcept
{
	// get value from attributes[]
	return get_ink(x, y);
}

template<int bpp, bool ic>
uint Pixmap<bpp, 0, ic>::get_pixel(coord x, coord y, uint* ink) const noexcept
{
	// get color and ink
	return *ink = get_ink(x, y);
}

template<int bpp, bool ic>
ColorMap<bpp>* Pixmap<bpp, 0, ic>::getColorMap() const noexcept
{
	return colormap;
}


// __________________________________________________________________
// IMPLEMENTATIONS: lines, rects, bmps & chars

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::draw_hline_to(coord x1, coord y1, coord x2, uint color, uint /*ink*/) noexcept
{
	if (x1 < x2)
	{
		assert(is_inside(x1, y1));
		assert(x2 <= width);

		bitblit::draw_hline<CD>(pixmap + y1 * row_offset, x1, x2 - x1, color);
	}
}

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::draw_vline_to(coord x1, coord y1, coord y2, uint color, uint /*ink*/) noexcept
{
	if (y1 < y2)
	{
		assert(is_inside(x1, y1));
		assert(y2 <= height);

		bitblit::draw_vline<CD>(pixmap + y1 * row_offset, row_offset, x1, y2 - y1, color);
	}
}

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::fill_rect(coord x1, coord y1, coord w, coord h, uint color, uint /*ink*/) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));
	}

	bitblit::clear_rect<CD>(pixmap + y1 * row_offset, row_offset, x1, w, h, color);
}

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::xor_rect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));
	}

	bitblit::xor_rect<CD>(pixmap + y1 * row_offset, row_offset, x1, w, h, xor_color);
}

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::copy_rect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy the pixels from a rectangular area within the same pixmap.
	// overlapping areas are handled safely.

	if (w > 0 && h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + w - 1, zy + h - 1));
		assert(q.is_inside(qx, qy));
		assert(q.is_inside(qx + w - 1, qy + h - 1));

		bitblit::copy_rect<CD>(
			pixmap + zy * row_offset, row_offset, zx,		//
			q.pixmap + qy * q.row_offset, q.row_offset, qx, //
			w, h);
	}
}

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::draw_bmp(
	coord zx, coord zy, const uint8* bmp, int bmp_row_offs, coord w, coord h, uint color, uint /*ink*/) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + w - 1, zy + h - 1));

		bitblit::draw_bitmap<CD>(
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

template<int bpp, bool ic>
void Pixmap<bpp, 0, ic>::draw_char(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint /*ink*/) noexcept
{
	if (h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + 8 - 1, zy + h - 1));
		assert((zx & 7) == 0);

		bitblit::draw_char<CD>(pixmap + zy * row_offset, row_offset, zx, bmp, h, color);
	}
}


extern template class Pixmap<1, 0, true>;
extern template class Pixmap<2, 0, true>;
extern template class Pixmap<4, 0, true>;
extern template class Pixmap<8, 0, true>;
extern template class Pixmap<color_bits, 0, false>;

} // namespace kilipili


/*





























*/
