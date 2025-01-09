// Copyright (c) 2007 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Colormap.h"
#include "geometry.h"
#include "standard_types.h"


namespace kio::Graphics
{

class Pixelmap;
class Colormap;

class Pixelmap
{
public:
	Pixelmap() : box(), dy(0), data(nullptr), pixels(nullptr) {}
	Pixelmap(const Rect& bbox) :
		box(bbox),
		dy(width()),
		data(new uchar[width() * height()]),
		pixels(data - x1() - y1() * dy)
	{}
	Pixelmap(int w, int h) : box(w, h), dy(w), data(new uchar[w * h]), pixels(data) {}
	Pixelmap(int x, int y, int w, int h) : box(x, y, w, h), dy(w), data(new uchar[w * h]), pixels(data - x - y * w) {}

	Pixelmap(uptr px, int x, int y, int w, int h, int z = 0) :
		box(x, y, w, h),
		dy(z ? z : w),
		data(px),
		pixels(px - x - y * dy)
	{}
	Pixelmap(uptr px, int w, int h, int z = 0) : box(w, h), dy(z ? z : w), data(px), pixels(px) {}

	Pixelmap(uptr px, const Rect& bx, int z = 0) : box(bx), dy(z ? z : width()), data(px), pixels(px - x1() - y1() * dy)
	{}
	Pixelmap(uptr px, const Point& p1, const Point& p2, int z = 0) :
		box(p1, p2),
		dy(z ? z : width()),
		data(px),
		pixels(px - p1.x - p1.y * dy)
	{}
	Pixelmap(uptr px, const Point& p1, const Dist& sz, int z = 0) :
		box(p1, sz),
		dy(z ? z : width()),
		data(px),
		pixels(px - p1.x - p1.y * dy)
	{}
	Pixelmap(uptr px, const Size& sz, int z = 0) : box(sz), dy(z ? z : width()), data(px), pixels(px) {}

	~Pixelmap() { kill(); }
	Pixelmap(const Pixelmap& q) { init(q); }
	Pixelmap& operator=(const Pixelmap& q)
	{
		if (this != &q)
		{
			kill();
			init(q);
		}
		return *this;
	}

	const Rect& getBox() const { return box; } // was: Frame()
	int			x1() const { return box.left(); }
	int			y1() const { return box.top(); }
	int			x2() const { return box.right(); }
	int			y2() const { return box.bottom(); }
	int			width() const { return box.width(); }
	int			height() const { return box.height(); }
	int			rowOffset() const { return dy; } // was: DY()
	//TODO const Point& p1() const { return box.top_left(); }
	Point p1() const { return box.top_left(); }
	Dist  getSize() const { return box.size(); }

	bool isEmpty() const { return box.is_empty(); }
	bool isNotEmpty() const { return !box.is_empty(); }

	// Reposition frame:
	// This affects which pixels are enclosed.
	// Pixel coordinates are not affected!
	void setFrame(const Rect& b) { box = b; }
	void setFrame(int x, int y, int w, int h) { box = Rect(x, y, w, h); }
	void setFrame(const Point& p1, const Point& p2) { box = Rect(p1, p2); }

	void set_data_ptr(uptr p) { data = p; }	   // use with care...
	void set_pixel_ptr(uptr p) { pixels = p; } // caveat: check data, p1 and p2!
	void set_dy(int z) { dy = z; }			   // caveat: check pixels, p1 and p2!

	// ptr -> pixel[x1,y1]:
	uptr  getPixels() { return getPixelPtr(p1()); }
	cuptr getPixels() const { return getPixelPtr(p1()); }

	// ptr -> first pixel of row:
	uptr  getPixelRow(int y) { return getPixelPtr(x1(), y); }
	cuptr getPixelRow(int y) const { return getPixelPtr(x1(), y); }

	// ptr -> pixel:
	uptr  getPixelPtr(int x, int y) { return pixels + x + y * dy; }
	cuptr getPixelPtr(int x, int y) const { return pixels + x + y * dy; }

	uptr  getPixelPtr(const Point& p) { return pixels + p.x + p.y * dy; }
	cuptr getPixelPtr(const Point& p) const { return pixels + p.x + p.y * dy; }

	uchar& getPixel(int x, int y) { return pixels[x + y * dy]; }
	uchar  getPixel(int x, int y) const { return pixels[x + y * dy]; }

	uptr  getData() { return data; }
	cuptr getData() const { return data; }


	// Member Functions:

	// Change the pixel coordinate system.
	// Thereafter all pixel coordinates, including the frame rect coordinates are shifted!
	// This does not reposition the frame rect: it does not affect which pixels are enclosed.
	//TODO void offsetAddresses(int dx, int dy) { offsetAddresses(Dist(dx, dy)); }
	//TODO void offsetAddresses(const Dist& d)
	//TODO {
	//TODO 	box.move(d);
	//TODO 	pixels -= d.dx + d.dy * dy;
	//TODO }
	//TODO void setAddressForP1(const Point& p) { offsetAddresses(p - box.p1); }

	// Color tools:
	int	 getMaxColorIndex() const;
	int	 countUsedColors(int* max_color_index = nullptr) const;
	void reduceColors(Colormap& cmap);

	// Special resizing:
	void setToDiff(const Pixelmap& new_pixmap, int transp_index = Colormap::unset) __deprecated;
	void reduceToDiff(const Pixelmap& old_pixmap, int transp_index = Colormap::unset);
	//TODO void shrinkToRect(const Rect& new_box) { box.intersectWith(new_box); }
	void cropBackground(int bgcolor);

	void clear(int color);

protected:
	void kill()
	{
		delete[] data;
		data = nullptr;
	}
	void init(const Pixelmap& q) noexcept(false); // bad_alloc

	Rect box;	 // frame: x,y,w,h
	int	 dy;	 // address offset per row
	uptr data;	 // allocated memory for delete[]
	uptr pixels; // -> pixel[0,0]
};

} // namespace kio::Graphics

/*



































*/
