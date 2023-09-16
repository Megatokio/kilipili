// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "DrawEngine.h"
#include "BitBlit.h"
#include "Pixmap.h"
#include "Pixmap_wAttr.h"
#include "utilities/PwmLoadSensor.h"
#include "utilities/utilities.h"
#include "fixint.h"
#include <functional>
#include <pico/stdlib.h>
#include <string.h>


namespace kipili::Graphics
{


template<ColorMode CM>
void DrawEngine<CM>::clearScreen(uint bgcolor)
{
	// clear pixels and attributes to bgcolor
	// bgcolor is true color or indexed color acc. to color mode
	// does not modify indexed color map
	// in attribute modes the pixels are set to 0

	// TODO: wait for ffb

	pixmap.clear(bgcolor);
}

template<ColorMode CM>
void DrawEngine<CM>::scrollScreen(coord dx, coord dy, uint bgcolor)
{
	// scroll with cpu power
	// TODO: "hardware scroll" by moving the screen start address

	int w = width() < abs(dx);
	int h = height() < abs(dy);

	if (w <= 0 || h <= 0) return clearScreen(bgcolor);

	Point q {0, 0};
	Point z {0, 0};

	if (dx)
	{
		if (dx < 0) q.x -= dx;
		else z.x += dx;
	}
	if (dy)
	{
		if (dy < 0) q.y -= dy;
		else z.y += dy;
	}

	pixmap.copyRect(z, pixmap, Rect(q.x, q.y, w, h));

	if (dx)
	{
		if (dx < 0) pixmap.fillRect(w, 0, width() - w, height(), bgcolor);
		else pixmap.fillRect(0, 0, dx, height(), bgcolor);
	}
	if (dy)
	{
		if (dy < 0) pixmap.fillRect(0, h, width(), height() - h, bgcolor);
		else pixmap.fillRect(0, 0, width(), dy, bgcolor);
	}
}

template<ColorMode CM>
void DrawEngine<CM>::drawLine(coord x, coord y, coord x2, coord y2, uint color, uint pixel)
{
	// draw arbitrary line from (x1,y1) to (x2,y2)
	// draws at least 1 pixel

	coord dx = abs(x2 - x);
	coord dy = abs(y2 - y);

	if (dx >= dy) // => advance in x dir
	{
		if (x > x2)
		{
			std::swap(x, x2); // => advance with x++
			std::swap(y, y2);
		}

		if (y != y2)
		{
			int step = sign(y2 - y); // step direction for y
			int dz	 = dx / 2;		 // step aside ~~ dy = (dx*dy+dx/2) / dx   => preset dz with "+dx/2" for rounding

			while (x <= x2)
			{
				setPixel(x++, y, color, pixel);
				dz += dy;
				if (dz >= dx)
				{
					dz -= dx;
					y += step;
				}
			}
		}
		else { drawHorLine(x, y, x2 + 1, color, pixel); }
	}
	else // dy > dx => advance in y dir
	{
		if (y > y2)
		{
			std::swap(y, y2); // => advance with y++
			std::swap(x, x2);
		}

		if (x != x2)
		{
			coord step = sign(x2 - x); // step direction for x
			coord dz   = dy / 2;	   // step aside ~~ dx = (dx*dy+dy/2) / dy   => preset dz with "+dy/2" for rounding

			while (y <= y2)
			{
				setPixel(x, y++, color, pixel);
				dz += dx;
				if (dz >= dy)
				{
					dz -= dy;
					x += step;
				}
			}
		}
		else { drawVertLine(x, y, y2 + 1, color, pixel); }
	}
}

template<ColorMode CM>
void DrawEngine<CM>::drawRect(coord x, coord y, coord x2, coord y2, uint color, uint pixel)
{
	// draw outline of rectangle.
	// outline is inset for rect and circle
	// => nothing is drawn for empty rect!

	sort(x, x2);
	sort(y, y2);
	if (x >= x2 || y >= y2) return;

	drawHorLine(x, y, x2, color, pixel);
	drawHorLine(x, y2 - 1, x2, color, pixel);
	drawVertLine(x, y, y2 - 1, color, pixel);
	drawVertLine(x2 - 1, y, y2 - 1, color, pixel);
}

template<ColorMode CM>
void DrawEngine<CM>::drawCircle(coord x, coord y, coord x2, coord y2, uint color, uint pixel)
{
	drawCircle(Rect(x, y, x2, y2), color, pixel);
}

template<ColorMode CM>
void DrawEngine<CM>::drawCircle(const Rect& rect, uint color, uint pixel)
{
	// draw outline of circle.
	// outline is inset for rect and circle
	// => nothing is drawn for empty circle!

	assert(rect.is_normalized());
	if (rect.is_empty()) return;

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

	if (rect.width() == rect.height()) // square circle
	{
		// because every point we plot draws a 1*1 pixel rect, we must reduce the diameter by 1
		// and move the center by -0.5,-0.5:

		// center:
		const fixint x0 = fixint(rect.p1.x + rect.p2.x - 1) * (one / 2);
		const fixint y0 = fixint(rect.p1.y + rect.p2.y - 1) * (one / 2);

		// radius:
		const fixint r	= fixint(rect.width() - 1) * (one / 2); // radius
		const fixint r2 = r * r;								// r²

		// colorful plotting routine:
		auto setpixels = [this, x0, y0, color, pixel](fixint x, fixint y) {
			setPixel(x0 - x, y0 + y, color, pixel);
			setPixel(x0 + x, y0 + y, color, pixel);
			setPixel(x0 - x, y0 - y, color, pixel);
			setPixel(x0 + x, y0 - y, color, pixel);
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

template<ColorMode CM>
void DrawEngine<CM>::fillCircle(coord x, coord y, coord x2, coord y2, uint color, uint pixel)
{
	fillCircle(Rect(x, y, x2, y2), color, pixel);
}

template<ColorMode CM>
void DrawEngine<CM>::fillCircle(const Rect& rect, uint color, uint pixel)
{
	assert(rect.is_normalized());
	if (rect.is_empty()) return;

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

	if (rect.width() == rect.height()) // square circle
	{
		// because every point we plot draws a 1*1 pixel rect, we must reduce the diameter by 1
		// and move the center by -0.5,-0.5:

		// center:
		const fixint x0 = fixint(rect.p1.x + rect.p2.x - 1) / 2;
		const fixint y0 = fixint(rect.p1.y + rect.p2.y - 1) / 2;

		// radius:
		const fixint r	= fixint(rect.width() - 1) / 2; // radius
		const fixint r2 = r * r;						// r²

		auto drawlines = [this, x0, y0, color, pixel](fixint x, fixint y) {
			drawHorLine(x0 - x, y0 + y, x0 + x + one, color, pixel);
			drawHorLine(x0 - x, y0 - y, x0 + x + one, color, pixel);
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

template<ColorMode CM>
void DrawEngine<CM>::drawPolygon(const Point* p, uint cnt, uint color, uint pixel)
{
	for (uint i = 0; i < cnt - 1; i++) { drawLine(p[i], p[i + 1], color, pixel); }
}

template<ColorMode CM>
void DrawEngine<CM>::fillPolygon(const Point*, uint cnt, uint color, uint pixel)
{
	TODO();
}

template<ColorMode CM>
int DrawEngine<CM>::adjust_l(coord l, coord r, coord y, uint pixel)
{
	// returns x of the left border of unset pixels
	// returns x == r if no unset pixel was found

	assert(uint(y) < uint(height()));
	assert(0 <= l && l < r && r <= width());

	if (getPixel(l, y) == pixel) // set => adjust to the right until first unset pixel found
	{
		while (++l < r && getPixel(l, y) == pixel) {}
		return l;
	}
	else // unset => adjust to the left until first unset pixel found
	{
		while (--l >= 0 && getPixel(l, y) != pixel) {}
		return l + 1;
	}
}

template<ColorMode CM>
int DrawEngine<CM>::adjust_r(coord l, coord r, coord y, uint pixel)
{
	// returns x of the right border of unset pixels
	// returns x == l if no unset pixel was found

	assert(uint(y) < uint(height()));
	assert(0 <= l && l < r && r <= width());

	if (getPixel(r - 1, y) == pixel) // set => adjust to the left until first unset pixel found
	{
		while (--r > l && getPixel(r - 1, y) == pixel) {}
		return r;
	}
	else // unset => adjust to the right until first unset pixel found
	{
		while (r < width() && getPixel(r, y) != pixel) { r++; }
		return r;
	}
}

template<ColorMode CM>
void DrawEngine<CM>::fill(coord x, coord y, uint color, uint pixel)
{
	if (!in_screen(x, y)) return;
	if (is_direct_color(CM)) pixel = color;
	if (getPixel(x, y) == pixel) return;

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

	uint32 start = time_us_32();

	int x1 = adjust_l(x, x + 1, y, pixel);
	int x2 = adjust_r(x, x + 1, y, pixel);
	draw_hor_line(x1, y, x2, color, pixel); // fill between x1 and x2
	if (y + 1 < height()) { stack.push(x1, x2, y, +1); }
	if (y - 1 >= 0) { stack.push(x1, x2, y, -1); }

	while (stack.avail())
	{
		int l, r, y, dy;
		stack.pop(l, r, y, dy);
		assert(l >= 0 && l < r && r <= width());
		// the pixels between (l,y) and (r,y) have been filled in and we shall resume in line y+dy:
		y += dy;

		int x1 = adjust_l(l, r, y, pixel);
		if (x1 == r) continue; // no unset pixel found
		int x2 =
			adjust_r(l, r, y, pixel); // note: x1 and x2 may refer to different ranges separated by some set pixels!

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
			int r1 = adjust_r(x1, max(x1, l) + 1, y, pixel);
			if (r1 == r) r1 = x2;

			// fill it:
			draw_hor_line(x1, y, r1, color, pixel);

			// push work in dy direction:
			if (uint(y + dy) < uint(height())) stack.push(x1, r1, y, dy);

			// done?
			if (r1 >= x2) break;

			// find l border of next area:
			x1 = adjust_l(r1, r, y, pixel);
		}
	}

	uint32 dur = time_us_32() - start;
	printf("flood fill: max stack usage = %u\n", stack.max_usage);
	printf("flood fill: duration = %u.%03u ms\n", dur / 1000, dur % 1000);
}


template<ColorMode CM>
void DrawEngine<CM>::writeBmpToScreen(coord x, coord y, coord w, coord h, const uint8 bmp[], uint fgcolor, uint bgcolor)
{
	for (int iy = 0; iy < h; iy++)
	{
		uint8 byte = bmp[iy];
		for (int ix = 0; ix < w; byte >>= 1, ix++)
		{
			if (byte & 1)
			{
				if (fgcolor != DONT_CLEAR) setPixel(x + ix, y + iy, fgcolor, 1);
			}
			else
			{
				if (bgcolor != DONT_CLEAR) setPixel(x + ix, y + iy, bgcolor, 0);
			}
		}
	}
}

template<ColorMode CM>
void DrawEngine<CM>::readBmpFromScreen(coord x, coord y, coord w, coord h, uint8[], uint bgcolor)
{
	TODO();
}


template<ColorMode CM>
void DrawEngine<CM>::savePixels(Pixmap<CM>& buffer, coord x, coord y, coord w, coord h)
{
	// save rectangular region to buffer pixmap.
	// use restorePixels() to restore the area.
	// if x and y are not aligned to alignment requirements, x and y are adjusted accordingly,
	// and buffer pixmap should be wider and higher appropriately.
	// alignment requirements:
	//   direct color:
	//		x will be aligned to full bytes, if color size < 8 bit per pixel
	//   attribute modes:
	//		in addition, x and y will be aligned to attr cell width and height

	coord x_offs = 0;
	coord y_offs = 0;

	if constexpr (is_attribute_mode(CM))
	{
		y_offs = y % pixmap.attrheight; // align y to attr cell height
	}

	if constexpr (is_attribute_mode(CM))
	{
		// x_offs = x & coord(bitmask(3 - attrmode));	// align x to full byte
		// x_offs = x & coord(bitmask(attrwidth));		// align x to attr cell width

		constexpr int x_align_bits = max(3 - attrmode, attrwidth);
		if constexpr (x_align_bits > 0) x_offs = x & coord(bitmask(x_align_bits));
	}

	if constexpr (is_direct_color(CM) && 3 - colordepth > 0)
	{
		x_offs = x & coord(bitmask(3 - colordepth)); // align x to full byte
	}

	buffer.copyRect(x_offs, y_offs, pixmap, x, y, w, h);
}

template<ColorMode CM>
void DrawEngine<CM>::restorePixels(const Pixmap<CM>& buffer, coord x, coord y, coord w, coord h)
{
	// restore formerly saved pixel area to buffer

	coord x_offs = 0;
	coord y_offs = 0;

	if constexpr (is_attribute_mode(CM))
	{
		y_offs = y % pixmap.attrheight; // align y to attr cell height
	}

	if constexpr (is_attribute_mode(CM))
	{
		// x_offs = x & coord(bitmask(3 - attrmode));	// align x to full byte
		// x_offs = x & coord(bitmask(attrwidth));		// align x to attr cell width

		constexpr int x_align_bits = max(3 - attrmode, attrwidth);
		if constexpr (x_align_bits > 0) x_offs = x & coord(bitmask(x_align_bits));
	}

	if constexpr (is_direct_color(CM) && 3 - colordepth > 0)
	{
		x_offs = x & coord(bitmask(3 - colordepth)); // align x to full byte
	}

	pixmap.copyRect(x, y, buffer, x_offs, y_offs, w, h);
}


// instantiate them all, unless modified they are only compiled once:
// otherwise we'll need to define which to implement which leads to idiotic problems.

template class DrawEngine<colormode_i1>;
template class DrawEngine<colormode_i2>;
template class DrawEngine<colormode_i4>;
template class DrawEngine<colormode_i8>;
template class DrawEngine<colormode_rgb>;
template class DrawEngine<colormode_a1w1_i4>;
template class DrawEngine<colormode_a1w1_i8>;
template class DrawEngine<colormode_a1w1_rgb>;
template class DrawEngine<colormode_a1w2_i4>;
template class DrawEngine<colormode_a1w2_i8>;
template class DrawEngine<colormode_a1w2_rgb>;
template class DrawEngine<colormode_a1w4_i4>;
template class DrawEngine<colormode_a1w4_i8>;
template class DrawEngine<colormode_a1w4_rgb>;
template class DrawEngine<colormode_a1w8_i4>;
template class DrawEngine<colormode_a1w8_i8>;
template class DrawEngine<colormode_a1w8_rgb>;
template class DrawEngine<colormode_a2w1_i4>;
template class DrawEngine<colormode_a2w1_i8>;
template class DrawEngine<colormode_a2w1_rgb>;
template class DrawEngine<colormode_a2w2_i4>;
template class DrawEngine<colormode_a2w2_i8>;
template class DrawEngine<colormode_a2w2_rgb>;
template class DrawEngine<colormode_a2w4_i4>;
template class DrawEngine<colormode_a2w4_i8>;
template class DrawEngine<colormode_a2w4_rgb>;
template class DrawEngine<colormode_a2w8_i4>;
template class DrawEngine<colormode_a2w8_i8>;
template class DrawEngine<colormode_a2w8_rgb>;


} // namespace kipili::Graphics
