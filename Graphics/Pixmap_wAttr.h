// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap.h"


namespace kio::Graphics
{

// handy names:
using PixMap_a1w1i4	 = Pixmap<colormode_a1w1_i4>;  // Pixels = 1bpp, Attribute cell width = 1px, colors = i4
using PixMap_a1w1i8	 = Pixmap<colormode_a1w1_i8>;  // Pixels = 1bpp, Attribute cell width = 1px, colors = i8
using PixMap_a1w1rgb = Pixmap<colormode_a1w1_rgb>; // Pixels = 1bpp, Attribute cell width = 1px, colors = true color
using PixMap_a1w2i4	 = Pixmap<colormode_a1w2_i4>;  // Pixels = 1bpp, Attribute cell width = 2px, colors = i4
using PixMap_a1w2i8	 = Pixmap<colormode_a1w2_i8>;
using PixMap_a1w2rgb = Pixmap<colormode_a1w2_rgb>;
using PixMap_a1w4i4	 = Pixmap<colormode_a1w4_i4>; // Pixels = 1bpp, Attribute cell width = 4px, colors = i4
using PixMap_a1w4i8	 = Pixmap<colormode_a1w4_i8>;
using PixMap_a1w4rgb = Pixmap<colormode_a1w4_rgb>;
using PixMap_a1w8i4	 = Pixmap<colormode_a1w8_i4>; // Pixels = 1bpp, Attribute cell width = 8px, colors = i4
using PixMap_a1w8i8	 = Pixmap<colormode_a1w8_i8>;
using PixMap_a1w8rgb = Pixmap<colormode_a1w8_rgb>;
using PixMap_a2w1i4	 = Pixmap<colormode_a2w1_i4>; // Pixels = 2bpp, Attribute cell width = 1px, colors = i4
using PixMap_a2w1i8	 = Pixmap<colormode_a2w1_i8>;
using PixMap_a2w1rgb = Pixmap<colormode_a2w1_rgb>;
using PixMap_a2w2i4	 = Pixmap<colormode_a2w2_i4>;
using PixMap_a2w2i8	 = Pixmap<colormode_a2w2_i8>;
using PixMap_a2w2rgb = Pixmap<colormode_a2w2_rgb>;
using PixMap_a2w4i4	 = Pixmap<colormode_a2w4_i4>;
using PixMap_a2w4i8	 = Pixmap<colormode_a2w4_i8>;
using PixMap_a2w4rgb = Pixmap<colormode_a2w4_rgb>;
using PixMap_a2w8i4	 = Pixmap<colormode_a2w8_i4>;
using PixMap_a2w8i8	 = Pixmap<colormode_a2w8_i8>;
using PixMap_a2w8rgb = Pixmap<colormode_a2w8_rgb>;


// how ugly can it be?
#define AttrModePixmap Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>


/***************************************************************************
				Template for the attribute color PixMaps
************************************************************************** */

template<ColorMode CM>
class AttrModePixmap final : public Pixmap<ColorMode(get_attrmode(CM))>
{
public:
	static constexpr int calc_ax(int x) noexcept { return x >> AW << bits_per_pixel; } // attributes[x] for ink = 0
	static constexpr int calc_aw(int w) noexcept { return (w + (1 << AW) - 1) >> AW << bits_per_pixel; }
	static constexpr int calc_ah(int h, int ah) noexcept { return (h + ah - 1) / ah; }
	int					 calc_ay(int y) const noexcept { return y / attrheight; }

	static constexpr ColorDepth CD				= get_colordepth(CM); // 0 .. 4  log2 of bits per color in attributes[]
	static constexpr AttrMode	AM				= get_attrmode(CM);	  // 0 .. 2  log2 of bits per color in pixmap[]
	static constexpr AttrWidth	AW				= get_attrwidth(CM);  // 0 .. 3  log2 of width of tiles
	static constexpr int		bits_per_color	= 1 << CD; // bits per color in pixmap[] (wAttr: in attributes[])
	static constexpr int		bits_per_pixel	= 1 << AM; // bits per pixel in pixmap[]
	static constexpr int		colors_per_attr = 1 << bits_per_pixel;

	static constexpr ColorDepth colordepth = CD; // 0 .. 4  log2 of bits per color in attributes[]
	static constexpr AttrMode	attrmode   = AM; // 0 .. 2  log2 of bits per color in pixmap[]
	static constexpr AttrWidth	attrwidth  = AW; // 0 .. 3  log2 of width of tiles

	using super = Pixmap<ColorMode(AM)>;
	using IPixmap::attrheight;
	using IPixmap::height;
	using IPixmap::size;
	using IPixmap::width;
	using super::pixmap;
	using super::row_offset; // in pixels[]


	Pixmap<ColorMode(CD)> attributes; // pixmap with color attributes


	// allocating ctor:
	Pixmap(const Size& size, AttrHeight ah) throws;
	Pixmap(coord w, coord h, AttrHeight ah) throws;

	// not allocating: wrap existing pixels:
	Pixmap(const Size& size, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept;
	Pixmap(coord w, coord h, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept;

	// window into another pixmap:
	Pixmap(Pixmap& q, const Rect& r) noexcept;
	Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept;
	Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept;

	virtual ~Pixmap() noexcept override = default;

	bool operator==(const Pixmap& other) const noexcept;

	// helper:
	using IPixmap::is_inside;
	uint attr_get_color(coord x, coord y, uint ink) const noexcept;
	void attr_set_color(coord x, coord y, uint color, uint ink) noexcept;
	void attr_draw_hline(coord x, coord y, coord x2, uint color, uint ink) noexcept;
	void attr_draw_vline(coord x, coord y, coord y2, uint color, uint ink) noexcept;
	void attr_fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink) noexcept;
	void attr_xor_rect(coord x, coord y, coord w, coord h, uint color) noexcept;
	void attr_copy_rect(coord x, coord y, coord qx, coord qy, coord w, coord h) noexcept;

	super&		 as_super() { return *this; }
	const super& as_super() const { return *this; }


	// overrides for IPixmap virtual drawing methods:

	virtual void set_pixel(coord x, coord y, uint color, uint ink = 0) noexcept override;
	virtual uint get_pixel(coord x, coord y, uint* ink) const noexcept override;
	virtual uint get_color(coord x, coord y) const noexcept override;
	virtual uint get_ink(coord x, coord y) const noexcept override;

	virtual void draw_hline(coord x, coord y, coord w, uint color, uint ink) noexcept override;
	virtual void draw_vline(coord x, coord y, coord h, uint color, uint ink) noexcept override;
	virtual void fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink) noexcept override;
	virtual void copy_rect(coord x, coord y, coord qx, coord qy, coord w, coord h) noexcept override;
	virtual void copy_rect(coord x, coord y, const IPixmap& q, coord qx, coord qy, coord w, coord h) noexcept override;
	//virtual void read_bmp(coord x, coord y, uint8*, int roffs, coord w, coord h, uint c, uint = 0) noexcept override;
	virtual void draw_bmp(coord x, coord y, const uint8*, int ro, coord w, coord h, uint c, uint ink) noexcept override;
	virtual void draw_char(coord x, coord y, const uint8* bmp, coord h, uint color, uint ink) noexcept override;
	virtual void xor_rect(coord x, coord y, coord w, coord h, uint xor_color) noexcept override;

	// non-overrides:

	void copy_rect(coord x, coord y, const Pixmap& q) noexcept;
	void copy_rect(coord x, coord y, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept;
	void draw_bmp(coord x, coord y, const Bitmap&, uint color, uint pixel) noexcept;

	void copyRect(coord x, coord y, const Pixmap& src) noexcept;
	void copyRect(coord x, coord y, const Pixmap& src, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(const Point& z, const Pixmap& src) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Point& q, const Size&) noexcept;
	void copyRect(const Point& z, const Pixmap& src, const Rect& q) noexcept;

	void drawBmp(coord x, coord y, const Bitmap& q, uint color, uint /*ink*/ = 0) noexcept;
	void drawBmp(const Point& z, const Bitmap& bmp, uint color, uint /*ink*/ = 0) noexcept;
};


// ####################################################################
//							Implementations
// ####################################################################


// __________________________________________________________________
// IMPLEMENTATIONS: ctor, dtor

// allocating, throws:
template<ColorMode CM>
AttrModePixmap::Pixmap(const Size& size, AttrHeight ah) : Pixmap(size.width, size.height, ah)
{}

template<ColorMode CM>
AttrModePixmap::Pixmap(coord w, coord h, AttrHeight ah) : super(w, h, CM, ah), attributes(calc_aw(w), calc_ah(h, ah))
{
	assert(attrheight != attrheight_none);
}

// not allocating: wrap existing pixels:
template<ColorMode CM>
AttrModePixmap::Pixmap(
	const Size& size, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept :
	Pixmap(size.width, size.height, pixels, row_offs, attr, attr_row_offs, ah)
{}

template<ColorMode CM>
AttrModePixmap::Pixmap(
	coord w, coord h, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept :
	super(w, h, CM, ah, pixels, row_offs),
	attributes(calc_aw(w), calc_ah(h, ah), attr, attr_row_offs)
{
	assert(AM != attrmode_none);
	assert(attrheight != attrheight_none);
}

// window into another pixmap:
template<ColorMode CM>
AttrModePixmap::Pixmap(Pixmap& q, const Rect& r) noexcept : Pixmap(q, r.left(), r.top(), r.width(), r.height())
{}

template<ColorMode CM>
AttrModePixmap::Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept :
	super(q, x, y, w, h),
	attributes(q.attributes, calc_ax(x), calc_ay(y), calc_aw(w), calc_ah(h, q.attrheight))
{
	assert(x % (1 << AW) == 0);	  // x must be a multiple of the attribute cell width
	assert(y % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((x << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]  ((tested in super too))
}

template<ColorMode CM>
AttrModePixmap::Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept :
	Pixmap(q, p.x, p.y, size.width, size.height)
{}


// __________________________________________________________________
// IMPLEMENTATIONS: helpers

template<ColorMode CM>
void AttrModePixmap::attr_set_color(coord x, coord y, uint color, uint ink) noexcept
{
	// set color in attributes[]

	assert((ink & ~pixelmask<super::CD>) == 0);

	x = calc_ax(x);
	y = calc_ay(y);
	attributes.set_pixel(x + ink, y, color);
}

template<ColorMode CM>
uint AttrModePixmap::attr_get_color(coord x, coord y, uint ink) const noexcept
{
	// get color from attributes[]

	assert((ink & ~pixelmask<super::CD>) == 0);

	x = calc_ax(x);
	y = calc_ay(y);
	return attributes.get_ink(x + ink, y);
}

template<ColorMode CM>
void AttrModePixmap::attr_draw_hline(coord x1, coord y1, coord x2, uint color, uint ink) noexcept
{
	assert((ink & ~pixelmask<super::CD>) == 0);

	x1 = calc_ax(x1);
	x2 = calc_ax(x2 - 1) + 1;
	y1 = calc_ay(y1);

	bitblit::attr_draw_hline<AM, CD>(attributes.pixmap + y1 * attributes.row_offset, x1 + ink, x2 - x1, color);
}

template<ColorMode CM>
void AttrModePixmap::attr_draw_vline(coord x1, coord y1, coord y2, uint color, uint ink) noexcept
{
	assert((ink & ~pixelmask<super::CD>) == 0);

	x1 = calc_ax(x1);
	y1 = calc_ay(y1);
	y2 = calc_ay(y2 - 1) + 1;

	attributes.draw_vline(x1 + ink, y1, y2, color);
}

template<ColorMode CM>
void AttrModePixmap::attr_fill_rect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	assert((ink & ~pixelmask<super::CD>) == 0);

	w = calc_ax(x1 + w - 1) - x1 + 1;
	h = calc_ay(y1 + h - 1) - y1 + 1;

	x1 = calc_ax(x1);
	y1 = calc_ay(y1);

	bitblit::attr_clear_rect<AM, CD>(
		attributes.pixmap + y1 * attributes.row_offset, attributes.row_offset, x1 + ink, w, h, color);
}

template<ColorMode CM>
void AttrModePixmap::attr_xor_rect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	w = calc_ax(x1 + w - 1) - x1 + colors_per_attr;
	h = calc_ay(y1 + h - 1) - y1 + 1;

	x1 = calc_ax(x1);
	y1 = calc_ay(y1);

	attributes.xor_rect(x1, y1, w, h, xor_color);
}

template<ColorMode CM>
void AttrModePixmap::attr_copy_rect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	w = calc_ax(zx + w - 1) - zx + colors_per_attr;
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

template<ColorMode CM>
bool AttrModePixmap::operator==(const Pixmap& other) const noexcept
{
	assert(size == other.size);
	assert(attrheight == other.attrheight);

	const super& base = *this;
	return base == other && attributes == other.attributes;
}


// __________________________________________________________________
// IMPLEMENTATIONS: set pixel, get pixel

template<ColorMode CM>
void AttrModePixmap::set_pixel(coord x, coord y, uint color, uint ink) noexcept
{
	attr_set_color(x, y, color, ink);
	super::set_pixel(x, y, ink);
}

template<ColorMode CM>
uint AttrModePixmap::get_ink(coord x, coord y) const noexcept
{
	// get ink from pixels[]
	// same as in Pixmap.getInk()
	assert(is_inside(x, y));
	return bitblit::get_pixel<colordepth>(pixmap + y * row_offset, x);
}

template<ColorMode CM>
uint AttrModePixmap::get_color(coord x, coord y) const noexcept
{
	// get color from attributes[]
	uint ink = get_ink(x, y);
	return attr_get_color(x, y, ink);
}

template<ColorMode CM>
uint AttrModePixmap::get_pixel(coord x, coord y, uint* ink) const noexcept
{
	// get color and ink
	return attr_get_color(x, y, *ink = get_ink(x, y));
}


// __________________________________________________________________
// IMPLEMENTATIONS: draw line, fill rect

template<ColorMode CM>
void AttrModePixmap::draw_hline(coord x, coord y, coord x2, uint color, uint ink) noexcept
{
	attr_draw_hline(x, y, x2, color, ink);
	super::draw_hline(x, y, x2, ink);
}

template<ColorMode CM>
void AttrModePixmap::draw_vline(coord x, coord y, coord y2, uint color, uint ink) noexcept
{
	attr_draw_vline(x, y, y2, color, ink);
	super::draw_vline(x, y, y2, ink);
}

template<ColorMode CM>
void AttrModePixmap::fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink) noexcept
{
	attr_fill_rect(x, y, w, h, color, ink);
	super::fill_rect(x, y, w, h, ink);
}

template<ColorMode CM>
void AttrModePixmap::xor_rect(coord x, coord y, coord w, coord h, uint color) noexcept
{
	attr_xor_rect(x, y, w, h, color);
}


// __________________________________________________________________
// IMPLEMENTATIONS: copy rect areas, draw image & bitmap

template<ColorMode CM>
void AttrModePixmap::copy_rect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	attr_copy_rect(zx, zy, qx, qy, w, h);
	super::copy_rect(zx, zy, qx, qy, w, h);
}

template<ColorMode CM>
void AttrModePixmap::copy_rect(coord zx, coord zy, const IPixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(CM == q.colormode); // must be same type
	const Pixmap& src = static_cast<const Pixmap&>(q);
	assert(attrheight == src.attrheight);

	copy_rect(zx, zy, src, qx, qy, w, h);
}

template<ColorMode CM>
void AttrModePixmap::draw_bmp(
	coord x, coord y, const uint8* bmp, int bmp_row_offset, int w, int h, uint color, uint ink) noexcept
{
	attr_fill_rect(x, y, w, h, color, ink);
	super::draw_bmp(x, y, bmp, bmp_row_offset, w, h, ink);
}

template<ColorMode CM>
void AttrModePixmap::draw_bmp(coord x, coord y, const Bitmap& bmp, uint color, uint ink) noexcept
{
	attr_fill_rect(x, y, bmp.width, bmp.height, color, ink);
	super::draw_bmp(x, y, bmp, ink);
}

template<ColorMode CM>
void AttrModePixmap::draw_char(coord x, coord y, const uint8* bmp, int height, uint color, uint ink) noexcept
{
	constexpr int width = 8;
	attr_fill_rect(x, y, width, height, color, ink);
	super::draw_char(x, y, bmp, height, ink);
}

template<ColorMode CM>
void AttrModePixmap::copy_rect(coord x, coord y, const Pixmap& source) noexcept
{
	assert(x % (1 << AW) == 0);	  // x must be a multiple of the attribute cell width
	assert(y % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((x << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

	super::copy_rect(x, y, source.as_super());
	x = calc_ax(x);
	y = calc_ay(y);
	attributes.copy_rect(x, y, source.attributes);
}

template<ColorMode CM>
void AttrModePixmap::copy_rect(coord zx, coord zy, const Pixmap& source, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(zx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
	assert(zy % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((zx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

	assert(qx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
	assert(qy % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((qx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

	super::copy_rect(zx, zy, source.as_super(), qx, qy, w, h);
	zx = calc_ax(zx);
	zy = calc_ay(zy);
	qx = calc_ax(qx);
	qy = calc_ay(qy);
	w  = calc_ax(zx + w - 1) + 1 - zx;
	h  = calc_ay(zy + h - 1) + 1 - zy;
	attributes.copy_rect(zx, zy, source.attributes, qx, qy, w, h);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(coord x, coord y, const Pixmap& q) noexcept
{
	assert(q.attrheight == attrheight);

	coord qx = 0;
	coord qy = 0;
	coord w	 = q.width;
	coord h	 = q.height;

	if (unlikely(x < 0))
	{
		qx -= x;
		w += x;
		x = 0;
	}
	if (unlikely(y < 0))
	{
		qy -= y;
		h += y;
		y = 0;
	}

	if (unlikely(w > width - x)) { w = width - x; }
	if (unlikely(h > height - y)) { h = height - y; }

	if (w > 0 && h > 0) copy_rect(x, y, q, qx, qy, w, h);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(q.attrheight == attrheight);

	if (unlikely(qx < 0))
	{
		w += qx;
		zx -= qx;
		qx = 0;
	}
	if (unlikely(qy < 0))
	{
		h += qy;
		zy -= qy;
		qy = 0;
	}
	if (unlikely(zx < 0))
	{
		w += zx;
		qx -= zx;
		zx = 0;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		qy -= zy;
		zy = 0;
	}

	if (unlikely(w > q.width - qx)) { w = q.width - qx; }
	if (unlikely(h > q.height - qy)) { h = q.height - qy; }
	if (unlikely(w > width - zx)) { w = width - zx; }
	if (unlikely(h > height - zy)) { h = height - zy; }

	if (w > 0 && h > 0) copy_rect(zx, zy, q, qx, qy, w, h);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(const Point& z, const Pixmap& q) noexcept
{
	copyRect(z.x, z.y, q);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(const Point& z, const Pixmap& q, const Point& p, const Size& s) noexcept
{
	copyRect(z.x, z.y, q, p.x, p.y, s.width, s.height);
}

template<ColorMode CM>
void AttrModePixmap::copyRect(const Point& z, const Pixmap& q, const Rect& r) noexcept
{
	copyRect(z.x, z.y, q, r.left(), r.top(), r.width(), r.height());
}

template<ColorMode CM>
void AttrModePixmap::drawBmp(coord x, coord y, const Bitmap& q, uint color, uint ink) noexcept
{
	const uint8* qp = q.pixmap;
	coord		 w	= q.width;
	coord		 h	= q.height;

	if (unlikely(x < 0))
	{
		assert((x & 7) == 0); // TODO: non-aligned draw_bmp
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

	if (unlikely(w > width - x)) { w = width - x; }
	if (unlikely(h > height - y)) { h = height - y; }

	if (w > 0 && h > 0) draw_bmp(x, y, q, color, ink);
}

template<ColorMode CM>
void AttrModePixmap::drawBmp(const Point& z, const Bitmap& bmp, uint color, uint ink) noexcept
{
	drawBmp(z.x, z.y, bmp, color, ink);
}


extern template class Pixmap<colormode_a1w1_i4>;
extern template class Pixmap<colormode_a1w1_i8>;
extern template class Pixmap<colormode_a1w1_rgb>;
extern template class Pixmap<colormode_a1w2_i4>;
extern template class Pixmap<colormode_a1w2_i8>;
extern template class Pixmap<colormode_a1w2_rgb>;
extern template class Pixmap<colormode_a1w4_i4>;
extern template class Pixmap<colormode_a1w4_i8>;
extern template class Pixmap<colormode_a1w4_rgb>;
extern template class Pixmap<colormode_a1w8_i4>;
extern template class Pixmap<colormode_a1w8_i8>;
extern template class Pixmap<colormode_a1w8_rgb>;
extern template class Pixmap<colormode_a2w1_i4>;
extern template class Pixmap<colormode_a2w1_i8>;
extern template class Pixmap<colormode_a2w1_rgb>;
extern template class Pixmap<colormode_a2w2_i4>;
extern template class Pixmap<colormode_a2w2_i8>;
extern template class Pixmap<colormode_a2w2_rgb>;
extern template class Pixmap<colormode_a2w4_i4>;
extern template class Pixmap<colormode_a2w4_i8>;
extern template class Pixmap<colormode_a2w4_rgb>;
extern template class Pixmap<colormode_a2w8_i4>;
extern template class Pixmap<colormode_a2w8_i8>;
extern template class Pixmap<colormode_a2w8_rgb>;


} // namespace kio::Graphics


//
