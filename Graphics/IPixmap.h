// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "geometry.h"
#include "graphics_types.h"


/* Interface class IPixmap 
   -----------------------
	
  IPixmap defines 
	virtual methods for setting and reading pixels, 
	drawing horizontal and vertical lines and rectangles,
	copying (and possibly converting) rectangular areas 
	and printing character glyphs.
	
  IPixmap provides 
	slow default implementations for most virtual methods, except:
		rgb_for_color, color_for_rgb, 
		set_color, set_pixel, get_color and get_pixel. 

  IPixmap provides 
	camelCase methods which check and limit their arguments.
	These come in two flavors:
		one which uses `coord` for it's arguments and
		one which uses `Point`, `Size`and `Rect`.

  subclasses:
    Pixmap templates are subclasses of IPixmap:
	  indexed color and true color: 
		this is opaque to IPixmap.
	  direct color and attribute modes:
		drawing methods typically take a `color` and an `ink` argument.
		  direct color:    the `color` goes into the pixels[] and argument `ink` is not needed and ignored.
		  attribute modes: the `color` goes into the attributes[] and the `ink` is stored in pixels[].
*/


namespace kio::Graphics
{


class IPixmap
{
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
	const AttrHeight attrheight; // = attrheight_none;
	char			 padding[2];

	// const ColorDepth colordepth		= get_colordepth(colormode); // 0 .. 4  log2 of bits per color in attributes[]
	// const AttrMode	 attrmode		= get_attrmode(colormode);	 // 0 .. 2  log2 of bits per color in pixmap[]
	// const AttrWidth	 attrwidth		= get_attrwidth(colormode);	 // 0 .. 3  log2 of width of tiles
	// const int		 bits_per_color = 1 << colordepth;			 // bits per color in pixmap[] or attributes[]
	// const int		 bits_per_pixel = is_attribute_mode(colormode) ? 1 << attrmode : bits_per_color; // bpp in pixmap[]

	ColorDepth colordepth() const noexcept { return get_colordepth(colormode); }
	AttrMode   attrmode() const noexcept { return get_attrmode(colormode); }
	AttrWidth  attrwidth() const noexcept { return get_attrwidth(colormode); }
	int		   bits_per_color() const noexcept { return 1 << colordepth(); }
	int bits_per_pixel() const noexcept { return is_attribute_mode(colormode) ? 1 << attrmode() : 1 << colordepth(); }

	IPixmap(ColorMode CM, AttrHeight AH, coord w, coord h) noexcept;
	IPixmap(ColorMode CM, AttrHeight AH, const Size& size) noexcept;
	virtual ~IPixmap() = default;


	// helper:
	bool is_inside(coord x, coord y) const noexcept { return uint(x) < uint(width) && (uint(y) < uint(height)); }
	bool is_inside(const Point& p) const noexcept { return is_inside(p.x, p.y); }


	/* set and get single pixels:
		In attribute modes the color is stored in attributes[] and the ink in pixels[].
		In direct color modes the color is stored in pixels[] and there are no attributes[]. 
			-> In setPixel() the ink is ignored, and getters always return data from pixels[].
	*/
	virtual void set_pixel(coord x, coord y, uint color, uint ink = 0) noexcept = 0;
	virtual uint get_pixel(coord x, coord y, uint* ink) const noexcept			= 0;
	virtual uint get_color(coord x, coord y) const noexcept						= 0;
	virtual uint get_ink(coord x, coord y) const noexcept						= 0;

	void setPixel(const Point& p, uint color, uint ink = 0) noexcept;
	uint getPixel(const Point& p, uint* ink) const noexcept;
	uint getColor(const Point& p) const noexcept;
	uint getInk(const Point& p) const noexcept;

	void setPixel(coord x, coord y, uint color, uint ink = 0) noexcept;
	uint getPixel(coord x, coord y, uint* ink) const noexcept;
	uint getColor(coord x, coord y) const noexcept;
	uint getInk(coord x, coord y) const noexcept;


	/* draw lines:
		draw horizontal line: width must be > 0.  draws nothing for width <= 0.
		draw vertical line:	  height must be > 0. draws nothing for height <= 0.
	*/
	virtual void draw_hline(coord x, coord y, coord w, uint color, uint ink) noexcept;
	virtual void draw_vline(coord x, coord y, coord h, uint color, uint ink) noexcept;

	void drawHLine(const Point& p1, coord w, uint color, uint ink = 0) noexcept;
	void drawVLine(const Point& p1, coord h, uint color, uint ink = 0) noexcept;

	void drawHLine(coord x, coord y, coord w, uint color, uint ink = 0) noexcept;
	void drawVLine(coord x, coord y, coord h, uint color, uint ink = 0) noexcept;


	/* paint rectangular area:
		fillRect(): paint rectangular area. width and height must be > 0; else nothing is drawn.
		clear():    paint entire pixmap. 
	*/
	virtual void fill_rect(coord x, coord y, coord w, coord h, uint color, uint ink) noexcept;
	virtual void xor_rect(coord x, coord y, coord w, coord h, uint color) noexcept;

	void fillRect(Rect zrect, uint color, uint ink = 0) noexcept;
	void fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 0) noexcept;
	void clear(uint color, uint ink = 0) noexcept;


	/* copy rectangular area:
		copy pixels inside a pixmap: Source and target area may overlap.
		copy pixels from other pixmap: Source and target must have same ColorMode.
	*/
	virtual void copy_rect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept;
	virtual void copy_rect(coord zx, coord zy, const IPixmap& src, coord qx, coord qy, coord w, coord h) noexcept;

	void copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(const Point& zpos, const Rect& qrect) noexcept;
	void copyRect(const Point& zpos, const Point& qpos, const Size& size) noexcept;

	void copyRect(coord zx, coord zy, const IPixmap& src) noexcept;
	void copyRect(coord zx, coord zy, const IPixmap& src, coord qx, coord qy, coord w, coord h) noexcept;
	void copyRect(const Point& zpos, const IPixmap& src) noexcept;
	void copyRect(const Point& zpos, const IPixmap& src, const Rect& qrect) noexcept;
	void copyRect(const Point& zpos, const IPixmap& src, const Point& qpos, const Size& size) noexcept;


	/* copy bitmaps:
	   drawBmp():   draw rectangular area from supplied bitmap at `bmp` with `row_offset`
	   drawChar():	variant of drawBmp() which assumes: w = 8, row_offset = 1, x%8 = 0
			  note: width and height must be positive. (draw left-to-right and top-down only)
			  note: direct color pixmaps: argument `ink` is not needed and ignored.
			  note: attribute pixmaps: the ink value in pixels[] is also set from `ink`.
	*/
	virtual void read_bmp(coord x, coord y, uint8* bmp, int row_offs, coord w, coord h, uint color, bool set) noexcept;
	virtual void draw_bmp(coord zx, coord zy, const uint8* bmp, int roffs, coord w, coord h, uint color, uint) noexcept;
	virtual void draw_char(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint ink) noexcept;

	void readBmp(coord zx, coord zy, uint8* bmp, int row_offset, coord w, coord h, uint color, bool set) noexcept;
	void drawBmp(coord zx, coord zy, const uint8* bmp, int row_offs, coord w, coord h, uint color, uint = 0) noexcept;
	void drawChar(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint ink = 0) noexcept;

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


// ###################################################################
//		Overloaded methods which call the `coord` versions:
// ###################################################################


inline void IPixmap::setPixel(const Point& p, uint color, uint ink) noexcept { setPixel(p.x, p.y, color, ink); }
inline uint IPixmap::getPixel(const Point& p, uint* ink) const noexcept { return getPixel(p.x, p.y, ink); }
inline uint IPixmap::getInk(const Point& p) const noexcept { return getInk(p.x, p.y); }
inline uint IPixmap::getColor(const Point& p) const noexcept { return getColor(p.x, p.y); }
inline void IPixmap::drawHLine(const Point& p1, coord w, uint color, uint ink) noexcept
{
	drawHLine(p1.x, p1.y, w, color, ink);
}
inline void IPixmap::drawVLine(const Point& p1, coord h, uint color, uint ink) noexcept
{
	drawVLine(p1.x, p1.y, h, color, ink);
}
inline void IPixmap::fillRect(Rect z, uint color, uint ink) noexcept
{
	fillRect(z.left(), z.top(), z.width(), z.height(), color, ink);
}
inline void IPixmap::copyRect(const Point& z, const Rect& q) noexcept
{
	copyRect(z.x, z.y, q.left(), q.top(), q.width(), q.height());
}
inline void IPixmap::copyRect(const Point& z, const Point& q, const Size& s) noexcept
{
	copyRect(z.x, z.y, q.x, q.y, s.width, s.height);
}
inline void IPixmap::copyRect(const Point& z, const IPixmap& q) noexcept { copyRect(z.x, z.y, q); }
inline void IPixmap::copyRect(const Point& z, const IPixmap& pm, const Rect& q) noexcept
{
	copyRect(z.x, z.y, pm, q.left(), q.top(), q.width(), q.height());
}
inline void IPixmap::copyRect(const Point& zpos, const IPixmap& src, const Point& qpos, const Size& size) noexcept
{
	copyRect(zpos.x, zpos.y, src, qpos.x, qpos.y, size.width, size.height);
}


inline void
IPixmap::readBmp(const Point& z, uint8* bmp, int row_offset, const Size& size, uint color, bool set) noexcept
{
	drawBmp(z.x, z.y, bmp, row_offset, size.width, size.height, color, set);
}
inline void
IPixmap::drawBmp(const Point& z, const uint8* bmp, int row_offs, const Size& size, uint color, uint ink) noexcept
{
	drawBmp(z.x, z.y, bmp, row_offs, size.width, size.height, color, ink);
}
inline void IPixmap::drawChar(const Point& z, const uint8* bmp, coord h, uint color, uint ink) noexcept
{
	drawChar(z.x, z.y, bmp, h, color, ink);
}


} // namespace kio::Graphics


/*


  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
*/
