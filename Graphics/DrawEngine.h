// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap.h"
#include "Pixmap_wAttr.h"
#include "geometry.h"
#include "graphics_types.h"
#include "kilipili_common.h"

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


class DrawEngine
{
	NO_COPY_MOVE(DrawEngine);

public:
	Canvas&		 pixmap;
	const Color* colormap; // if current color mode uses index colors

	const ColorMode	 CM = pixmap.colormode;
	const ColorDepth CD = get_colordepth(CM);
	const AttrMode	 AM = get_attrmode(CM);
	const AttrWidth	 AW = get_attrwidth(CM);

	const ColorDepth colordepth = CD; // 0 .. 4  log2 of bits per color in attributes[]
	const AttrMode	 attrmode	= AM; // 0 .. 2  log2 of bits per color in pixmap[]
	const AttrWidth	 attrwidth	= AW; // 0 .. 3  log2 of width of tiles

	const int bits_per_color = 1 << CD; // bits per color in pixmap[] (wAttr: in attributes[])
	const int bits_per_pixel = is_attribute_mode(CM) ? 1 << AM : 1 << CD; // bits per pixel in pixmap[]

	union
	{
		const Size size;
		struct
		{
			const coord width, height;
		};
	};


	DrawEngine(Canvas& pixmap, Color* colormap) noexcept : //
		pixmap(pixmap),
		colormap(colormap),
		size(pixmap.size)
	{}

	~DrawEngine() noexcept = default;


	void clearScreen(uint bgcolor, uint ink);
	void scrollScreen(coord dx, coord dy, uint bgcolor, uint ink);


	uint getPixel(coord x, coord y, uint* ink) const noexcept;
	uint getInk(coord x, coord y) const noexcept;
	uint getColor(coord x, coord y) const noexcept;

	void setPixel(coord x, coord y, uint color, uint ink = 1) noexcept;
	void drawHLine(coord x, coord y, coord x2, uint color, uint ink = 1) noexcept;
	void drawVLine(coord x, coord y, coord y2, uint color, uint ink = 1) noexcept;
	void drawLine(coord x, coord y, coord x2, coord y2, uint color, uint ink = 1) noexcept;
	void drawRect(coord x, coord y, coord x2, coord y2, uint color, uint ink = 1) noexcept;
	void fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 1) noexcept;
	void drawCircle(coord x, coord y, coord x2, coord y2, uint color, uint ink = 1) noexcept;
	void fillCircle(coord x, coord y, coord x2, coord y2, uint color, uint ink = 1) noexcept;
	void floodFill(coord x, coord y, uint color, uint ink = 1);

	void copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h);
	void copyRect(coord zx, coord zy, const Canvas& q) { pixmap.copyRect(zx, zy, q); }
	void copyRect(coord zx, coord zy, const Canvas& q, coord qx, coord qy, coord w, coord h);

	void savePixels(Canvas& buffer, coord x, coord y, coord w, coord h); // same CM
	void restorePixels(const Canvas& buffer, coord x, coord y, coord w, coord h);


	uint getInk(const Point& p) const noexcept { return pixmap.getInk(p); }
	uint getPixel(const Point& p, uint* ink) const noexcept { return pixmap.getPixel(p, ink); }
	uint getColor(const Point& p) const noexcept { return pixmap.getColor(p); }

	void setPixel(const Point& p, uint color, uint ink = 1) noexcept;
	void drawHLine(const Point& p, coord x2, uint color, uint ink = 1) noexcept;
	void drawVLine(const Point& p, coord y2, uint color, uint ink = 1) noexcept;
	void drawLine(const Point& p1, const Point& p2, uint color, uint ink = 1) noexcept;
	void drawRect(const Rect& r, uint color, uint ink = 1) noexcept;
	void fillRect(const Rect& r, uint color, uint ink = 1) noexcept;
	void drawCircle(const Rect&, uint color, uint ink = 1) noexcept;
	void fillCircle(const Rect&, uint color, uint ink = 1) noexcept;
	void floodFill(const Point& p, uint color, uint ink = 1);

	void drawPolygon(const Point*, uint cnt, uint color, uint ink = 1);
	void fillPolygon(const Point*, uint cnt, uint color, uint ink = 1);

	void copyRect(const Point& z, const Point& q, const Size& size);
	void copyRect(const Rect& z, const Point& q) { copyRect(z.p1, q, z.size()); }
	void copyRect(const Point& z, const Rect& q) { copyRect(z, q.p1, q.size()); }
	void copyRect(const Point& z, const Canvas& src) { pixmap.copyRect(z, src); }
	void copyRect(const Point& z, const Canvas& src, const Rect& q) { pixmap.copyRect(z, src, q); }
	void copyRect(const Rect& z, const Canvas& src, const Point& q) { pixmap.copyRect(z.p1, src, q, z.size()); }


	void readBmpFromScreen(coord x, coord y, coord w, coord h, uint8[], uint color, bool set);
	void writeBmpToScreen(coord x, coord y, coord w, coord h, const uint8[], uint fgcolor, uint bgcolor);

	//void savePixels(Canvas& buffer, const Point& p, const Size& s); // same CM
	//void savePixels(Canvas& buffer, const Rect& r) { savePixels(buffer, r.left(), r.top(), r.width(), r.height()); }
	void restorePixels(const Canvas& buffer, const Point& p, const Size& s);
	void restorePixels(const Canvas& buffer, const Rect& r);


	// helper:
	void draw_hline(coord x, coord y, coord x2, uint color, uint ink) noexcept;
	void draw_vline(coord x, coord y, coord y2, uint color, uint ink) noexcept;
	bool in_screen(coord x, coord y) { return pixmap.is_inside(x, y); }

private:
	int adjust_l(coord l, coord r, coord y, uint ink); // -> floodFill()
	int adjust_r(coord l, coord r, coord y, uint ink); // -> floodFill()
};


//
//
//
//
// #######################################################################
//							Implementations
// #######################################################################


inline uint DrawEngine::getPixel(coord x, coord y, uint* ink) const noexcept { return pixmap.getPixel(x, y, ink); }
inline uint DrawEngine::getInk(coord x, coord y) const noexcept { return pixmap.getInk(x, y); }
inline uint DrawEngine::getColor(coord x, coord y) const noexcept { return pixmap.getColor(x, y); }
inline void DrawEngine::setPixel(coord x, coord y, uint color, uint ink) noexcept { pixmap.setPixel(x, y, color, ink); }
inline void DrawEngine::setPixel(const Point& p, uint color, uint ink) noexcept { pixmap.setPixel(p, color, ink); }

inline void DrawEngine::drawHLine(coord x, coord y, coord x2, uint color, uint ink) noexcept
{
	pixmap.drawHLine(x, y, x2, color, ink);
}

inline void DrawEngine::drawVLine(coord x, coord y, coord y2, uint color, uint ink) noexcept
{
	pixmap.drawVLine(x, y, y2, color, ink);
}
inline void DrawEngine::drawLine(const Point& p1, const Point& p2, uint color, uint ink) noexcept
{
	drawLine(p1.x, p1.y, p2.x, p2.y, color, ink);
}
inline void DrawEngine::draw_hline(coord x, coord y, coord x2, uint color, uint ink) noexcept
{
	pixmap.draw_hline(x, y, x2, color, ink);
}
inline void DrawEngine::draw_vline(coord x, coord y, coord y2, uint color, uint ink) noexcept
{
	pixmap.draw_vline(x, y, y2, color, ink);
}

inline void DrawEngine::copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h)
{
	pixmap.copyRect(zx, zy, pixmap, qx, qy, w, h);
}
inline void DrawEngine::copyRect(const Point& z, const Point& q, const Size& size)
{
	copyRect(z.x, z.y, q.x, q.y, size.width, size.height);
}
inline void DrawEngine::copyRect(coord zx, coord zy, const Canvas& q, coord qx, coord qy, coord w, coord h)
{
	pixmap.copyRect(zx, zy, q, qx, qy, w, h);
}
//inline void DrawEngine::savePixels(Canvas& buffer, const Point& p, const Size& s)
//{
//	savePixels(buffer, p.x, p.y, s.width, s.height);
//}
inline void DrawEngine::restorePixels(const Canvas& buffer, const Point& p, const Size& s)
{
	restorePixels(buffer, p.x, p.y, s.width, s.height);
}
inline void DrawEngine::restorePixels(const Canvas& buffer, const Rect& r)
{
	restorePixels(buffer, r.left(), r.top(), r.width(), r.height());
}


} // namespace kio::Graphics


/*







































*/
