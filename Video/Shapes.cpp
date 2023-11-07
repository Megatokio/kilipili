// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Shapes.h"

namespace kio::Video
{

#if 0
Shape::Row* Shape::Row::construct(int width, int height, const Color* pixels) noexcept
{
	// copy and 'compress' data from bitmap
	// kind of placement new
	//
	// this Shape must be large enough to hold the resulting data!
	// use calc_size() for this

	assert(width >= 0);
	assert(height >= 0);
	assert(pixels != nullptr);

	// copy & 'compress' pixels:
	Row* p	 = this; // running pointer
	int	 dx0 = 0;	 // x offset of last row (absolute dx)

	for (int y = 0; y < height; y++, pixels += width, p = p->next())
	{
		int dx, sz;
		tie(dx, sz) = calc_sz(width, pixels);
		if (sz)
		{
			dx -= dx0;
			dx0 += dx;
		}
		else dx = 0;

		assert(dx == int8(dx) && dx != dxStopper);
		assert(sz == uint8(sz));
		assert(uint(dx0 + sz) <= uint(width));

		p->dx	 = int8(dx);
		p->width = uint8(sz);
		memcpy(p->pixels, pixels + dx0, uint(sz) * sizeof(uint16));
	}

	p->dx = dxStopper;
	assert((uint16ptr(p) - uint16ptr(this) + 1) == calc_size(width, height, pixels - (width * height)).first);
	return this;
}

Shape::Row* Shape::Row::newShape(int width, int height, const Color* pixels) throws
{
	// allocate and initialize a new Shape

	assert(width >= 0);
	assert(height >= 0);
	assert(pixels != nullptr);

	int count;
	tie(count, height) = calc_size(width, height, pixels);				  // calc size and adjust height for empty lines
	Row* shape		   = reinterpret_cast<Row*>(new uint16[uint(count)]); // allocate: may throw.
	return shape->construct(width, height, pixels);
}

Shape::Row* Shape::Row::newShape(const Graphics::Pixmap_rgb& pixmap) throws
{
	assert(pixmap.row_offset == pixmap.width * int(sizeof(Color)));

	return newShape(pixmap.width, pixmap.height, reinterpret_cast<const Color*>(pixmap.pixmap));
}

Shape::Shape(const Graphics::Pixmap_rgb& pixmap, int16 hot_x, int16 hot_y) :
	width(int16(pixmap.width)),
	height(int16(pixmap.height)),
	hot_x(hot_x),
	hot_y(hot_y),
	rows(Shape::Row::newShape(pixmap)) // throws
{}

constexpr Color bitmap_test1[] = {
  #define _ transparent,
  #define b Color::fromRGB8(0, 0, 0),
  #define F Color::fromRGB8(0xEE, 0xEE, 0xFF),

	_ _ F _ _ F _ _ _ _ _ _ _ F F _

  #undef _
  #undef b
  #undef F
};

static_assert(Shape::Row::calc_size(4, 4, bitmap_test1).second == 4);
static_assert(Shape::Row::calc_size(4, 4, bitmap_test1).first == 9);
#endif


//	using std::pair;
//	using std::tie;
//
//	constexpr pair<int, int> Shape::Row::calc_sz(int width, const Color* pixels) noexcept
//	{
//		// calculate start offset dx and width of non-transparent pixels in pixel row
//		// returns dx=0 and width=0 for fully transparent lines
//
//		int dx = 0;
//		while (width && pixels[width - 1].is_transparent()) { width--; }
//		if (width)
//			while (pixels[dx].is_transparent()) { dx++; }
//		width -= dx;
//
//		return {dx, width};
//	}
//
//	constexpr pair<int, int> Shape::Row::calc_size(int width, int height, const Color* px) noexcept
//	{
//		// calculate size in uint16 words for Shape
//		// and reduced height for empty bottom lines
//
//		int size = 1; // +1 for the stopper
//		int ymax = 0; // height without empty lines at bottom of shape
//
//		for (int y = 0; y < height; y++, px += width)
//		{
//			int dx, sz;
//			tie(dx, sz) = calc_sz(width, px);
//			size += sz;
//			if (sz) ymax = y;
//		}
//
//		height = ymax + 1; // +1 for dx&width
//		size += height * 1;
//
//		return {size, height};
//	}


} // namespace kio::Video
