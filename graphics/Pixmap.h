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
	Template class Pixmap defines pixmaps with and without attributes.
	Pixmaps with attributes are based on class Pixmap which is based on the common base class `Canvas`.

	Canvas --> Pixmap --> Pixmap_wAttr.

	template class Pixmap<0,…> defines pixmaps for direct color modes (without color attributes):
	Pixmap<0,bpp,ic>
		0   = no attributes
		bpp = bits per pixel = 1, 2, 4, 8, 16 or 32
		ic  = flag whether the Pixmap intends to use the colormap, or whether it is not `colormode_rgb`.

	template class Pixmap<1,…> defines pixmaps which have an additional color attributes map:
	Pixmap<1,bpp,aw>
		1   = with attributes
		bpp = bits per pixel = 1 or 2
		aw  = attribute width = 1, 2, 4 or 8

	For convenience all supported Pixmaps have an alias name:
	- Bitmap	  = Pixmap<0, 1, true>
	- Pixmap_i1	  = Pixmap<0, 1, true>
	- Pixmap_i2	  = Pixmap<0, 2, true>
	- Pixmap_i4	  = Pixmap<0, 4, true>
	- Pixmap_i8	  = Pixmap<0, 8, true>
	- Pixmap_rgb  = Pixmap<0, color_bits, false>
	- Pixmap_a1w1 = Pixmap<1, 1, 1>
	- Pixmap_a1w2 = Pixmap<1, 1, 2>
	- Pixmap_a1w4 = Pixmap<1, 1, 4>
	- Pixmap_a1w8 = Pixmap<1, 1, 8>
	- Pixmap_a2w1 = Pixmap<1, 2, 1>
	- Pixmap_a2w2 = Pixmap<1, 2, 2>
	- Pixmap_a2w4 = Pixmap<1, 2, 4>
	- Pixmap_a2w8 = Pixmap<1, 2, 8>

	Pixmaps with attributes are used to reduce the amount of memory required for a colorful display dramatically
	but at the cost of a reduced color resolution, most commonly matching the character sized of 8x12 pixels.
	This leads to color clash when drawing to this Pixmap with disregard of this limitation.
	You can adjust the trade-of by using Pixmaps with 1 or 2 bpp for 2 or 4 different colors per attribute cell,
	and attribute width of 1, 2, 4 or 8 pixels and any arbitrary attribute height.

	The pixmaps provide a data member `ColorMap<bits> colormap`, but beyond that know nothing about colors.
	They just handle pixels and color attributes of varying sizes.

	The Pixmap constructors which allocate their pixels take a size and one additional argument:
	- Pixmap<0,…> ( Size, ColorMap<bpp>* )	an optional ColorMap (required for display in a FrameBuffer)
	- Pixmap<1,…> ( Size, AttrHeight )		height of the attributes, normally the character height, default = 12

	Other constructors don't allocate the pixel array but use a provided pixel array:
	- Pixmap<0,…>(Size, pixels*, row_offs, ColorMap*)
	- Pixmap<1,…>(Size, pixels*, row_offs, attr*, row_offs, AttrHeight)
	- Pixmap<>(Pixmap&, x, y, w, h) 
	They don't lock the provided pixel array, so be careful with the life time of them both.

	A special case for a dependent Pixmap is:
	- virtual Canvas* Canvas::createWindow(x,y,w,h)
	which is provided so that the common base class `Canvas` can return a window into a Pixmap
	without actually beeing itself a template for the actual Pixmap<>.

	The Pixmaps provide basic drawing methods.
	They take a `color` and an `ink` argument:

	For Pixmaps without attributes the color is used and the ink is ignored.
	The color is the type and size of the pixels, that is, for pixmap_rgb it is a `Color`
	and for indexed color modes it is presumably the index. But as said, Pixmaps don't know anything about colors…

	For Pixmaps with attributes the `ink` set's the pixels in the pixels array
	and the `color` set's the true `Color` in the attributes map.
	Normally clearing the screen uses ink 0 and drawing is done using ink 1.
	This leads to 'color clash' when drawing pixels in the same attribute area.
	The last drawing command sets the color for all pixels in that attribute.
	As said, you can adjust the trade-of by using Pixmaps with 1 or 2 bpp, and optimizing the attribute size.
	For attribute modes with 1bpp you have 2 Colors per attribute cell and ink 0 to 1 can be used.
	For attribute modes with 2bpp you have 4 Colors per attribute cell and ink 0 to 3 can be used.
*/
template<int wAttr, int bpp, int ic_or_aw>
class Pixmap;

// handy names:
using Bitmap	 = Pixmap<0, 1, true>;
using Pixmap_i1	 = Pixmap<0, 1, true>;
using Pixmap_i2	 = Pixmap<0, 2, true>;
using Pixmap_i4	 = Pixmap<0, 4, true>;
using Pixmap_i8	 = Pixmap<0, 8, true>;
using Pixmap_rgb = Pixmap<0, color_bits, false>;


/***************************************************************************
				Template for the direct color Pixmaps
************************************************************************** */

template<int bpp, int ic>
class Pixmap<0, bpp, ic> : public Canvas
{
	static_assert(8 % bpp == 0 || bpp == color_bits);

public:
	static constexpr bool		is_attr_mode	 = false;
	static constexpr bool		is_indexed_color = ic;
	static constexpr bool		is_colormode_rgb = ic == false && bpp == color_bits;
	static constexpr int		bits_per_pixel	 = bpp;					   // 1 .. 32  bits per pixel
	static constexpr ColorDepth colordepth		 = ColorDepth(msbit(bpp)); // 0 .. 5  log2 of bits per pixel
	static constexpr ColorMode	colormode		 = is_colormode_rgb ? colormode_rgb : ColorMode(colordepth);

	const int			 row_offset; // in pixmap[]
	uint8* const		 pixmap;	 //
	RCPtr<ColorMap<bpp>> colormap;	 // if indexed color


	// helper: calculate (minimum) sizes for the pixels pixmap:
	static constexpr int calc_row_offset(int w) noexcept { return ((w << colordepth) + 7) >> 3; }

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
	Pixmap(typename std::enable_if_t<bpp == 1, const Pixmap<0, a, b>>& q, uint color, bool set) noexcept;

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
template<int bpp, int ic>
constexpr Pixmap<0, bpp, ic>::Pixmap(coord w, coord h, const PmInfo& pminfo, ColorMap<bpp>* cmap) throws :
	Canvas(w, h, pminfo, true),
	row_offset(calc_row_offset(w)),
	pixmap(new uint8[uint(h * row_offset)]),
	colormap(cmap)
{}

// not allocating: wrap existing pixels.
template<int bpp, int ic>
constexpr Pixmap<0, bpp, ic>::Pixmap(
	coord w, coord h, const PmInfo& pminfo, uint8* pixels, int row_offs, ColorMap<bpp>* cmap) noexcept :
	Canvas(w, h, pminfo, false),
	row_offset(row_offs),
	pixmap(pixels),
	colormap(cmap)
{}

template<int bpp, int ic>
Pixmap<0, bpp, ic>::~Pixmap() noexcept
{
	if (is_allocated) delete[] pixmap;
}


// __________________________________________________________________
// IMPLEMENTATIONS: other ctor

template<int bpp, int ic>
Pixmap<0, bpp, ic>::Pixmap(coord w, coord h, ColorMap<bpp>* cmap) throws : // allocating
	Pixmap(w, h, PmInfo{msbit(bpp),ic}, cmap)
{}

template<int bpp, int ic>
Pixmap<0, bpp, ic>::Pixmap(const Size& sz, ColorMap<bpp>* cmap) throws : // allocating
	Pixmap(sz.width, sz.height, PmInfo{msbit(bpp),ic}, cmap)
{}

template<int bpp, int ic> // not allocating
constexpr Pixmap<0, bpp, ic>::Pixmap(coord w, coord h, uint8* pixels, int row_offset, ColorMap<bpp>* cmap) noexcept :
	Pixmap(w, h, PmInfo {msbit(bpp), ic}, pixels, row_offset, cmap)
{}

template<int bpp, int ic> // not allocating
constexpr Pixmap<0, bpp, ic>::Pixmap(const Size& sz, uint8* pixels, int row_offset, ColorMap<bpp>* cmap) noexcept :
	Pixmap(sz.width, sz.height, PmInfo {msbit(bpp), ic}, pixels, row_offset, cmap)
{}

template<int bpp, int ic>
Pixmap<0, bpp, ic>::Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept : // window into other pixmap
	Pixmap(
		w, h, PmInfo(q.ss_bpp, q.is_indexed_color, q.ss_aw, q.attrheight),
		q.pixmap + y * q.row_offset + ((x << colordepth) >> 3), q.row_offset, q.colormap)
{
	//assert(x >= 0 && w >= 0 && x + w <= q.width);
	//assert(y >= 0 && h >= 0 && y + h <= q.height);
	//assert((x << CD) % 8 == 0);

	assert_ge(x, 0);
	assert_ge(w, 0);
	assert_le(x + w, q.width);
	assert_ge(y, 0);
	assert_ge(h, 0);
	assert_le(y + h, q.height);
	assert_eq((x << colordepth) % 8, 0);
}

template<int bpp, int ic>
Pixmap<0, bpp, ic>::Pixmap(Pixmap& q, const Rect& r) noexcept : // window into other pixmap
	Pixmap(q, r.left(), r.top(), r.width(), r.height())
{}

template<int bpp, int ic>
Pixmap<0, bpp, ic>::Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept : // window into other pixmap
	Pixmap(q, p.x, p.y, size.width, size.height)
{}

template<int bpp, int ic>
Pixmap<0, bpp, ic>* Pixmap<0, bpp, ic>::createWindow(coord x, coord y, coord w, coord h) // window into this pixmap
{
	return new Pixmap(*this, x, y, w, h);
}

template<int bpp, int ic> // create Bitmap from Pixmap:
template<int a, bool b>
Pixmap<0, bpp, ic>::Pixmap(typename std::enable_if_t<bpp == 1, const Pixmap<0, a, b>>& q, uint c, bool set) noexcept :
	Pixmap(q.width, q.height, nullptr /*no colormap*/)
{
	bitblit::copy_rect_as_bitmap<q.CD>(pixmap, row_offset, q.pixmap, q.row_offset, q.width, q.height, c, set);
}


// __________________________________________________________________
// IMPLEMENTATIONS: operator ==

template<int bpp, int ic>
bool Pixmap<0, bpp, ic>::operator==(const Pixmap& other) const noexcept
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
// IMPLEMENTATIONS: set pixel, get pixel, getColorMap:

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::set_pixel(coord x, coord y, uint color, uint /*ink*/) noexcept
{
	// set pixels[] and attributes[]
	//set_ink(x, y, color);
	assert(is_inside(x, y));
	bitblit::set_pixel<colordepth>(pixmap + y * row_offset, x, color);
}

template<int bpp, int ic>
uint Pixmap<0, bpp, ic>::get_ink(coord x, coord y) const noexcept
{
	// get value from pixels[]
	assert(is_inside(x, y));
	return bitblit::get_pixel<colordepth>(pixmap + y * row_offset, x);
}

template<int bpp, int ic>
uint Pixmap<0, bpp, ic>::get_color(coord x, coord y) const noexcept
{
	// get value from attributes[]
	return get_ink(x, y);
}

template<int bpp, int ic>
uint Pixmap<0, bpp, ic>::get_pixel(coord x, coord y, uint* ink) const noexcept
{
	// get color and ink
	return *ink = get_ink(x, y);
}

template<int bpp, int ic>
ColorMap<bpp>* Pixmap<0, bpp, ic>::getColorMap() const noexcept
{
	return colormap;
}


// __________________________________________________________________
// IMPLEMENTATIONS: lines, rects, bmps & chars

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::draw_hline_to(coord x1, coord y1, coord x2, uint color, uint /*ink*/) noexcept
{
	if (x1 < x2)
	{
		assert(is_inside(x1, y1));
		assert(x2 <= width);

		bitblit::draw_hline<colordepth>(pixmap + y1 * row_offset, x1, x2 - x1, color);
	}
}

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::draw_vline_to(coord x1, coord y1, coord y2, uint color, uint /*ink*/) noexcept
{
	if (y1 < y2)
	{
		assert(is_inside(x1, y1));
		assert(y2 <= height);

		bitblit::draw_vline<colordepth>(pixmap + y1 * row_offset, row_offset, x1, y2 - y1, color);
	}
}

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::fill_rect(coord x1, coord y1, coord w, coord h, uint color, uint /*ink*/) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));
	}

	bitblit::clear_rect<colordepth>(pixmap + y1 * row_offset, row_offset, x1, w, h, color);
}

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::xor_rect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));
	}

	bitblit::xor_rect<colordepth>(pixmap + y1 * row_offset, row_offset, x1, w, h, xor_color);
}

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::copy_rect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
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

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::draw_bmp(
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

template<int bpp, int ic>
void Pixmap<0, bpp, ic>::draw_char(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint /*ink*/) noexcept
{
	if (h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + 8 - 1, zy + h - 1));
		assert((zx & 7) == 0);

		bitblit::draw_char<colordepth>(pixmap + zy * row_offset, row_offset, zx, bmp, h, color);
	}
}


extern template class Pixmap<0, 1, true>;
extern template class Pixmap<0, 2, true>;
extern template class Pixmap<0, 4, true>;
extern template class Pixmap<0, 8, true>;
extern template class Pixmap<0, color_bits, false>;

} // namespace kilipili


/*





























*/
