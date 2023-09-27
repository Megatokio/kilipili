// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Canvas.h"
#include "fixint.h"


namespace kio::Graphics
{

Canvas::Canvas(coord w, coord h, ColorMode CM, AttrHeight AH, bool allocated) noexcept :
	width(w),
	height(h),
	colormode(CM),
	attrheight(AH),
	allocated(allocated)
{}

void Canvas::draw_hline(coord x1, coord y1, coord w, uint color, uint ink) noexcept
{
	// draw horizontal line
	// no bound test for speed

	assert(x1 >= 0 && x1 + w <= width);
	assert(y1 >= 0 && y1 + 1 <= height);

	while (--w >= 0) set_pixel(x1++, y1, color, ink);
}

void Canvas::drawHLine(coord x1, coord y1, coord w, uint color, uint ink) noexcept
{
	// draw horizontal line

	if (uint(y1) >= uint(height)) return;

	coord x2 = min(x1 + w, width);
	x1		 = max(x1, 0);

	while (x1 < x2) set_pixel(x1++, y1, color, ink);
}

void Canvas::drawVLine(coord x1, coord y1, coord h, uint color, uint ink) noexcept
{
	// draw vertical line

	if (uint(x1) >= uint(width)) return;

	coord y2 = min(y1 + h, height);
	y1		 = max(y1, 0);

	while (y1 < y2) set_pixel(x1, y1++, color, ink);
}

void Canvas::fillRect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	// draw filled rectangle

	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);
	w		 = x2 - x1;

	if (w > 0)
		while (y1 < y2) draw_hline(x1, y1++, w, color, ink);
}

void Canvas::xorRect(coord x1, coord y1, coord w, coord h, uint xor_color) noexcept
{
	// xor all colors in the rect area with the xorColor

	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);

	for (coord y = y1; y < y2; y++)
	{
		for (coord x = x1; x < x2; x++)
		{
			uint ink, color = get_pixel(x, y, &ink);
			set_pixel(x, y, color ^ xor_color, ink);
		}
	}
}


void Canvas::copyRect(coord zx, coord zy, const Canvas& q, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy a rectangular area from another pixmap of the same ColorDepth.

	if (qx < 0)
	{
		w += qx;
		zx -= qx;
		qx -= qx;
	}
	if (zx < 0)
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (qy < 0)
	{
		h += qy;
		zy -= qy;
		qy -= qy;
	}
	if (zy < 0)
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}
	w = min(w, q.width - qx, width - zx);
	h = min(h, q.height - qy, height - zy);

	while (--h >= 0)
	{
		for (coord i = 0; i < w; i++)
		{
			uint ink, color = get_pixel(qx + i, qy++, &ink);
			set_pixel(zx + i, zy++, color, ink);
		}
	}
}

void Canvas::copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	// copy the pixels from a rectangular area within the same pixmap.
	// overlapping areas are handled safely.

	if (qx < 0)
	{
		w += qx;
		zx -= qx;
		qx -= qx;
	}
	if (zx < 0)
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (qy < 0)
	{
		h += qy;
		zy -= qy;
		qy -= qy;
	}
	if (zy < 0)
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}

	w = min(w, width - zx, width - qx);
	h = min(h, height - zy, height - qy);

	if (zy != qy ? zy < qy : zx < qx) // copy down (towards larger y) -> with incrementing addresses
	{
		while (--h >= 0)
		{
			for (coord i = 0; i < w; i++)
			{
				uint ink, color = get_pixel(qx + i, qy++, &ink);
				set_pixel(zx + i, zy++, color, ink);
			}
		}
	}
	else // copy up (towards smaller y) -> with decrementing addresses
	{
		while (--h >= 0)
		{
			for (coord i = w; --i >= 0;)
			{
				uint ink, color = get_pixel(qx + i, qy + h, &ink);
				set_pixel(zx + i, zy + h, color, ink);
			}
		}
	}
}

void Canvas::draw_hline_bmp(coord x, coord y, coord w, const uint8* q, uint color, uint ink) noexcept
{
	// helper:
	// draw one line from a bitmap
	// draw the set bits with `color`, skip the zeros

	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + 1 <= height);

	while (w > 0)
	{
		w -= 8;
		uint8 byte = *q++;
		if (w < 0) byte &= 0xff >> -w;

		for (uint m = 1; m < 0x100; m = m << 1)
		{
			if (byte & m) set_pixel(x, y, color, ink);
			x++;
		}
	}
}

void Canvas::drawBmp(
	coord zx, coord zy, const uint8* bmp, int row_offset, coord w, coord h, uint color, uint ink) noexcept
{
	// draw bitmap into Canvas
	// draw the set bits with `color`, skip the zeros

	if (unlikely(zx < 0))
	{
		w += zx;
		bmp -= zx / 8; // TODO: need x0 offset in draw_hline_bmp()
		zx -= zx;
	}
	if (unlikely(zy < 0))
	{
		h += zy;
		bmp -= zy * row_offset;
		zy -= zy;
	}
	w = min(w, width - zx);
	h = min(h, height - zy);

	if (w > 0)
		while (--h >= 0)
		{
			draw_hline_bmp(zx, zy++, w, bmp, color, ink);
			bmp += row_offset;
		}
}

void Canvas::drawChar(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint ink) noexcept
{
	// version of drawBmp optimized for:
	//   bmp_row_offset = 1
	//   width = 8
	//   zx = N * 8

	if (unlikely(zx < 0 || zx > width - 8)) return drawBmp(zx, zy, bmp, 1, 8, h, color, ink);

	if (unlikely(zy < 0))
	{
		h += zy;
		bmp -= zy;
		zy -= zy;
	}
	h = min(h, height - zy);

	while (--h >= 0)
	{
		uint8 byte = *bmp++;
		for (int i = 0; byte != 0; byte >>= 1, i++)
		{
			if (byte & 1) set_pixel(zx + i, zy, color, ink);
		}
		zy++;
	}
}

void Canvas::read_hline_bmp(coord x, coord y, coord w, uint8* z, uint color, bool set) noexcept
{
	// helper:
	// read & convert horizontal line to bitmap
	// for attributed pixmaps the bitmap is constructed from the attribute colors, not just from the pixmap pixels.
	// set=0: `color` is a `bgcolor` => clear bit in bmp for pixel == color
	// set=1: `color` is a `fgcolor` => set bit in bmp for pixel == color

	assert(x >= 0 && x + w <= width);
	assert(y >= 0 && y + 1 <= height);

	while (w > 0)
	{
		uint8 byte = set ? 0 : 0xff;
		for (uint m = 1; m < 0x100; m = m << 1)
		{
			if (get_color(x++, y) == color) byte ^= m;
		}

		w -= 8;
		if (w < 0) byte &= 0xff >> -w;
		*z++ = byte;
	}
}


void Canvas::readBmp(coord zx, coord zy, uint8* bmp, int row_offset, coord w, coord h, uint color, bool set) noexcept
{
	// read bitmap from Canvas.
	// for attributed pixmaps the bitmap is constructed from the attribute colors, not just from the pixmap pixels.
	// set=0: `color` is a `bgcolor` => clear bit in bmp for pixel == color
	// set=1: `color` is a `fgcolor` => set bit in bmp for pixel == color

	if (zx < 0)
	{
		w += zx;
		bmp -= zx / 8; // TODO: need x0 offset in read_hline_bmp()
		zx -= zx;
	}
	if (zy < 0)
	{
		h += zy;
		bmp -= zy * row_offset;
		zy -= zy;
	}
	w = min(w, width - zx);
	h = min(h, height - zy);

	if (w > 0)
		while (--h >= 0)
		{
			read_hline_bmp(zx, zy++, w, bmp, color, set);
			bmp += row_offset;
		}
}


void Canvas::drawLine(coord x1, coord y1, coord x2, coord y2, uint color, uint ink) noexcept
{
	// draw arbitrary line from (x1,y1) to (x2,y2)

	// Geometry assumption:
	// pixel at (X,Y) occupies the area between (X,Y) and (X+1,Y+1).
	// the 1-pixel wide line is drawn from (X1,Y1) to (x2,y2) with pixels hanging to bottom right (x+1 and y+1).
	// draws at least 1 pixel (from (x1,y1) to (x2,y2) if (x1,y1) == (x2,y2)).

	if (unlikely(x1 == x2))
	{
		if (y1 > y2) std::swap(y1, y2);
		return drawHLine(x1, y1, y2 - y1 + 1, color, ink);
	}

	if (unlikely(y1 == y2))
	{
		if (x1 > x2) std::swap(x1, x2);
		return drawVLine(x1, y1, x2 - x1 + 1, color, ink);
	}

	// need clipping?
	if (uint(x1) >= uint(width) || uint(x2) >= uint(width) || uint(y1) >= uint(height) || uint(y2) >= uint(height))
	{
		// proper clipping needs calculating the initial dz and the number of pixels to plot
		// but we do it the easy way by intersecting the line with the borders:

		auto cutLineAtX = [=](coord x) { return y1 + (y2 - y1) * (x - x1 + (x2 - x1) / 2) / (x2 - x1); };
		auto cutLineAtY = [=](coord y) { return x1 + (x2 - x1) * (y - y1 + (y2 - y1) / 2) / (y2 - y1); };

		if (uint(x1) >= uint(width))
		{
			if (x1 < 0)
			{
				if (x2 < 0) return;
				y1 = cutLineAtX(0);
				x1 = 0;
			}
			else // if (x1 >= width)
			{
				if (x2 >= width) return;
				y1 = cutLineAtX(width - 1);
				x1 = width - 1;
			}
			if (uint(y1) >= uint(height)) return;
		}

		if (uint(x2) >= uint(width))
		{
			if (x2 < 0)
			{
				y2 = cutLineAtX(0);
				x2 = 0;
			}
			else // if (x2 >= width)
			{
				y2 = cutLineAtX(width - 1);
				x2 = width - 1;
			}
			if (uint(y2) >= uint(height)) return;
		}

		if (uint(y1) >= uint(height))
		{
			if (y1 < 0)
			{
				if (y2 < 0) return;
				x1 = cutLineAtY(0);
				y1 = 0;
			}
			else // if (y1 >= height)
			{
				if (y2 >= height) return;
				x1 = cutLineAtY(height - 1);
				y1 = height - 1;
			}
			if (uint(x1) >= uint(width)) return;
		}

		if (uint(y2) >= uint(height))
		{
			if (y2 < 0)
			{
				x2 = cutLineAtY(0);
				y2 = 0;
			}
			else // if (y2 >= height)
			{
				x2 = cutLineAtY(height - 1);
				y2 = height - 1;
			}
			if (uint(x2) >= uint(width)) return;
		}

		assert(uint(x1) < uint(width) && uint(x2) < uint(width) && uint(y1) < uint(height) && uint(y2) < uint(height));
	}

	coord dx = abs(x2 - x1);
	coord dy = abs(y2 - y1);

	if (dx >= dy) // => advance in x dir
	{
		if (x1 > x2)
		{
			std::swap(x1, x2); // => advance with x++
			std::swap(y1, y2);
		}

		int step = sign(y2 - y1); // step direction for y
		int dz	 = dx / 2;		  // step aside ~~ dy = (dx*dy+dx/2) / dx   => preset dz with "+dx/2" for rounding

		while (x1 <= x2)
		{
			set_pixel(x1++, y1, color, ink);
			dz += dy;
			if (dz >= dx)
			{
				dz -= dx;
				y1 += step;
			}
		}
	}
	else // dy > dx => advance in y dir
	{
		if (y1 > y2)
		{
			std::swap(y1, y2); // => advance with y++
			std::swap(x1, x2);
		}

		coord step = sign(x2 - x1); // step direction for x
		coord dz   = dy / 2;		// step aside ~~ dx = (dx*dy+dy/2) / dy   => preset dz with "+dy/2" for rounding

		while (y1 <= y2)
		{
			set_pixel(x1, y1++, color, ink);
			dz += dx;
			if (dz >= dy)
			{
				dz -= dy;
				x1 += step;
			}
		}
	}
}

void Canvas::drawRect(coord x1, coord y1, coord x2, coord y2, uint color, uint ink) noexcept
{
	// draw outline of rectangle.
	// outline is inset: drawn *between* (x1,y1) and (x2,y2)
	// => outer dimensions are as for fillRect()
	// => nothing is drawn for empty rect!

	sort(x1, x2);
	sort(y1, y2);
	if (x1 >= x2 || y1 >= y2) return;

	drawHLine(x1, y1, x2 - x1, color, ink);
	drawHLine(x1, y2 - 1, x2 - x1, color, ink);
	drawVLine(x1, y1, y2 - y1 - 1, color, ink);
	drawVLine(x2 - 1, y1, y2 - y1 - 1, color, ink);
}

void Canvas::drawCircle(coord x1, coord y1, coord x2, coord y2, uint color, uint ink) noexcept
{
	// draw outline of circle.
	// outline is inset for rect and circle
	// => nothing is drawn for empty circle!

	if (x2 <= x1 || y2 <= y1) return;

	/*	Circle:
		x² + y² = r²
		x² = r² - y²
		y² = r² - x²

		Ellipse:
		(x/a)² + (y/b)² = 1
		(bx)² + (ay)² = (ab)²
		(bx)² = (ab)² - (ay)²
		(ay)² = (ab)² - (bx)²
	*/

	if (x2 - x1 == y2 - y1) // square circle
	{
		// because every point we plot draws a 1*1 pixel rect, we must reduce the diameter by 1
		// and move the center by -0.5,-0.5:

		// center:
		const fixint x0 = fixint(x1 + x2 - 1) * (one / 2);
		const fixint y0 = fixint(y1 + y2 - 1) * (one / 2);

		// radius:
		const fixint r	= fixint(x2 - x1 - 1) * (one / 2); // radius
		const fixint r2 = r * r;						   // r²

		// colorful plotting routine:
		auto setpixels = [this, x0, y0, color, ink](fixint x, fixint y) {
			setPixel(x0 - x, y0 + y, color, ink);
			setPixel(x0 + x, y0 + y, color, ink);
			setPixel(x0 - x, y0 - y, color, ink);
			setPixel(x0 + x, y0 - y, color, ink);
		};

		// if we have an odd number of lines (=> r is xxx.0), then there is a center line at y=0.0.
		// if we have an even number of lines (=> r is xxx.5) then the first line is at y=0.5.

		fixint x = r;			  // start at x=r
		fixint y = r & (one / 2); // and y=0 or y=0.5

		while (y <= x)
		{
			setpixels(x, y); // plot 8 mirrored points
			setpixels(y, x);

			y += one; // step in y direction

			fixint x2_ref = r2 - y * y;
			fixint new_x  = x - one; // step aside in x direction?
			if (abs(x2_ref - new_x * new_x) < abs(x2_ref - x * x)) { x = new_x; }
		}
	}
	else // ellipse
	{
		TODO();
	}
}

void Canvas::fillCircle(coord x1, coord y1, coord x2, coord y2, uint color, uint ink) noexcept
{
	if (x2 <= x1 || y2 <= y1) return;

	/*	Circle:
		x² + y² = r²
		x² = r² - y²
		y² = r² - x²

		Ellipse:
		(x/a)² + (y/b)² = 1
		(bx)² + (ay)² = (ab)²
		(bx)² = (ab)² - (ay)²
		(ay)² = (ab)² - (bx)²
	*/

	if (x2 - x1 == y2 - y1) // square circle
	{
		// because every point we plot draws a 1*1 pixel rect, we must reduce the diameter by 1
		// and move the center by -0.5,-0.5:

		// center:
		const fixint x0 = fixint(x1 + x2 - 1) / 2;
		const fixint y0 = fixint(y1 + y2 - 1) / 2;

		// radius:
		const fixint r	= fixint(x2 - x1 - 1) / 2; // radius
		const fixint r2 = r * r;				   // r²

		auto drawlines = [this, x0, y0, color, ink](fixint x, fixint y) {
			drawHLine(x0 - x, y0 + y, x0 + x + one, color, ink);
			drawHLine(x0 - x, y0 - y, x0 + x + one, color, ink);
		};

		// if we have an odd number of lines (=> r is xxx.0), then there is a center line at y=0.0.
		// if we have an even number of lines (=> r is xxx.5) then the first line is at y=0.5.

		fixint x = r;			  // start at x=r
		fixint y = r & (one / 2); // and y=0 or y=0.5

		drawlines(x, y);

		while (y < r)
		{
			y += one;

			fixint x2_ref =
				r2 - (y - one / 2) * (y - one / 2); // TODO rounding vgl. mit drawcircle contour mismatch ...
		a:
			fixint new_x = x - one;
			if (abs(x2_ref - new_x * new_x) < abs(x2_ref - x * x))
			{
				x = new_x;
				if (y >= x) goto a;
			}

			drawlines(x, y);
		}
	}
	else // ellipse
	{
		TODO();
	}
}

int Canvas::adjust_l(coord l, coord r, coord y, uint ink)
{
	// helper:
	// returns x of the left border of unset pixels
	// returns x == r if no unset pixel was found

	assert(uint(y) < uint(height));
	assert(0 <= l && l < r && r <= width);

	if (getInk(l, y) == ink) // set => adjust to the right until first unset pixel found
	{
		while (++l < r && getInk(l, y) == ink) {}
		return l;
	}
	else // unset => adjust to the left until first unset pixel found
	{
		while (--l >= 0 && getInk(l, y) != ink) {}
		return l + 1;
	}
}


int Canvas::adjust_r(coord l, coord r, coord y, uint ink)
{
	// helper:
	// returns x of the right border of unset pixels
	// returns x == l if no unset pixel was found

	assert(uint(y) < uint(height));
	assert(0 <= l && l < r && r <= width);

	if (getInk(r - 1, y) == ink) // set => adjust to the left until first unset pixel found
	{
		while (--r > l && getInk(r - 1, y) == ink) {}
		return r;
	}
	else // unset => adjust to the right until first unset pixel found
	{
		while (r < width && getInk(r, y) != ink) { r++; }
		return r;
	}
}


void Canvas::floodFill(coord x, coord y, uint color, uint ink)
{
	// TODO:
	// stack on heap, use Array<>?
	// burn-in grid needs stack up to 2k pixel
	// set_pixel()? get_ink()?

	if (!is_inside(x, y)) return;
	if (is_direct_color(colormode)) ink = color;
	if (getInk(x, y) == ink) return;

	constexpr uint ssz = 1 << 9; // fill full screen 1024*768 filled with text: stack usage ~ 346
	class Stack
	{
		struct Data
		{
			// area between (l,y) and (r,y) has been filled. => need to resume in line y+dy.

			uint l	: 10; // left border of filled area
			uint r	: 11; // right border of filled area: 0 <= l < r <= width
			uint y	: 10; // scanline of filled area
			uint dy : 1;  // direction to go: 0 => y+1, 1 => y-1
			Data() {}
			Data(uint l, uint r, uint y, uint dy) : l(l), r(r), y(y), dy(dy) {}
		};
		static_assert(sizeof(Data) == 4);

		Data data[ssz]; // 64*4 = 256 bytes
		uint wi = 0, ri = 0;
		void push(const Data& d)
		{
			assert(free());
			data[wi++ & (ssz - 1)] = d;
			max_usage			   = max(max_usage, avail());
		}
		Data& pop()
		{
			assert(avail());
			return data[ri++ & (ssz - 1)];
		}

	public:
		uint max_usage = 0;
		uint avail() { return wi - ri; }
		uint free() { return ssz - (wi - ri); }
		void push(int l, int r, int y, int dy) { push(Data(uint(l), uint(r), uint(y), dy < 0)); }
		void pop(int& l, int& r, int& y, int& dy)
		{
			Data d = pop();
			l	   = d.l;
			r	   = d.r;
			y	   = d.y;
			dy	   = d.dy ? -1 : +1;
		}
	} stack;

	int x1 = adjust_l(x, x + 1, y, ink);
	int x2 = adjust_r(x, x + 1, y, ink);
	draw_hline(x1, y, x2, color, ink); // fill between x1 and x2
	if (y + 1 < height) { stack.push(x1, x2, y, +1); }
	if (y - 1 >= 0) { stack.push(x1, x2, y, -1); }

	while (stack.avail())
	{
		int l, r, y, dy;
		stack.pop(l, r, y, dy);
		assert(l >= 0 && l < r && r <= width);
		// the pixels between (l,y) and (r,y) have been filled in and we shall resume in line y+dy:
		y += dy;

		int x1 = adjust_l(l, r, y, ink);
		if (x1 == r) continue;			 // no unset pixel found
		int x2 = adjust_r(l, r, y, ink); // note: x1 and x2 may refer to different ranges separated by some set pixels!

		// push ranges left & right of original range, if any:
		if (x1 < l - 1) stack.push(x1, l - 1, y, -dy);
		if (x2 > r + 1) stack.push(r + 1, x2, y, -dy);

		// examine and fill (x1,y) to (x2,y) and push ranges in dy direction:
		for (;;)
		{
			// x1 is the l border of a fill area
			// x2 is the r border of a fill area, maybe the same, maybe another
			// if x1 <= l then range x1 to l+1 is area
			// if x2 >= r then range r-1 to x2 is area

			//int r1 = adjust_r(x1,x1+1,y,px);
			// avoid testing known area pixels:
			int r1 = adjust_r(x1, max(x1, l) + 1, y, ink);
			if (r1 == r) r1 = x2;

			// fill it:
			draw_hline(x1, y, r1, color, ink);

			// push work in dy direction:
			if (uint(y + dy) < uint(height)) stack.push(x1, r1, y, dy);

			// done?
			if (r1 >= x2) break;

			// find l border of next area:
			x1 = adjust_l(r1, r, y, ink);
		}
	}
}

void Canvas::drawPolygon(const Point* p, uint cnt, uint color, uint ink) noexcept
{
	for (uint i = 0; i < cnt - 1; i++) { drawLine(p[i], p[i + 1], color, ink); }
}


} // namespace kio::Graphics


/*
  
























*/
