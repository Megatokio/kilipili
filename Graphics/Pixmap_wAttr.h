// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap.h"


namespace kilipili
{

// color attributes assign colors to a rectangular cell of pixels.
// color attributes are used to disguise the fact that we don't have enough memory
// to store an individual color for each pixel.
// then the pixels are still high-res but the color isn't.
// the pixels can be 1 bit or 2 bit per pixel, allowing 2 or 4 colors per attribute cell.

// attribute mode with 1 bit per pixel to provide 2 colors per attribute cell:
// attribute width 1..4 pixel may be used for proportional text and horizontal scrolling.
using Pixmap_a1w1 = Pixmap<1, 1, 1>; // 1 bit/pixel, 1 pixel/attr, true color attribute. very cpu intensive! deprecated
using Pixmap_a1w2 = Pixmap<1, 1, 2>; // 1 bit/pixel, 2 pixel/attr, true color attribute. very cpu intensive!
using Pixmap_a1w4 = Pixmap<1, 1, 4>; // 1 bit/pixel, 4 pixel/attr, true color attribute. very cpu intensive!
using Pixmap_a1w8 = Pixmap<1, 1, 8>; // 1 bit/pixel, 8 pixel/attr, true color attribute. THIS IS THE RECOMMENDED MODE!

// attribute mode with 2 bit per pixel to provide 4 colors per attribute cell:
using Pixmap_a2w1 = Pixmap<1, 2, 1>; // 2 bit/pixel, 1 pixel/attr, true color attribute. very cpu intensive! deprecated
using Pixmap_a2w2 = Pixmap<1, 2, 2>; // 2 bit/pixel, 2 pixel/attr, true color attribute. very cpu intensive!
using Pixmap_a2w4 = Pixmap<1, 2, 4>; // 2 bit/pixel, 4 pixel/attr, true color attribute. very cpu intensive!
using Pixmap_a2w8 = Pixmap<1, 2, 8>; // 2 bit/pixel, 8 pixel/attr, true color attribute. very cpu intensive!


/***************************************************************************
				Template for the Pixmaps with Attributes
************************************************************************** */

template<int bpp, int aw>
class Pixmap<1, bpp, aw> final : public Pixmap<0, bpp, true>
{
	static_assert(bpp == 1 || bpp == 2);
	static_assert(aw == 1 || aw == 2 || aw == 4 || aw == 8);

public:
	static constexpr AttrMode	AM = AttrMode(msbit(bpp)); // 0 .. 1  log2 of bits per pixel in pixmap[]
	static constexpr AttrWidth	AW = AttrWidth(msbit(aw)); // 0 .. 3  log2 of width of color cells
	static constexpr ColorDepth CD = ColorDepth(color_ss); // 0 .. 5  log2 of bits per color in attributes[]

	static constexpr bool		isa_Pixmap_wAttr = true; // --> FrameBuffer
	static constexpr bool		is_attr_mode	 = true;
	static constexpr bool		is_indexed_color = false;
	static constexpr bool		is_colormode_rgb = false;
	static constexpr int		bits_per_pixel	 = bpp;					 // 1 .. 2  bits per pixel in pixmap[]
	static constexpr ColorDepth colordepth		 = ColorDepth(color_ss); // 0 .. 5  log2 of bits per color
	static constexpr ColorMode	colormode		 = ColorMode(colormode_a1w1 + AM * 4 + AW);

	using Canvas::attrheight;
	using Canvas::height;
	using Canvas::is_inside;
	using Canvas::size;
	using Canvas::width;
	using super = Pixmap<0, bpp, true>;
	using super::pixmap;
	using super::row_offset; // in pixels[]

	super&		 as_super() { return *this; }
	const super& as_super() const { return *this; }

	Pixmap_rgb attributes; // pixmap with color attributes


	// allocating ctor:
	Pixmap(const Size& size, AttrHeight ah = attrheight_12px) throws;
	Pixmap(coord w, coord h, AttrHeight ah = attrheight_12px) throws;

	// not allocating: wrap existing pixels:
	Pixmap(const Size& size, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept;
	Pixmap(coord w, coord h, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept;

	// window into another pixmap:
	Pixmap(Pixmap& q, const Rect& r) noexcept;
	Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept;
	Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept;

	// window into this pixmap:
	virtual Pixmap* createWindow(coord x, coord y, coord w, coord h) throws override;

	virtual ~Pixmap() noexcept override = default;

	bool operator==(const Pixmap& other) const noexcept;

	// _______________________________________________________________________________________
	// overrides for Canvas virtual drawing methods:

	virtual void set_pixel(coord x, coord y, uint color, uint ink = 1) noexcept override;
	virtual uint get_pixel(coord x, coord y, uint* ink) const noexcept override;
	virtual uint get_color(coord x, coord y) const noexcept override;
	virtual uint get_ink(coord x, coord y) const noexcept override;

	virtual void draw_hline_to(coord x, coord y, coord w, uint color, uint ink = 1) noexcept override;
	virtual void draw_vline_to(coord x, coord y, coord h, uint color, uint ink = 1) noexcept override;
	virtual void fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 1) noexcept override;
	virtual void xorRect(coord x, coord y, coord w, coord h, uint xor_color) noexcept override;
	virtual void clear(uint color) noexcept override;
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
	void draw_bmp(coord x, coord y, const uint8*, int ro, coord w, coord h, uint color, uint = 1) noexcept;
	void draw_char(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink = 1) noexcept;

	using super::drawBmp;
	void drawBmp(coord x, coord y, const Bitmap&, uint color, uint ink = 1) noexcept;

	using super::copyRect;
	void copyRect(coord x, coord y, const Pixmap& src, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(coord x, coord y, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Rect& q) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Point& q, const Size&) noexcept;

	//TODO private:
	// _______________________________________________________________________________________
	// helper:
	// calculate (minimum) sizes for the pixels and attributes pixmap:
	static constexpr int calc_row_offset(int w) noexcept { return super::calc_row_offset(w); } // min.
	static constexpr int calc_attr_width(int w) noexcept { return (w + (1 << AW) - 1) >> AW << bpp; }
	static constexpr int calc_attr_row_offset(int w) noexcept { return calc_attr_width(w) << CD >> 3; } // min.
	static constexpr int calc_attr_height(int h, int ah) noexcept { return (h + ah - 1) / ah; }

	// get attribute x/y for pixel x/y
	static constexpr int calc_ax(int x) noexcept { return x >> AW << bpp; } // attributes[x] for ink = 0
	int					 calc_ay(int y) const noexcept { return y / attrheight; }

	uint attr_get_color(coord x, coord y, uint ink) const noexcept;
	void attr_set_color(coord x, coord y, uint color, uint ink) noexcept;
	void attr_draw_hline(coord x, coord y, coord x2, uint color, uint ink) noexcept;
	void attr_draw_vline(coord x, coord y, coord y2, uint color, uint ink) noexcept;
	void attr_fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink) noexcept;
	void attr_xor_rect(coord x, coord y, coord w, coord h, uint color) noexcept;
	void attr_copy_rect(coord x, coord y, coord qx, coord qy, coord w, coord h) noexcept;
};


// ####################################################################
//							Implementations
// ####################################################################


// __________________________________________________________________
// IMPLEMENTATIONS: dedicated ctor, dtor

template<int bpp, int aw>
Pixmap<1, bpp, aw>::Pixmap(coord w, coord h, AttrHeight ah) throws : // allocating
	super(w, h, PmInfo{msbit(bpp),0,msbit(aw),ah}),
	attributes(calc_attr_width(w), calc_attr_height(h, ah))
{
	assert(ah != attrheight_none);
	assert(Canvas::is_allocated);
}

template<int bpp, int aw> // not allocating: wrap existing pixels.
Pixmap<1, bpp, aw>::Pixmap(
	coord w, coord h, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept :
	super(w, h, PmInfo {msbit(bpp), 0, msbit(aw), ah}, pixels, row_offs),
	attributes(calc_attr_width(w), calc_attr_height(h, ah), attr, attr_row_offs)
{
	assert(ah != attrheight_none);
	assert(Canvas::is_allocated == false);
}


// __________________________________________________________________
// IMPLEMENTATIONS: other ctor

template<int bpp, int aw>
Pixmap<1, bpp, aw>::Pixmap(const Size& size, AttrHeight ah) throws :// allocating
	Pixmap(size.width, size.height, ah)
{}

template<int bpp, int aw> // not allocating: wrap existing pixels
Pixmap<1, bpp, aw>::Pixmap(
	const Size& size, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept :
	Pixmap(size.width, size.height, pixels, row_offs, attr, attr_row_offs, ah)
{}

template<int bpp, int aw> // not allocating: window into another pixmap
Pixmap<1, bpp, aw>::Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept :
	super(q, x, y, w, h),
	attributes(q.attributes, calc_ax(x), calc_ay(y), calc_attr_width(w), calc_attr_height(h, q.attrheight))
{
	assert(Canvas::is_attr_mode); // super mustn't clobber this
	assert(x % aw == 0);		  // x must be a multiple of the attribute cell width
	assert(y % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((x * aw) & 7) == 0);  // x must be a multiple of full bytes in pixmap[]  ((tested in super too))
}

template<int bpp, int aw> // not allocating: window into another pixmap.
Pixmap<1, bpp, aw>::Pixmap(Pixmap& q, const Rect& r) noexcept : Pixmap(q, r.left(), r.top(), r.width(), r.height())
{}

template<int bpp, int aw> // not allocating: window into another pixmap.
Pixmap<1, bpp, aw>::Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept :
	Pixmap(q, p.x, p.y, size.width, size.height)
{}

template<int bpp, int aw>
Pixmap<1, bpp, aw>* Pixmap<1, bpp, aw>::createWindow(coord x, coord y, coord w, coord h)
{
	return new Pixmap(*this, x, y, w, h);
}


// __________________________________________________________________
// IMPLEMENTATIONS: helpers

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::attr_set_color(coord x, coord y, uint color, uint ink) noexcept
{
	// set color in attributes[]

	assert((ink & ~pixelmask<super::colordepth>) == 0);

	x = calc_ax(x);
	y = calc_ay(y);
	attributes.set_pixel(x + ink, y, color);
}

template<int bpp, int aw>
uint Pixmap<1, bpp, aw>::attr_get_color(coord x, coord y, uint ink) const noexcept
{
	// get color from attributes[]

	assert((ink & ~pixelmask<super::colordepth>) == 0);

	x = calc_ax(x);
	y = calc_ay(y);
	return attributes.get_ink(x + ink, y);
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::attr_draw_hline(coord x1, coord y1, coord x2, uint color, uint ink) noexcept
{
	assert((ink & ~pixelmask<super::colordepth>) == 0);

	x1 = calc_ax(x1);
	x2 = calc_ax(x2 - 1) + 1;
	y1 = calc_ay(y1);

	bitblit::attr_clear_rect<AM, CD>(attributes.pixmap + y1 * attributes.row_offset, 0, x1 + ink, x2 - x1, 1, color);
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::attr_draw_vline(coord x1, coord y1, coord y2, uint color, uint ink) noexcept
{
	assert((ink & ~pixelmask<super::colordepth>) == 0);

	x1 = calc_ax(x1);
	y1 = calc_ay(y1);
	y2 = calc_ay(y2 - 1) + 1;

	attributes.draw_vline_to(x1 + ink, y1, y2, color);
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::attr_fill_rect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	assert((ink & ~pixelmask<super::colordepth>) == 0);

	int x2 = calc_ax(x1 + w - 1); // inner bounds
	int y2 = calc_ay(y1 + h - 1);

	x1 = calc_ax(x1);
	y1 = calc_ay(y1);

	w = x2 - x1 + 1; // +1 is enough to include 1 color from the next attribute cell
	h = y2 - y1 + 1;

	assert(x1 >= 0);
	assert(y1 >= 0);
	assert(x1 + w <= attributes.width);
	assert(y1 + h <= attributes.height);

	bitblit::attr_clear_rect<AM, CD>(
		attributes.pixmap + y1 * attributes.row_offset, attributes.row_offset, x1 + ink, w, h, color);
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::attr_xor_rect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	int x2 = calc_ax(x1 + w - 1); // inner bounds
	int y2 = calc_ay(y1 + h - 1);

	x1 = calc_ax(x1);
	y1 = calc_ay(y1);

	w = x2 - x1 + (1 << (1 << AM));
	h = y2 - y1 + 1;

	attributes.xor_rect(x1, y1, w, h, xor_color);
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::attr_copy_rect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	TODO(); // TODO unused untested remove

	w = calc_ax(zx + w - 1) - zx + (1 << bpp); // colors_per_attr;
	h = calc_ay(zy + h - 1) - zy + 1;

	zx = calc_ax(zx);
	zy = calc_ay(zy);
	qx = calc_ax(qx);
	qy = calc_ay(qy);

	bitblit::copy_rect<CD>(
		attributes.pixmap + zy * attributes.row_offset, attributes.row_offset, zx, //
		attributes.pixmap + qy * attributes.row_offset, attributes.row_offset, qx, //
		w, h);
}


// __________________________________________________________________
// IMPLEMENTATIONS: operator ==

template<int bpp, int aw>
bool Pixmap<1, bpp, aw>::operator==(const Pixmap& other) const noexcept
{
	assert(size == other.size);
	assert(attrheight == other.attrheight);

	const super& base = *this;
	return base == other && attributes == other.attributes;
}


// __________________________________________________________________
// IMPLEMENTATIONS: set pixel, get pixel

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::set_pixel(coord x, coord y, uint color, uint ink) noexcept
{
	attr_set_color(x, y, color, ink);
	super::set_pixel(x, y, ink);
}

template<int bpp, int aw>
uint Pixmap<1, bpp, aw>::get_ink(coord x, coord y) const noexcept
{
	// get ink from pixels[]
	// same as in Pixmap.getInk()
	return super::get_ink(x, y);
}

template<int bpp, int aw>
uint Pixmap<1, bpp, aw>::get_color(coord x, coord y) const noexcept
{
	// get color from attributes[]
	uint ink = super::get_ink(x, y);
	return attr_get_color(x, y, ink);
}

template<int bpp, int aw>
uint Pixmap<1, bpp, aw>::get_pixel(coord x, coord y, uint* ink) const noexcept
{
	// get color and ink
	return attr_get_color(x, y, *ink = super::get_ink(x, y));
}


// __________________________________________________________________
// IMPLEMENTATIONS: draw line, fill rect

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::draw_hline_to(coord x1, coord y1, coord x2, uint color, uint ink) noexcept
{
	if (x1 < x2)
	{
		assert(is_inside(x1, y1));
		assert(x2 <= width);

		attr_draw_hline(x1, y1, x2, color, ink);
		super::draw_hline_to(x1, y1, x2, ink);
	}
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::draw_vline_to(coord x1, coord y1, coord y2, uint color, uint ink) noexcept
{
	if (y1 < y2)
	{
		assert(is_inside(x1, y1));
		assert(y2 <= height);

		attr_draw_vline(x1, y1, y2, color, ink);
		super::draw_vline_to(x1, y1, y2, ink);
	}
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::fill_rect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));

		attr_fill_rect(x1, y1, w, h, color, ink);
		super::fill_rect(x1, y1, w, h, ink);
	}
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::xor_rect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(x1, y1));
		assert(is_inside(x1 + w - 1, y1 + h - 1));

		attr_xor_rect(x1, y1, w, h, xor_color);
	}
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::copy_rect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy the pixels from a rectangular area within the same pixmap.
	// overlapping areas are handled safely.

	if (w > 0 && h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + w - 1, zy + h - 1));
		assert(q.is_inside(qx, qy));
		assert(q.is_inside(qx + w - 1, qy + h - 1));

		assert(zx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
		assert(zy % attrheight == 0);  // y must be a multiple of the attribute cell height
		assert(((zx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

		assert(qx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
		assert(qy % attrheight == 0);  // y must be a multiple of the attribute cell height
		assert(((qx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

		super::copy_rect(zx, zy, q.as_super(), qx, qy, w, h);
		zx = calc_ax(zx);
		zy = calc_ay(zy);
		qx = calc_ax(qx);
		qy = calc_ay(qy);
		w  = calc_ax(zx + w - 1) + 1 - zx;
		h  = calc_ay(zy + h - 1) + 1 - zy;
		attributes.copy_rect(zx, zy, q.attributes, qx, qy, w, h);
	}
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::draw_bmp(
	coord zx, coord zy, const uint8* bmp, int bmp_row_offs, coord w, coord h, uint color, uint ink) noexcept
{
	if (w > 0 && h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + w - 1, zy + h - 1));

		attr_fill_rect(zx, zy, w, h, color, ink);
		super::draw_bmp(zx, zy, bmp, bmp_row_offs, w, h, ink);
	}
}

template<int bpp, int aw>
void Pixmap<1, bpp, aw>::draw_char(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint ink) noexcept
{
	// optimized version of drawBmp:
	//   row_offset = 1
	//   width = 8
	//   x = N * 8

	if (h > 0)
	{
		assert(is_inside(zx, zy));
		assert(is_inside(zx + 8 - 1, zy + h - 1));
		assert((zx & 7) == 0);

		attr_fill_rect(zx, zy, 8, h, color, ink);
		super::draw_char(zx, zy, bmp, h, ink);
	}
}


extern template class Pixmap<1, 1, 1>;
extern template class Pixmap<1, 1, 2>;
extern template class Pixmap<1, 1, 4>;
extern template class Pixmap<1, 1, 8>;
extern template class Pixmap<1, 2, 1>;
extern template class Pixmap<1, 2, 2>;
extern template class Pixmap<1, 2, 4>;
extern template class Pixmap<1, 2, 8>;


} // namespace kilipili


/*





























  
*/
