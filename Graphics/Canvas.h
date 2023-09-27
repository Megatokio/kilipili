// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "geometry.h"
#include "graphics_types.h"
#include "no_copy_move.h"


/* Interface class Canvas 
   ----------------------
	
  Canvas defines virtual methods 
	for setting and reading pixels, 
	drawing horizontal and vertical lines and rectangles,
	copying rectangular areas 
	and printing bitmaps and character glyphs.
	
  Canvas provides slow default implementations for most virtual methods, 
	except set_pixel, get_pixel, get_color and get_ink. 

  All camelCase methods clip their arguments to the canvas area.
  All snake_case methods assume fully clipped coordinates.
  All methods do nothing if width or height is zero or negative. 
*/


namespace kio::Graphics
{


class Canvas
{
	NO_COPY_MOVE(Canvas);

protected:
	Canvas(coord w, coord h, ColorMode CM, AttrHeight AH, bool allocated) noexcept;

public:
	union
	{
		const Size size;
		struct
		{
			const coord width, height;
		};
	};

	const ColorMode	 colormode;
	const AttrHeight attrheight;
	const bool		 allocated; // whether pixmap[] was allocated and will be deleted in dtor
	char			 padding[1];

	ColorDepth colordepth() const noexcept { return get_colordepth(colormode); } // log2 of bits per color in attr[]
	AttrMode   attrmode() const noexcept { return get_attrmode(colormode); }	 // log2 of bits per color in pixmap[]
	AttrWidth  attrwidth() const noexcept { return get_attrwidth(colormode); }	 // log2 of width of attribute cells
	int		   bits_per_color() const noexcept { return 1 << colordepth(); }	 // bits per color in pixmap[] or attr[]
	int bits_per_pixel() const noexcept { return is_attribute_mode(colormode) ? 1 << attrmode() : 1 << colordepth(); }

	virtual ~Canvas() = default;


	// helper:
	bool is_inside(coord x, coord y) const noexcept { return uint(x) < uint(width) && (uint(y) < uint(height)); }
	bool is_inside(const Point& p) const noexcept { return is_inside(p.x, p.y); }


	/* _______________________________________________________________________________________
	   set and get single pixels:
		In attribute modes the color is stored in attributes[] and the ink in pixels[].
		In direct color modes the color is stored in pixels[] and there are no attributes[]. 
			-> set_pixel() ignores `ink`, and getters always return data from pixels[].
	*/
	virtual void set_pixel(coord x, coord y, uint color, uint ink = 0) noexcept = 0;
	virtual uint get_pixel(coord x, coord y, uint* ink) const noexcept			= 0;
	virtual uint get_color(coord x, coord y) const noexcept						= 0;
	virtual uint get_ink(coord x, coord y) const noexcept						= 0;

	/* _______________________________________________________________________________________
	   draw lines:
		- draw nothing if width or height is <= 0.
		- draw_hline(): no bounds test!
		- drawHLine():  draw horizontal line
		- drawVLine():  draw vertical line
	*/
	virtual void draw_hline(coord x, coord y, coord w, uint color, uint ink) noexcept;
	virtual void drawHLine(coord x, coord y, coord w, uint color, uint ink = 0) noexcept;
	virtual void drawVLine(coord x, coord y, coord h, uint color, uint ink = 0) noexcept;

	/* _______________________________________________________________________________________
	   paint rectangular area:
		- fillRect(): paint rectangular area
		- xorRect():  xor all colors in the rect area with the xorColor
		              Pixmap_wAttr: xor colors of all inks in the attributes[]
		- clear():    entire pixmap 
	*/
	virtual void fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 0) noexcept;
	virtual void xorRect(coord x, coord y, coord w, coord h, uint color) noexcept;
	void		 clear(uint color, uint ink = 0) noexcept;

	/* _______________________________________________________________________________________
	   copy rectangular area:
		- copy pixels inside a pixmap. Source and target area may overlap
		- copy pixels from other pixmap. Source and target must have same ColorMode
	*/
	virtual void copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept;
	virtual void copyRect(coord zx, coord zy, const Canvas& src, coord qx, coord qy, coord w, coord h) noexcept;

	/* _______________________________________________________________________________________
	   copy bitmaps:
	   - drawBmp():  draw rectangular area from bitmap: set `1` bits with color, skip `0` bits
	   - readBmp():  read rect area into bitmap. 
						set=0: `color` is background => clear bit in bmp for pixel == color
						set=1: `color` is foreground => set bit in bmp for pixel == color
	   - drawChar(): variant of drawBmp() which assumes: w = 8, row_offset = 1, x%8 = 0
	*/
	virtual void readBmp(coord x, coord y, uint8* bmp, int row_offs, coord w, coord h, uint color, bool set) noexcept;
	virtual void drawBmp(coord zx, coord zy, const uint8* bmp, int ro, coord w, coord h, uint color, uint = 0) noexcept;
	virtual void drawChar(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint = 0) noexcept;

	/* _______________________________________________________________________________________
	   more drawing primitives:
	*/
	void drawLine(coord x1, coord y1, coord x2, coord y2, uint color, uint ink = 1) noexcept;
	void drawRect(coord x, coord y, coord x2, coord y2, uint color, uint ink = 1) noexcept;


	// ########################
	//		Variants:
	// ########################

	void setPixel(coord x, coord y, uint color, uint ink = 0) noexcept;
	uint getPixel(coord x, coord y, uint* ink) const noexcept;
	uint getColor(coord x, coord y) const noexcept;
	uint getInk(coord x, coord y) const noexcept;

	void setPixel(const Point& p, uint color, uint ink = 0) noexcept;
	uint getPixel(const Point& p, uint* ink) const noexcept;
	uint getColor(const Point& p) const noexcept;
	uint getInk(const Point& p) const noexcept;

	void drawHLine(const Point& p1, coord w, uint color, uint ink = 0) noexcept;
	void drawVLine(const Point& p1, coord h, uint color, uint ink = 0) noexcept;
	void fillRect(Rect zrect, uint color, uint ink = 0) noexcept;

	void copyRect(const Point& zpos, const Rect& qrect) noexcept;
	void copyRect(const Point& zpos, const Point& qpos, const Size& size) noexcept;
	void copyRect(coord zx, coord zy, const Canvas& src) noexcept;
	void copyRect(const Point& zpos, const Canvas& src) noexcept;
	void copyRect(const Point& zpos, const Canvas& src, const Rect& qrect) noexcept;
	void copyRect(const Point& zpos, const Canvas& src, const Point& qpos, const Size& size) noexcept;

	void readBmp(const Point& zpos, uint8* bmp, int row_offset, const Size& size, uint color, bool set) noexcept;
	void drawBmp(const Point& zpos, const uint8* bmp, int row_offset, const Size& size, uint color, uint = 0) noexcept;
	void drawChar(const Point& z, const uint8* bmp, coord h, uint color, uint ink = 0) noexcept;

private:
	void read_hline_bmp(coord x, coord y, coord w, uint8* z, uint color, bool set) noexcept;
	void draw_hline_bmp(coord x, coord y, coord w, const uint8*, uint color, uint ink) noexcept;
};


// ###################################################################################
//								Implementations
// ###################################################################################


inline void Canvas::setPixel(coord x, coord y, uint color, uint ink) noexcept
{
	if (is_inside(x, y)) set_pixel(x, y, color, ink);
}
inline void Canvas::setPixel(const Point& p, uint color, uint ink) noexcept { setPixel(p.x, p.y, color, ink); }

inline uint Canvas::getPixel(coord x, coord y, uint* ink) const noexcept
{
	return is_inside(x, y) ? get_pixel(x, y, ink) : (*ink = 0);
}
inline uint Canvas::getPixel(const Point& p, uint* ink) const noexcept { return getPixel(p.x, p.y, ink); }

inline uint Canvas::getInk(coord x, coord y) const noexcept { return is_inside(x, y) ? get_ink(x, y) : 0; }
inline uint Canvas::getInk(const Point& p) const noexcept { return getInk(p.x, p.y); }

inline uint Canvas::getColor(coord x, coord y) const noexcept { return is_inside(x, y) ? get_color(x, y) : 0; }
inline uint Canvas::getColor(const Point& p) const noexcept { return getColor(p.x, p.y); }

inline void Canvas::drawHLine(const Point& p1, coord w, uint color, uint ink) noexcept
{
	drawHLine(p1.x, p1.y, w, color, ink);
}
inline void Canvas::drawVLine(const Point& p1, coord h, uint color, uint ink) noexcept
{
	drawVLine(p1.x, p1.y, h, color, ink);
}
inline void Canvas::fillRect(Rect z, uint color, uint ink) noexcept
{
	fillRect(z.left(), z.top(), z.width(), z.height(), color, ink);
}
inline void Canvas::clear(uint color, uint ink) noexcept { fillRect(0, 0, width, height, color, ink); }

inline void Canvas::copyRect(coord zx, coord zy, const Canvas& q) noexcept
{
	copyRect(zx, zy, q, 0, 0, q.width, q.height);
}
inline void Canvas::copyRect(const Point& z, const Rect& q) noexcept
{
	copyRect(z.x, z.y, q.left(), q.top(), q.width(), q.height());
}
inline void Canvas::copyRect(const Point& z, const Point& q, const Size& s) noexcept
{
	copyRect(z.x, z.y, q.x, q.y, s.width, s.height);
}
inline void Canvas::copyRect(const Point& z, const Canvas& q) noexcept { copyRect(z.x, z.y, q); }
inline void Canvas::copyRect(const Point& z, const Canvas& pm, const Rect& q) noexcept
{
	copyRect(z.x, z.y, pm, q.left(), q.top(), q.width(), q.height());
}
inline void Canvas::copyRect(const Point& zpos, const Canvas& src, const Point& qpos, const Size& size) noexcept
{
	copyRect(zpos.x, zpos.y, src, qpos.x, qpos.y, size.width, size.height);
}

inline void Canvas::readBmp(const Point& z, uint8* bmp, int row_offset, const Size& size, uint color, bool set) noexcept
{
	drawBmp(z.x, z.y, bmp, row_offset, size.width, size.height, color, set);
}
inline void Canvas::drawBmp(const Point& z, const uint8* bmp, int roff, const Size& size, uint color, uint ink) noexcept
{
	drawBmp(z.x, z.y, bmp, roff, size.width, size.height, color, ink);
}
inline void Canvas::drawChar(const Point& z, const uint8* bmp, coord h, uint color, uint ink) noexcept
{
	drawChar(z.x, z.y, bmp, h, color, ink);
}


} // namespace kio::Graphics


/*


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
*/
