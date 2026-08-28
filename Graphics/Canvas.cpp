// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Canvas.h"
#include "Array.h"
#include "FlexQueue.h"
#include "fixint.h"
#include <algorithm>


namespace kilipili
{
template<class T>
inline constexpr T max(T a, T b, T c, T d) noexcept
{
	return max(max(a, b, c), d);
}

template<class T>
inline constexpr T min(T a, T b, T c, T d) noexcept
{
	return min(min(a, b, c), d);
}
} // namespace kilipili


namespace kilipili::Graphics
{

void Canvas::draw_hline_to(coord x1, coord y1, coord x2, uint color, uint ink) noexcept
{
	// draw horizontal line from x1:left to x2:right
	// the final pixel at x2 is not drawn: w = x2 - x1.
	// if x2 <= x1 then nothing is drawn. (=> easier clipping)
	// no clipping test for speed

	assert(x1 >= 0 && x2 <= width);
	assert(uint(y1) < uint(height));

	while (x1 < x2) set_pixel(x1++, y1, color, ink);
}

void Canvas::drawHLine(coord x1, coord y1, coord w, uint color, uint ink) noexcept
{
	// draw horizontal line from min(x1,x2) to max(x1,x2).
	// the final rightmost pixel is not drawn: w = x2 - x1.
	// full clipping to screen.

	if (uint(y1) >= uint(height)) return;

	if (w >= 0) draw_hline_to(max(x1, 0), y1, min(x1 + w, width), color, ink);
	else draw_hline_to(max(x1 + w, 0), y1, min(x1, width), color, ink);
}

void Canvas::drawHLineTo(coord x1, coord y1, coord x2, uint color, uint ink) noexcept
{
	// draw horizontal line from min(x1,x2) to max(x1,x2).
	// the final rightmost pixel is not drawn: w = x2 - x1.
	// full clipping to screen.

	if (uint(y1) >= uint(height)) return;

	if (x2 >= x1) draw_hline_to(max(x1, 0), y1, min(x2, width), color, ink);
	else draw_hline_to(max(x2, 0), y1, min(x1, width), color, ink);
}

void Canvas::draw_vline_to(coord x1, coord y1, coord y2, uint color, uint ink) noexcept
{
	// draw vertical line from y1:top to y2:bottom
	// the final pixel at y2 is not drawn: h = y2 - y1.
	// if y2 <= y1 then nothing is drawn. (=> easier clipping)
	// no clipping test for speed

	assert(uint(x1) < uint(width));
	assert(y1 >= 0 && y2 <= height);

	while (y1 < y2) set_pixel(x1, y1++, color, ink);
}

void Canvas::drawVLine(coord x1, coord y1, coord h, uint color, uint ink) noexcept
{
	// draw vertical line from min(y1,y2) to max(y1,y2).
	// the final bottommost pixel is not drawn: h = y2 - y1.
	// full clipping to screen.

	if (uint(x1) >= uint(width)) return;

	if (h >= 0) draw_vline_to(x1, max(y1, 0), min(y1 + h, height), color, ink);
	else draw_vline_to(x1, max(y1 + h, 0), min(y1, height), color, ink);
}

void Canvas::drawVLineTo(coord x1, coord y1, coord y2, uint color, uint ink) noexcept
{
	// draw vertical line from min(y1,y2) to max(y1,y2).
	// the final bottommost pixel is not drawn: h = y2 - y1.
	// full clipping to screen.

	if (uint(x1) >= uint(width)) return;

	if (y2 >= y1) draw_vline_to(x1, max(y1, 0), min(y2, height), color, ink);
	else draw_vline_to(x1, max(y2, 0), min(y1, height), color, ink);
}

void Canvas::fillRect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	// draw filled rectangle

	coord x2 = min(x1 + w, width);
	coord y2 = min(y1 + h, height);
	x1		 = max(x1, 0);
	y1		 = max(y1, 0);

	if (x1 < x2)
		while (y1 < y2) draw_hline_to(x1, y1++, x2, color, ink);
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

static inline void advance(coord& x, coord& y, int& error, int dx, int dy, int n)
{
	// advance (x,y,error) n steps along dy/dx

	assert(n >= 0);
	assert(dx >= 0);
	assert(dy >= 0);
	assert(dx >= dy);
	assert(error < dx && error >= 0);

	x += n;
	error += n * dy;
	y += error / dx;
	error = error % dx;
}

static inline int clip_line(coord& x, coord& y, int dx, int dy, int& error, int width, int height)
{
	// clip line for drawLine():
	// formula used here must match exactly drawLine(), else points may be drawn outside the screen!
	//
	// returns number of pixels to set, which may be 0
	// if n=0 then x1, y1 and error are void.
	// x1 and y1 are modified if clipped.
	// error is set to the dy/dx error at the original or modified point x1,y1

	// assert that clipping is actually needed:
	// this is tested before calling clip_line() in drawLine()
	assert(
		uint(x) >= uint(width) || uint(x + dx) >= uint(width) || //
		uint(y) >= uint(height) || uint(y + dy) >= uint(height));

	// use geometrical symmetries to reduce the number of cases:

	bool fx = dx < 0; // flip hor
	if (fx)
	{
		x  = width - 1 - x;
		dx = -dx;
	}
	if (x >= width) return 0; // fully right of screen => no points to draw
	if (x + dx < 0) return 0; // fully left of screen

	bool fy = dy < 0; // flip vert
	if (fy)
	{
		y  = height - 1 - y;
		dy = -dy;
	}
	if (y >= height) return 0; // fully below screen
	if (y + dy < 0) return 0;  // fully above screen

	// assert that clipping is still needed:
	assert(x < 0 || y < 0 || x + dx >= width || y + dy >= height);

	bool fxy = dx < dy; // flip x <-> y
	if (fxy)
	{
		swap(x, y);
		swap(dx, dy);
		swap(width, height);
	}

	// now clip the line:

	assert(dx >= dy); // we draw advancing in x direction, stepping aside in y direction
	assert(dx >= 0);  // we draw with increasing x (left to right)
	assert(dy >= 0);  // we draw with increasing y (top to bottom)

	// cases:
	// - y2 end below screen
	// - y1 start above screen
	// - x2 end right of screen
	// - x1 start left of screen

	error = dx / 2;
	int w = dx + 1; // number of points to set

	if (y + dy >= height) // line ends below screen
	{
		w = ((height - y) * dx - error + dy - 1) / dy; // first point outside screen
		assert(w > 0);

		if constexpr (debug)
		{
			coord x1 = x, y1 = y;
			int	  error1 = error;
			advance(x1, y1, error1, dx, dy, w - 1);
			assert(y1 == height - 1);
			assert(x1 == x + w - 1);
		}

		// check whether the line crossed the lower border left of the screen:
		if (x + w <= 0) return 0; // fully left of screen
	}

	if (x + w > width) // line ends right of screen
	{
		w = width - x;
		assert(w > 0);
	}

	if (y < 0) // line starts above screen
	{
		int n = (-y * dx - error + dy - 1) / dy; // first point inside screen
		assert(n >= 0);
		advance(x, y, error, dx, dy, n);
		assert(y == 0);

		// check whether the line crossed the upper border right of the screen:
		if (x >= width) return 0; // fully right of screen
		w -= n;
		assert(w > 0);
	}

	if (x < 0) // line starts left of screen
	{
		int n = -x;
		advance(x, y, error, dx, dy, n);
		assert(x == 0);

		assert(y < height); // fully below screen: can no longer happen here
		//if (y >= height) return 0;
		w -= n;
		assert(w > 0);
	}

	if (fxy)
	{
		swap(x, y);
		swap(width, height);
	}
	if (fy) { y = height - 1 - y; }
	if (fx) { x = width - 1 - x; }

	return w;
}

void Canvas::drawLine(coord x1, coord y1, coord x2, coord y2, uint color, uint ink) noexcept
{
	// draw arbitrary line from (x1,y1) to (x2,y2) incl.

	if unlikely (x1 == x2)
	{
		if (y1 > y2) swap(y1, y2);
		return drawVLine(x1, y1, y2 - y1 + 1, color, ink);
	}

	if unlikely (y1 == y2)
	{
		if (x1 > x2) swap(x1, x2);
		return drawHLine(x1, y1, x2 - x1 + 1, color, ink);
	}

	int dx	  = abs(x2 - x1);
	int dy	  = abs(y2 - y1);
	int n	  = max(dx, dy);
	int error = n / 2;
	n += 1;

	// clip:
	if (uint(x1) >= uint(width) || uint(x2) >= uint(width) || uint(y1) >= uint(height) || uint(y2) >= uint(height))
	{
		n = clip_line(x1, y1, x2 - x1, y2 - y1, error, width, height);
		if (n <= 0) return;
	}

	int step_x = x2 > x1 ? +1 : -1;
	int step_y = y2 > y1 ? +1 : -1;

	if (dx >= dy) // => advance in x dir
	{
		while (--n >= 0)
		{
			set_pixel(x1, y1, color, ink);
			x1 += step_x;
			error += dy;
			if (error >= dx)
			{
				error -= dx;
				y1 += step_y;
			}
		}
	}
	else // dy > dx => advance in y dir
	{
		while (--n >= 0)
		{
			set_pixel(x1, y1, color, ink);
			y1 += step_y;
			error += dx;
			if (error >= dy)
			{
				error -= dy;
				x1 += step_x;
			}
		}
	}
}

void Canvas::drawRect(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	// draw outline of rectangle.
	// outline is inset: drawn *between* (x1,y1) and (x2,y2)
	// => outer dimensions are as for fillRect()
	// => nothing is drawn for empty rect!

	if (w > 0 && h > 0)
	{
		drawHLine(x1, y1, w, color, ink);
		drawHLine(x1, y1 + h - 1, w, color, ink);
		drawVLine(x1, y1, h - 1, color, ink);
		drawVLine(x1 + w - 1, y1, h - 1, color, ink);
	}
}

void Canvas::drawCircle(coord x1, coord y1, coord w, coord h, uint color, uint ink) noexcept
{
	// draw outline of circle.
	// outline is inset for rect and circle
	// => nothing is drawn for empty circle!

	if (w <= 0 || h <= 0) return;

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

	if (w == h) // square circle
	{
		// because every point we plot draws a 1*1 pixel rect, we must reduce the diameter by 1
		// and move the center by -0.5,-0.5:

		// center:
		const fixint x0 = fixint(x1 + x1 + w - 1) * (one / 2);
		const fixint y0 = fixint(y1 + y1 + h - 1) * (one / 2);

		// radius:
		const fixint r	= fixint(w - 1) * (one / 2); // radius
		const fixint r2 = r * r;					 // r²

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
			drawHLine(x0 - x, y0 + y, x * 2 + one, color, ink);
			drawHLine(x0 - x, y0 - y, x * 2 + one, color, ink);
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
	// burn-in grid needs 2k*sizeof(Data) stack (max_usage=1025)
	// set_pixel()? get_ink()?

	if (!is_inside(x, y)) return;
	if (is_direct_color(colormode)) ink = color;
	if (getInk(x, y) == ink) return;


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

	struct Stack : public FlexQueue<Data, 64>
	{
		//uint max_usage = 0;

		void push(int l, int r, int y, int dy)
		{
			put(Data(uint(l), uint(r), uint(y), dy < 0));
			//max_usage = max(max_usage, avail());
		}
		void pop(int& l, int& r, int& y, int& dy)
		{
			Data d = get();
			l	   = d.l;
			r	   = d.r;
			y	   = d.y;
			dy	   = d.dy ? -1 : +1;
		}
	} stack;

	int x1 = adjust_l(x, x + 1, y, ink);
	int x2 = adjust_r(x, x + 1, y, ink);
	draw_hline_to(x1, y, x2, color, ink); // fill between x1 and x2
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
			draw_hline_to(x1, y, r1, color, ink);

			// push work in dy direction:
			if (uint(y + dy) < uint(height)) stack.push(x1, r1, y, dy);

			// done?
			if (r1 >= x2) break;

			// find l border of next area:
			x1 = adjust_l(r1, r, y, ink);
		}
	}

	//fprintf(stderr, "stack.max_usage = %u\n", stack.max_usage);
}

void Canvas::drawPolygon(const Point* p, uint cnt, uint color, uint ink) noexcept
{
	for (uint i = 0; i < cnt - 1; i++) { drawLine(p[i], p[i + 1], color, ink); }
}

void Canvas::drawTriangle(const Point& p1, const Point& p2, const Point& p3, uint color, uint ink) noexcept
{
	drawLine(p1, p2, color, ink);
	drawLine(p2, p3, color, ink);
	drawLine(p3, p1, color, ink);
}

struct VLine
{
	/*	Helper struct for vertically running left & right border lines for filling shapes. */

	VLine() = default; // uninitialized
	VLine(const Point* p1, const Point* p2);

	int	 next_x();
	bool operator<(const VLine& other) const { return x < other.x; }

	const Point* p1;
	const Point* p2;
	int			 x;
	uint		 dx, dy; // >= 0
	uint		 error;	 //
	int			 min_dx; // signed
	int			 max_dx;
};

inline VLine::VLine(const Point* p1, const Point* p2) :
	p1(p1),
	p2(p2),
	x(p1->x),
	dx(abs(p2->x - p1->x)),
	dy(p2->y - p1->y),
	error(dy / 2)
{
	assert(dy > 0);

	int m = 0;
	int s = sign(p2->x - x);
	for (; dx > dy; dx -= dy) { m += s; }
	min_dx = m;
	max_dx = m + s;
}

inline int VLine::next_x()
{
	error += dx;
	if (error < dy) return x += min_dx;
	error -= dy;
	return x += max_dx;
}


struct PolyPoints
{
	/*	Helper struct fore fillPolygon() to wrap an array of points:
		=> handle points via pointer (not index).
		=> step to left or right neighbour incl. wrap at start & end. */

	const Point* const points;
	const int		   count;

	constexpr PolyPoints(const Point* p, int n) noexcept : points(p), count(n) {}

	constexpr bool is_valid(const Point* p) const noexcept { return uint(p - points) < uint(count); }

	constexpr const Point* first() const noexcept { return points; }
	constexpr const Point* last() const noexcept { return points + count - 1; }

	constexpr const Point* before(const Point* p) const noexcept
	{
		assert(is_valid(p));
		return p > points ? p - 1 : &points[count - 1];
	}
	constexpr const Point* after(const Point* p) const noexcept
	{
		assert(is_valid(p));
		return ++p < points + count ? p : &points[0];
	}
	constexpr const Point* next(const Point* p1, const Point* p2) const noexcept
	{
		assert(is_valid(p1));
		assert(is_valid(p2));

		const Point* p = p2 + (p2 - p1);
		if (is_valid(p)) return p;
		p = p < points ? p + count : p - count;
		assert(is_valid(p));
		return p;
	}
	constexpr bool is_top_point(const Point* p) const noexcept
	{
		// a top point has 2 neighbours with both higher y.
		// if 2 or more points are the same height, then the last one (with the highest i) ist used.

		assert(is_valid(p));

		int y = p->y;
		if (after(p)->y <= y) return false; // else right neighbour is lower
		for (const Point* p0 = p;;)
		{
			p = before(p);
			if (p->y != y) return y < p->y;
			if (p == p0) return false; // all points on same scanline => no top point at all!
		}
	}
};

void Canvas::fillTriangle(Point p1, Point p2, Point p3, uint color, uint ink) noexcept
{
	/*	In general a triangle has 3 points: 1 point is top, 1 point is bottom and one point in between.
		The triangle is split horizontally at the middle point:
		an upper "standing" triangle with a horizonal base line and
		a lower "hanging" triangle with a horizontal top line.
		If the triangle itself has a horizontal base line then there is no lower triangle.
		If the triangle itself has a horizontal top line then there is no upper triangle.
		If all points are on the same scanline, then the triangle is empty. 

		We want that adjacent triangles fit nicely without gap and overlap.
		=> the left and the right border are calculated by VLine with the exact same algorithm.
		if the left and right point of the hlines were drawn, then we'd have a 1 pixel overlap.
		=> right pixel is not drawn, as in drawHLine() and equivalent with fillRect() and fillCircle().
		
		The top and bottom pixel of the triangle are not drawn, because l+r border of the hline are the same point => w=0.

		The horizontal base of the upper triangle and top border of the lower triangle should not be drawn twice.
		this should also be true for two separate triangles which share a horizontal border.
		=> the lower horizonal border line is not drawn, which is consistent with fillRect() and fillCircle().

		The fill is narrower than the outline of drawTriangle(). 
		Call drawTriangle() as a second step to enlarge the fill or to draw a border in a different color.
	*/

	// order points y1 <= y2 <= y3:
	if (p1.y > p2.y) swap(p1, p2);
	if (p1.y > p3.y) swap(p1, p3);
	if (p2.y > p3.y) swap(p2, p3);
	int y = p1.y;

	// p1 is the top point, p2 is the middle point, p3 is the bottom point.

	// does triangle start with a hline at top?
	if unlikely (p2.y == y)
	{
		if unlikely (p3.y == y) return; // y1=y2=y3

		VLine left(&p2, &p3);  // may be vice versa
		VLine right(&p1, &p3); //

		drawHLineTo(left.x, y, right.x, color, ink);
		while (++y < p3.y) { drawHLineTo(left.next_x(), y, right.next_x(), color, ink); }
	}
	else
	{
		VLine left(&p1, &p2);  // may be vice versa
		VLine right(&p1, &p3); //

		while (++y < p2.y) { drawHLineTo(left.next_x(), y, right.next_x(), color, ink); }
		if (y == p3.y) return;

		drawHLineTo(p2.x, y, right.next_x(), color, ink);

		new (&left) VLine(&p2, &p3);

		while (++y < p3.y) { drawHLineTo(left.next_x(), y, right.next_x(), color, ink); }
	}
}

#if 0
void fillPolygon(
	const Point* _points, uint _count, std::function<void(coord x, coord y, coord x2)> drawHLineTo) noexcept
#endif


void Canvas::fillPolygon(const Point* _points, uint _count, uint color, uint ink)
{
	/*
		to fill the polygon we move scanline for scanline from the top point to the lowest point.
		we keep track of the currently involved edge lines. these are always a multiple of 2.
		the polygon can have multiple "top" points.
		whenever we encounter a top point 2 more border lines are added.
		therefore we must search for all top points first.
		scanlines are drawn from left to right between each pair of lines, skipping between the pairs.
		this results in XOR mode.
		whenever 2 lines cross each other, we swap them to keep l2r sorted order.
		whenever a line reaches the next vertice point, we update the line to reflect the new line segment.
		whenever a line goes up again, this reached a 'bottom' point and we remove it from the list.		
	*/

	assert(_points != nullptr || _count == 0);

	if (_count <= 2) return;
	if (_points[0] == _points[_count - 1] && --_count <= 2) return;
	//if (_count == 3) return fillTriangle(_points, color, ink);

	// wrap supplied data:
	PolyPoints points(_points, _count);

	// find all 'top' points:
	// these are points with both neighbours lower than them. (higher y)
	// a polygon with N points can have up to N/2 top points!
	// then we may have up to N active edge lines while rendering!
	// topmost top point is at the end of the array

	Array<const Point*, 20> top_points;
	for (const Point* p = points.last(); p >= points.first(); p--)
	{
		if (!points.is_top_point(p)) continue;
		top_points.insertsorted(p, [](const Point* a, const Point* b) { return a->y > b->y; });
	}
	if (top_points.count() == 0) return;				   // exit if all points on same scanline
	assert(top_points.first()->y >= top_points.last()->y); // assert ordering: topmost point is last

	// array for active edge lines.
	// they always come in pairs.
	Array<VLine, 8> vlines;

	// *** do it! ***
	for (int y = top_points.last()->y;; y++)
	{
		// prepare to draw scanline y:

		// -> check if at vertice point in active edge lines
		// -> skip over hlines
		// -> check for next edge going up again: remove at bottom point & check if finished

		for (uint i = 0; i < vlines.count(); i++)
		{
			VLine& line = vlines[i];
			if (y < line.p2->y) continue; // not yet at vertice point p2
			const Point* p1 = line.p1;
			const Point* p2 = line.p2;
			const Point* p3 = points.next(p1, p2); // next vertice point
			while (p3->y == y)
			{
				p1 = p2;
				p2 = p3;
				p3 = points.next(p1, p2); // skip hline
			}
			if (p3->y > y) new (&line) VLine(p2, p3);		   // setup line for next edge
			else if (vlines.count() > 2) vlines.removeat(i--); // p3.y<p.y => bottom point
			else											   // vlines empty => finished
			{
				assert(top_points.count() == 0); // must also be empty then
				return;
			}
		}

		// -> check for new top points
		// -> skip over hlines

		while (top_points.count() && top_points.last()->y == y)
		{
			const Point* p1 = top_points.pop();
			const Point* p2 = points.after(p1); // no hline here. see is_top_point()
			vlines.insertsorted(VLine(p1, p2));

			p2 = points.before(p1);
			while (p2->y == p1->y)
			{
				p1 = p2;
				p2 = points.before(p2); // skip hline
			}
			vlines.insertsorted(VLine(p1, p2));
		}

		// draw lines
		// advance edge line to next y
		// advance y: in for(;;y++)

		for (uint i = 1; i < vlines.count(); i++) // sort: normally very little re-arangement is needed
		{
			while (i && vlines[i - 1].x > vlines[i].x)
			{
				std::swap(vlines[i - 1], vlines[i]);
				i--;
			}
		}

		assert((vlines.count() & 1) == 0);
		for (uint i = 0; i < vlines.count(); i += 2)
		{
			if (uint(y) < uint(height)) //
				draw_hline_to(max(vlines[i].x, 0), y, min(vlines[i + 1].x, width), color, ink);
			vlines[i].next_x();
			vlines[i + 1].next_x();
		}
	}
}

void Canvas::drawBezier_f(
	const Point& p0, const Point& p1, const Point& p2, const Point& p3, uint color, uint ink) noexcept
{
	/*	Draw a cubic Bezier curve using float.
		p0: start point
		p1: control point 
		p2: control point 
		p3: end point
		version for reference.
	*/

	int	  sx	= max(p0.x, p1.x, p2.x, p3.x) - min(p0.x, p1.x, p2.x, p3.x);
	int	  sy	= max(p0.y, p1.y, p2.y, p3.y) - min(p0.y, p1.y, p2.y, p3.y);
	int	  steps = max(sx, sy);
	float step	= 1.0f / float(steps);

	// Calculate the coordinates using the cubic Bezier formula
	// B(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3


	setPixel(p0, color, ink);
	setPixel(p3, color, ink);

	float p0x = float(p0.x);
	float p0y = float(p0.y);
	float p1x = float(p1.x);
	float p1y = float(p1.y);
	float p2x = float(p2.x);
	float p2y = float(p2.y);
	float p3x = float(p3.x);
	float p3y = float(p3.y);

	// for detecting gaps between last & current point:
	float x0 = p0x;
	float y0 = p0y;
	float x3 = p3x;
	float y3 = p3y;

	for (float t = 0, u = 1; t <= u; t += step, u -= step)
	{
		float tt   = t * t;
		float ttu3 = tt * u * 3;
		float ttt  = tt * t;

		float uu   = u * u;
		float uut3 = uu * t * 3;
		float uuu  = uu * u;

		float x = p0x * uuu + p1x * uut3 + p2x * ttu3 + p3x * ttt;
		float y = p0y * uuu + p1y * uut3 + p2y * ttu3 + p3y * ttt;
		setPixel(int(x), int(y), color, ink);

		if (abs(int(x) - int(x0)) > 1 || abs(int(y) - int(y0)) > 1) // gap?
			setPixel(int((x + x0) / 2), int((y + y0) / 2), color, ink);
		x0 = x;
		y0 = y;

		x = p3x * uuu + p2x * uut3 + p1x * ttu3 + p0x * ttt;
		y = p3y * uuu + p2y * uut3 + p1y * ttu3 + p0y * ttt;
		setPixel(int(x), int(y), color, ink);

		if (abs(int(x) - int(x3)) > 1 || abs(int(y) - int(y3)) > 1) // gap?
			setPixel(int((x + x3) / 2), int((y + y3) / 2), color, ink);
		x3 = x;
		y3 = y;
	}
}

void Canvas::drawBezier(
	const Point& p0, const Point& p1, const Point& p2, const Point& p3, uint color, uint ink) noexcept
{
	/*	Draw a cubic Bezier curve.
		p0: start point
		p1: control point 
		p2: control point 
		p3: end point
	*/

	// use fixed-point math: ss = number of fractional bits:
	constexpr int ss  = 15;
	constexpr int ssh = ((0)) ? 1 << (ss - 1) : 0; // rounding?

	// estimate number of points to set:
	int sx	  = max(p0.x, p1.x, p2.x, p3.x) - min(p0.x, p1.x, p2.x, p3.x);
	int sy	  = max(p0.y, p1.y, p2.y, p3.y) - min(p0.y, p1.y, p2.y, p3.y);
	int steps = min(max(sx, sy), 1 << ss);
	int step  = (1 << ss) / steps + 1;

	// for detecting gaps between last & current point:
	int x0 = p0.x << ss;
	int y0 = p0.y << ss;
	int x3 = p3.x << ss;
	int y3 = p3.y << ss;
	setPixel(x0, y0, color, ink);
	setPixel(x3, y3, color, ink);

	// do it:
	for (int t = step, u = (1 << ss) - step; t <= u; t += step, u -= step)
	{
		// calculate the coordinates using the cubic Bezier formula:
		// P(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3

		int tt	 = (t * t + ssh) >> ss;
		int ttu3 = (tt * u * 3 + ssh) >> ss;
		int ttt	 = (tt * t + ssh) >> ss;

		int uu	 = (u * u + ssh) >> ss;
		int uut3 = (uu * t * 3 + ssh) >> ss;
		int uuu	 = (uu * u + ssh) >> ss;

		int x = p0.x * uuu + p1.x * uut3 + p2.x * ttu3 + p3.x * ttt + 0 * ssh;
		int y = p0.y * uuu + p1.y * uut3 + p2.y * ttu3 + p3.y * ttt + 0 * ssh;
		setPixel(x >> ss, y >> ss, color, ink);

		if (abs((x >> ss) - (x0 >> ss)) > 1 || abs((y >> ss) - (y0 >> ss)) > 1) // gap?
			setPixel((x + x0) >> (ss + 1), (y + y0) >> (ss + 1), color, ink);	// 1 point should be enough
		x0 = x;
		y0 = y;

		x = p3.x * uuu + p2.x * uut3 + p1.x * ttu3 + p0.x * ttt + 0 * ssh;
		y = p3.y * uuu + p2.y * uut3 + p1.y * ttu3 + p0.y * ttt + 0 * ssh;
		setPixel(x >> ss, y >> ss, color, ink);

		if (abs((x >> ss) - (x3 >> ss)) > 1 || abs((y >> ss) - (y3 >> ss)) > 1) // gap?
			setPixel((x + x3) >> (ss + 1), (y + y3) >> (ss + 1), color, ink);
		x3 = x;
		y3 = y;
	}
}


} // namespace kilipili::Graphics


/*
  




	
































*/
