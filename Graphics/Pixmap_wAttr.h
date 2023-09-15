// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap.h"


namespace kipili::Graphics
{


/***************************************************************************
				Template for the attribute color PixMaps
************************************************************************** */

template<ColorMode CM>
class Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>> : public Pixmap<ColorMode(get_attrmode(CM))>
{
public:
	static constexpr int calc_ax(int x) noexcept { return x >> AW << bits_per_pixel; } // attributes[x] for pixel = 0
	static constexpr int calc_ay(int y, int ah) noexcept { return y / ah; }
	static constexpr int calc_aw(int w) noexcept { return (w + (1 << AW) - 1) >> AW << bits_per_pixel; }
	static constexpr int calc_ah(int h, int ah) noexcept { return (h + ah - 1) / ah; }

public:
	static constexpr ColorDepth CD			   = get_colordepth(CM); // 0 .. 4  log2 of bits per color in attributes[]
	static constexpr AttrMode	AM			   = get_attrmode(CM);	 // 0 .. 2  log2 of bits per color in pixmap[]
	static constexpr AttrWidth	AW			   = get_attrwidth(CM);	 // 0 .. 3  log2 of width of tiles
	static constexpr int		bits_per_color = 1 << CD; // bits per color in pixmap[] (wAttr: in attributes[])
	static constexpr int		bits_per_pixel = 1 << AM; // bits per pixel in pixmap[]

	static constexpr ColorDepth colordepth = CD; // 0 .. 4  log2 of bits per color in attributes[]
	static constexpr AttrMode	attrmode   = AM; // 0 .. 2  log2 of bits per color in pixmap[]
	static constexpr AttrWidth	attrwidth  = AW; // 0 .. 3  log2 of width of tiles


	using super = Pixmap<ColorMode(AM)>;
	using super::height;
	using super::pixmap;
	using super::row_offset; // in pixels[]
	using super::size;
	using super::width;

	const AttrHeight attrheight; // rows per tile: 1 .. 16

	Pixmap<ColorMode(CD)> attributes;


	// allocating, throws:
	Pixmap(const Size& size, AttrHeight ah) : Pixmap(size.width, size.height, ah) {}

	Pixmap(coord w, coord h, AttrHeight ah) : super(w, h), attrheight(ah), attributes(calc_aw(w), calc_ah(h, ah))
	{
		assert(AM != attrmode_none);
		assert(attrheight != attrheight_none);
	}

	// not allocating: wrap existing pixels:
	Pixmap(const Size& size, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept :
		Pixmap(size.width, size.height, pixels, row_offs, attr, attr_row_offs, ah)
	{}

	Pixmap(coord w, coord h, uint8* pixels, int row_offs, uint8* attr, int attr_row_offs, AttrHeight ah) noexcept :
		super(w, h, pixels, row_offs),
		attrheight(ah),
		attributes(calc_aw(w), calc_ah(h, ah), attr, attr_row_offs)
	{
		assert(AM != attrmode_none);
		assert(attrheight != attrheight_none);
	}

	// window into another pixmap:
	Pixmap(Pixmap& q, const Rect& r) noexcept : Pixmap(q, r.left(), r.top(), r.width(), r.height()) {}

	Pixmap(Pixmap& q, const Point& p, const Size& size) noexcept : Pixmap(q, p.x, p.y, size.width, size.height) {}

	Pixmap(Pixmap& q, coord x, coord y, coord w, coord h) noexcept :
		super(q, x, y, w, h),
		attrheight(q.attrheight),
		attributes(q.attributes, calc_ax(x), calc_ay(y, q.attrheight), calc_aw(w), calc_ah(h, q.attrheight))
	{
		assert(x % (1 << AW) == 0);	  // x must be a multiple of the attribute cell width
		assert(y % attrheight == 0);  // y must be a multiple of the attribute cell height
		assert(((x << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]  ((tested in super too))
	}

	~Pixmap() noexcept = default;


	// methods working on pixels[] and attributes[]:

	using super::getPixel;
	uint getColor(coord x, coord y) const noexcept;
	uint getColor(const Point& p) const noexcept { return getColor(p.x, p.y); }

	using super::setPixel;
	void setPixel(coord x, coord y, uint color, uint pixel) noexcept;
	void setPixel(const Point& p, uint color, uint pixel) noexcept { setPixel(p.x, p.y, color, pixel); }

	using super::drawHorLine;
	void drawHorLine(coord x, coord y, coord x2, uint color, uint pixel) noexcept;
	void drawHorLine(const Point& p, coord x2, uint color, uint pixel) noexcept
	{
		drawHorLine(p.x, p.y, x2, color, pixel);
	}

	using super::drawVertLine;
	void drawVertLine(coord x, coord y, coord y2, uint color, uint pixel) noexcept;
	void drawVertLine(const Point& p, coord y2, uint color, uint pixel) noexcept
	{
		drawVertLine(p.x, p.y, y2, color, pixel);
	}

	void clear(uint color) noexcept; // whole pixmap
	using super::fillRect;
	void fillRect(coord x, coord y, coord w, coord h, uint color, uint pixel) noexcept;
	void fillRect(const Rect& r, uint color, uint pixel) noexcept
	{
		fillRect(r.left(), r.top(), r.width(), r.height(), color, pixel);
	}

	void copy(const Pixmap& source) noexcept;
	void copyRect(coord x, coord y, const Pixmap& q) noexcept;
	void copyRect(coord x, coord y, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(const Point& z, const Pixmap& q) noexcept { copyRect(z.x, z.y, q); }
	void copyRect(const Point& z, const Pixmap& q, const Point& p, const Size& s) noexcept
	{
		copyRect(z.x, z.y, q, p.x, p.y, s.width, s.height);
	}
	void copyRect(const Point& z, const Pixmap& q, const Rect& r) noexcept
	{
		copyRect(z.x, z.y, q, r.left(), r.top(), r.width(), r.height());
	}
	void copyRect(const Point& z, const Rect& r) noexcept
	{
		copyRect(z.x, z.y, *this, r.left(), r.top(), r.width(), r.height());
	}
	void copyRect(const Point& z, const Point& q, const Size& s) noexcept
	{
		copyRect(z.x, z.y, *this, q.x, q.y, s.width, s.height);
	}

	using super::drawBmp;
	void drawBmp(coord x, coord y, const Bitmap& bmp, uint color, uint pixel) noexcept;
	void drawBmp(const Point& z, const Bitmap& bmp, uint color, uint pixel) noexcept
	{
		drawBmp(z.x, z.y, bmp, color, pixel);
	}
	using super::drawChar;
	void drawChar(coord x, coord y, const uint8* bmp, coord h, uint color, uint pixel) noexcept;
	void drawChar(const Point& z, const uint8* bmp, coord h, uint color, uint pixel) noexcept
	{
		drawChar(z.x, z.y, bmp, h, color, pixel);
	}

	// helper:
	using super::is_inside;
	void calcRectForAttr(Rect& r) noexcept;

	// no checking and ordered coordinates only

	using super::get_pixel;							 // get pixel value
	uint get_color(coord x, coord y) const noexcept; // get color from attribute

	using super::set_pixel;											   // pixel only
	void set_pixel(coord x, coord y, uint color, uint pixel) noexcept; // pixel + attribute
	void set_color(coord x, coord y, uint color, uint pixel) noexcept; // attr only

	using super::draw_hline;														   // pixels only
	void draw_hline(coord x, coord y, coord x2, uint color, uint pixel) noexcept;	   // pixel + attribute
	void attr_draw_hline(coord x, coord y, coord x2, uint color, uint pixel) noexcept; // attr only

	using super::draw_vline;														   // pixels only
	void draw_vline(coord x, coord y, coord y2, uint color, uint pixel) noexcept;	   // pixel + attribute
	void attr_draw_vline(coord x, coord y, coord y2, uint color, uint pixel) noexcept; // attr only

	using super::fill_rect;
	void fill_rect(coord x, coord y, coord w, coord h, uint color, uint pixel) noexcept;
	void attr_fill_rect(coord x, coord y, coord w, coord h, uint color, uint pixel) noexcept;

	using super::copy_rect;											 // pixels only
	void copy_rect(coord x, coord y, const Pixmap& source) noexcept; // pixels + attributes
	void
	copy_rect(coord x, coord y, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept; // pixels + attributes

	using super::draw_bmp;															 // pixels only
	void draw_bmp(coord x, coord y, const Bitmap&, uint color, uint pixel) noexcept; // pixels + attributes
	void draw_bmp(
		coord x, coord y, const uint8* bmp, int bmp_row_offset, int w, int h, uint color,
		uint pixel) noexcept; // pix+attr

	using super::draw_char; // pixels only
	void
	draw_char(coord x, coord y, const uint8* bmp, int height, uint color, uint pixel) noexcept; // pixels + attributes
};


// handy names:

using PixMap_aw1i4	= Pixmap<colormode_a1w1_i4>;
using PixMap_aw1i8	= Pixmap<colormode_a1w1_i8>;
using PixMap_aw1rgb = Pixmap<colormode_a1w1_rgb>;

using PixMap_aw2i4	= Pixmap<colormode_a1w2_i4>;
using PixMap_aw2i8	= Pixmap<colormode_a1w2_i8>;
using PixMap_aw2rgb = Pixmap<colormode_a1w2_rgb>;

using PixMap_aw4i4	= Pixmap<colormode_a1w4_i4>;
using PixMap_aw4i8	= Pixmap<colormode_a1w4_i8>;
using PixMap_aw4rgb = Pixmap<colormode_a1w4_rgb>;

using PixMap_aw8i4	= Pixmap<colormode_a1w8_i4>;
using PixMap_aw8i8	= Pixmap<colormode_a1w8_i8>;
using PixMap_aw8rgb = Pixmap<colormode_a1w8_rgb>;

using PixMap_a2w1i4	 = Pixmap<colormode_a2w1_i4>;
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


// ########################################################

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::calcRectForAttr(Rect& r) noexcept
{
	// calculates the rect
	r.p1.x = calc_ax(r.p1.x);
	r.p2.x = calc_ax(r.p2.x - 1) + 1;
	r.p1.y = calc_ay(r.p1.y, attrheight);
	r.p2.y = calc_ay(r.p2.y - 1, attrheight) + 1;
}

template<ColorMode CM>
uint Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::get_color(coord x, coord y) const noexcept
{
	uint pixel = super::get_pixel(x, y);
	x		   = calc_ax(x) + pixel;
	y		   = calc_ay(y, attrheight);
	return attributes.get_pixel(x, y);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::set_color(
	coord x, coord y, uint color, uint pixel) noexcept
{
	assert((pixel & ~pixelmask<super::CD>) == 0);

	x = calc_ax(x) + pixel;
	y = calc_ay(y, attrheight);
	attributes.set_pixel(x, y, color);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::set_pixel(
	coord x, coord y, uint color, uint pixel) noexcept
{
	set_color(x, y, color, pixel);
	super::set_pixel(x, y, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::attr_draw_hline(
	coord x, coord y, coord x2, uint color, uint pixel) noexcept
{
	assert((pixel & ~pixelmask<super::CD>) == 0);

	x  = calc_ax(x) + pixel;
	x2 = calc_ax(x2 - 1) + 1 + pixel;
	y  = calc_ay(y, attrheight);

	bitblit::attr_draw_hline<AM, CD>(attributes.pixmap + y * attributes.row_offset, x, x2 - x, color);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::draw_hline(
	coord x, coord y, coord x2, uint color, uint pixel) noexcept
{
	attr_draw_hline(x, y, x2, color, pixel);
	super::draw_hline(x, y, x2, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::attr_draw_vline(
	coord x, coord y, coord y2, uint color, uint pixel) noexcept
{
	assert((pixel & ~pixelmask<super::CD>) == 0);

	x  = calc_ax(x) + pixel;
	y  = calc_ay(y, attrheight);
	y2 = calc_ay(y2 - 1, attrheight) + 1;

	attributes.draw_vline(x, y, y2, color);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::draw_vline(
	coord x, coord y, coord y2, uint color, uint pixel) noexcept
{
	attr_draw_vline(x, y, y2, color, pixel);
	super::draw_vline(x, y, y2, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::attr_fill_rect(
	coord x, coord y, coord w, coord h, uint color, uint pixel) noexcept
{
	assert((pixel & ~pixelmask<super::CD>) == 0);

	x = calc_ax(x) + pixel;
	w = calc_ax(x + w - 1) + 1 - x;
	y = calc_ay(y, attrheight);
	h = calc_ay(y + h - 1, attrheight) + 1 - h;

	bitblit::attr_clear_rect<AM, CD>(
		attributes.pixmap + y * attributes.row_offset, attributes.row_offset, x, w, h, color);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::fill_rect(
	coord x, coord y, coord w, coord h, uint color, uint pixel) noexcept
{
	attr_fill_rect(x, y, w, h, color, pixel);
	super::fill_rect(x, y, w, h, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::copy_rect(
	coord x, coord y, const Pixmap& source) noexcept
{
	assert(x % (1 << AW) == 0);	  // x must be a multiple of the attribute cell width
	assert(y % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((x << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

	super::copy_rect(x, y, source);
	x = calc_ax(x);
	y = calc_ay(y, attrheight);
	attributes.copy_rect(x, y, source.attributes);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::copy_rect(
	coord zx, coord zy, const Pixmap& source, coord qx, coord qy, coord w, coord h) noexcept
{
	assert(zx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
	assert(zy % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((zx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

	assert(qx % (1 << AW) == 0);   // x must be a multiple of the attribute cell width
	assert(qy % attrheight == 0);  // y must be a multiple of the attribute cell height
	assert(((qx << AM) & 7) == 0); // x must be a multiple of full bytes in pixmap[]

	super::copy_rect(zx, zy, source, qx, qy, w, h);
	zx = calc_ax(zx);
	zy = calc_ay(zy, attrheight);
	qx = calc_ax(qx);
	qy = calc_ay(qy, attrheight);
	w  = calc_ax(zx + w - 1) + 1 - zx;
	h  = calc_ay(zy + h - 1, attrheight) + 1 - zy;
	attributes.copy_rect(zx, zy, source.attributes, qx, qy, w, h);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::draw_bmp(
	coord x, coord y, const Bitmap& bmp, uint color, uint pixel) noexcept
{
	attr_fill_rect(x, y, bmp.width, bmp.height, color, pixel);
	super::draw_bmp(x, y, bmp, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::draw_bmp(
	coord x, coord y, const uint8* bmp, int bmp_row_offset, int w, int h, uint color, uint pixel) noexcept
{
	attr_fill_rect(x, y, w, h, color, pixel);
	super::draw_bmp(x, y, bmp, bmp_row_offset, w, h, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::draw_char(
	coord x, coord y, const uint8* bmp, int height, uint color, uint pixel) noexcept
{
	constexpr int width = 8;
	attr_fill_rect(x, y, width, height, color, pixel);
	super::draw_char(x, y, bmp, height, pixel);
}


template<ColorMode CM>
uint Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::getColor(coord x, coord y) const noexcept
{
	return is_inside(x, y) ? get_color(x, y) : 0;
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::setPixel(
	coord x, coord y, uint color, uint pixel) noexcept
{
	if (is_inside(x, y)) set_pixel(x, y, color, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::drawHorLine(
	coord x, coord y, coord x2, uint color, uint pixel) noexcept
{
	// draw hor line between x1 and x2.
	// draws abs(x2-x1) pixels.
	// draws nothing if x1==x2.

	if (uint(y) >= uint(height)) return;
	sort(x, x2);
	x  = max(x, 0);
	x2 = min(x2, width);
	draw_hline(x, y, x2, color, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::drawVertLine(
	coord x, coord y, coord y2, uint color, uint pixel) noexcept
{
	// draw vert line between y1 and y2.
	// draws abs(y2-y1) pixels.
	// draws nothing if y1==y2.

	if (uint(x) >= uint(width)) return;
	sort(y, y2);
	y  = max(y, 0);
	y2 = min(y2, height);
	draw_vline(x, y, y2, color, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::clear(uint color) noexcept
{
	attributes.clear(color);
	super::clear(0);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::fillRect(
	coord x, coord y, coord w, coord h, uint color, uint pixel) noexcept
{
	// draw filled rectangle.
	// nothing is drawn for empty rect!

	x = max(x, 0);
	w = min(w, width - x);
	y = max(y, 0);
	h = min(h, height - y);

	if (w > 0 && h > 0) fill_rect(x, y, w, h, color, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::copy(const Pixmap& source) noexcept
{
	assert(source.attrheight == attrheight);

	attributes.copy(source.attributes);
	super::copy(source);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::copyRect(coord x, coord y, const Pixmap& q) noexcept
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
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::copyRect(
	coord zx, coord zy, const Pixmap& q, coord qx, coord qy, coord w, coord h) noexcept
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
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::drawBmp(
	coord x, coord y, const Bitmap& q, uint color, uint pixel) noexcept
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

	if (w > 0 && h > 0) draw_bmp(x, y, q, color, pixel);
}

template<ColorMode CM>
void Pixmap<CM, typename std::enable_if_t<is_attribute_mode(CM)>>::drawChar(
	coord x, coord y, const uint8* bmp, int h, uint color, uint pixel) noexcept
{
	constexpr int bmp_row_offs = 1;
	constexpr int w			   = 8;

	if (unlikely(x < 0 || x > width - w))
	{
		drawBmp(x, y, Bitmap(w, h, const_cast<uint8*>(bmp), bmp_row_offs), color, pixel);
		return;
	}

	if (unlikely(h > height - y)) { h = height - y; }
	if (unlikely(y < 0))
	{
		bmp -= y * bmp_row_offs;
		h += y;
		y = 0;
	}

	if (h > 0) draw_char(x, y, bmp, h, color, pixel);
}


} // namespace kipili::Graphics
