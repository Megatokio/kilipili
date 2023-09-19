// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap.h"
#include "Pixmap_wAttr.h"
#include "geometry.h"
#include "graphics_types.h"
#include "kipili_common.h"

/*
	Basic 2D Graphics

	Graphics Model:

		Y-axis is top down. (unless we display the scanlines in reverse order)

		Screen pixels are assumed to lie not on but *between* coordinates.
		Thus the top left pixel of the screen lies between (0,0) and (1,1).
		When drawing 1 pixel wide lines, then the pixels hang in the x++,y++ direction.
		When drawing the outline of a rectangle with coordinates (0,0),(10,10) as a polyline
			then the outer dimensions of the frame is 11 x 11 pixels in size.
		When filling a rectangle with coordinates (0,0),(10,10)
			then the solid rectangle is 10 x 10 pixels in size.
*/

namespace kio::Graphics
{

static constexpr uint DONT_CLEAR = ~0u;

template<ColorMode CM>
class DrawEngine
{
public:
	Pixmap<CM>&	 pixmap;
	const Color* colormap; // if current color mode uses index colors

	static constexpr ColorDepth CD			   = get_colordepth(CM); // 0 .. 4  log2 of bits per color in attributes[]
	static constexpr AttrMode	AM			   = get_attrmode(CM);	 // 0 .. 2  log2 of bits per color in pixmap[]
	static constexpr AttrWidth	AW			   = get_attrwidth(CM);	 // 0 .. 3  log2 of width of tiles
	static constexpr int		bits_per_color = 1 << CD; // bits per color in pixmap[] (wAttr: in attributes[])
	static constexpr int bits_per_pixel = is_attribute_mode(CM) ? 1 << AM : 1 << CD; // bits per pixel in pixmap[]

	static constexpr ColorDepth colordepth = get_colordepth(CM); // 0 .. 4  log2 of bits per color in attributes[]
	static constexpr AttrMode	attrmode   = get_attrmode(CM);	 // 0 .. 2  log2 of bits per color in pixmap[]
	static constexpr AttrWidth	attrwidth  = get_attrwidth(CM);	 // 0 .. 3  log2 of width of tiles

	int			width() const noexcept { return pixmap.width; }
	int			height() const noexcept { return pixmap.height; }
	const Size& size() const noexcept { return pixmap.size; }


	DrawEngine(Pixmap<CM>& pixmap, Color* colormap) noexcept : pixmap(pixmap), colormap(colormap) {}

	void clearScreen(uint bgcolor);
	void scrollScreen(coord dx, coord dy, uint bgcolor);

	uint getPixel(coord x, coord y, uint* ink) const noexcept // if attr mode then return the pixels[]
	{
		return pixmap.getPixel(x, y, ink);
	}
	uint getInk(coord x, coord y) const noexcept // return value from pixels[]
	{
		return pixmap.getInk(x, y);
	}
	uint getColor(coord x, coord y) const noexcept // if attr mode then return the color
	{
		return pixmap.getColor(x, y);
	}
	uint getInk(const Point& p) { return pixmap.getInk(p); }
	uint getPixel(const Point& p, uint* ink) { return pixmap.getPixel(p, ink); }
	uint getColor(const Point& p) { return pixmap.getColor(p); }

	void setPixel(coord x, coord y, uint color, uint ink = 1) noexcept { pixmap.setPixel(x, y, color, ink); }
	void setPixel(const Point& p, uint color, uint ink = 1) noexcept { pixmap.setPixel(p, color, ink); }

	void drawHorLine(coord x, coord y, coord x2, uint color, uint ink = 1) noexcept
	{
		pixmap.drawHLine(x, y, x2, color, ink);
	}
	void drawHorLine(const Point& p, coord x2, uint color, uint ink = 1) noexcept
	{
		pixmap.drawHLine(p, x2, color, ink);
	}

	void drawVertLine(coord x, coord y, coord y2, uint color, uint ink = 1) noexcept
	{
		pixmap.drawVLine(x, y, y2, color, ink);
	}
	void drawVertLine(const Point& p, coord y2, uint color, uint ink = 1) noexcept
	{
		pixmap.drawVLine(p, y2, color, ink);
	}

	void drawLine(coord x, coord y, coord x2, coord y2, uint color, uint ink = 1);
	void drawLine(const Point& p1, const Point& p2, uint color, uint ink = 1)
	{
		drawLine(p1.x, p1.y, p2.x, p2.y, color, ink);
	}

	void drawRect(
		coord x, coord y, coord x2, coord y2, uint color, uint ink = 1); // outline drawn is inset for rect and circle
	void drawRect(const Rect& r, uint color, uint ink = 1) { drawRect(r.p1.x, r.p1.y, r.p2.x, r.p2.y, color, ink); }

	void fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 1) noexcept
	{
		pixmap.fillRect(x, y, w, h, color, ink);
	}
	void fillRect(const Rect& r, uint color, uint ink = 1) noexcept { pixmap.fillRect(r, color, ink); }

	void drawCircle(
		coord x, coord y, coord x2, coord y2, uint color, uint ink = 1); // outline drawn is inset for rect and circle
	void drawCircle(const Rect&, uint color, uint ink = 1);

	void fillCircle(coord x, coord y, coord x2, coord y2, uint color, uint ink = 1);
	void fillCircle(const Rect&, uint color, uint ink = 1);

	void drawPolygon(const Point*, uint cnt, uint color, uint ink = 1);
	void fillPolygon(const Point*, uint cnt, uint color, uint ink = 1);

	void fill(coord x, coord y, uint color, uint ink = 1);
	void fill(const Point& p, uint color, uint ink = 1) { fill(p.x, p.y, color, ink); }

	void copyRect(coord zx, coord zy, const Pixmap<CM>& q) { pixmap.copyRect(zx, zy, q); }
	void copyRect(coord zx, coord zy, const Pixmap<CM>& q, coord qx, coord qy, coord w, coord h)
	{
		pixmap.copyRect(zx, zy, q, qx, qy, w, h);
	}
	void copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h)
	{
		pixmap.copyRect(zx, zy, pixmap, qx, qy, w, h);
	}

	void copyRect(const Point& z, const Point& q, const Size& sz)
	{
		pixmap.copyRect(z.x, z.y, pixmap, q.x, q.y, sz.width, sz.height);
	}
	void copyRect(const Rect& q, const Point& z) { copyRect(q.p1, z, q.size()); }
	void copyRect(const Point& q, const Rect& z) { copyRect(q, z.p1, z.size()); }

	void readBmpFromScreen(coord x, coord y, coord w, coord h, uint8[], uint bgcolor);
	void writeBmpToScreen(coord x, coord y, coord w, coord h, const uint8[], uint fgcolor, uint bgcolor);

	void savePixels(Pixmap<CM>& buffer, coord x, coord y, coord w, coord h);
	void savePixels(Pixmap<CM>& buffer, const Point& p, const Size& s)
	{
		savePixels(buffer, p.x, p.y, s.width, s.height);
	}
	void savePixels(Pixmap<CM>& buffer, const Rect& r) { savePixels(buffer, r.left(), r.top(), r.width(), r.height()); }
	void restorePixels(const Pixmap<CM>& buffer, coord x, coord y, coord w, coord h);
	void restorePixels(const Pixmap<CM>& buffer, const Point& p, const Size& s)
	{
		restorePixels(buffer, p.x, p.y, s.width, s.height);
	}
	void restorePixels(const Pixmap<CM>& buffer, const Rect& r)
	{
		restorePixels(buffer, r.left(), r.top(), r.width(), r.height());
	}

	// helper:
	static uint32 flood_filled_color(uint color) noexcept { return Graphics::flood_filled_color<colordepth>(color); }
	void		  draw_hor_line(coord x, coord y, coord x2, uint color, uint ink) noexcept
	{
		pixmap.draw_hline(x, y, x2, color, ink);
	}
	void draw_vert_line(coord x, coord y, coord y2, uint color, uint ink) noexcept
	{
		pixmap.draw_vline(x, y, y2, color, ink);
	}
	bool in_screen(coord x, coord y) { return pixmap.is_inside(x, y); }

private:
	int adjust_l(coord l, coord r, coord y, uint ink); // -> fill()
	int adjust_r(coord l, coord r, coord y, uint ink); // -> fill()
};


extern template class DrawEngine<colormode_i1>;
extern template class DrawEngine<colormode_i2>;
extern template class DrawEngine<colormode_i4>;
extern template class DrawEngine<colormode_i8>;
extern template class DrawEngine<colormode_rgb>;
extern template class DrawEngine<colormode_a1w1_i4>;
extern template class DrawEngine<colormode_a1w1_i8>;
extern template class DrawEngine<colormode_a1w1_rgb>;
extern template class DrawEngine<colormode_a1w2_i4>;
extern template class DrawEngine<colormode_a1w2_i8>;
extern template class DrawEngine<colormode_a1w2_rgb>;
extern template class DrawEngine<colormode_a1w4_i4>;
extern template class DrawEngine<colormode_a1w4_i8>;
extern template class DrawEngine<colormode_a1w4_rgb>;
extern template class DrawEngine<colormode_a1w8_i4>;
extern template class DrawEngine<colormode_a1w8_i8>;
extern template class DrawEngine<colormode_a1w8_rgb>;
extern template class DrawEngine<colormode_a2w1_i4>;
extern template class DrawEngine<colormode_a2w1_i8>;
extern template class DrawEngine<colormode_a2w1_rgb>;
extern template class DrawEngine<colormode_a2w2_i4>;
extern template class DrawEngine<colormode_a2w2_i8>;
extern template class DrawEngine<colormode_a2w2_rgb>;
extern template class DrawEngine<colormode_a2w4_i4>;
extern template class DrawEngine<colormode_a2w4_i8>;
extern template class DrawEngine<colormode_a2w4_rgb>;
extern template class DrawEngine<colormode_a2w8_i4>;
extern template class DrawEngine<colormode_a2w8_i8>;
extern template class DrawEngine<colormode_a2w8_rgb>;


} // namespace kio::Graphics
