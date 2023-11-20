// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Pixmap_wAttr.h"
#include "RCPtr.h"
#include "VideoBackend.h"
#include "atomic.h"
#include "geometry.h"
#include "kilipili_cdefs.h"

namespace kio::Video
{

// A Shape is merely a string of true color pixels
// of which some are interpreted as commands to define how they are placed.

// Softening is done by down scaling the sprite horizontally 2:1.
// => there is no difference in the stored data.
// displaying a softened shape as a normal one will display it in double width.
// displaying a normal shape as a softened one will display it in half width.
// the constructors are different thou.

// Animated Shapes are just the frames stringed together with some added data to the FramePrefix.

/*
	layout of one row:
	
		{dx,width} pixels[width]  N*{ cmd:gap {dx,width} pixels[width] } 

	layout of a static frame:

		{width,height,hot_x,hot_y}  row  { row }*N cmd:end  [alignment byte]
		
	layout of an animated shape:
	
		{ {next_frame.w,duration,magic}  frame }*N
*/

// frame = sequence of rows.
// each frame starts with a preamble to define the total size and the hot spot for this frame.
// Each row starts with a HDR and then that number of colors follow.
// After that there is the HDR of the next row.
// If the next HDR is a CMD, then handle this CMD as part of the current line:
// 	 END:  shape is finished, remove it from hotlist.
// 	 SKIP: resume one more HDR at the current position: used to insert transparent space.


enum Softening : bool { NotSoftened = 0, Softened = 1 };
enum Animation : bool { NotAnimated = 0, Animated = 1 };
enum ZPlane : bool { NoZ = 0, HasZ = 1 };


/*	——————————————————————————————————————————————————————————————————————————
	reference-counted array of Colors
*/
class Colors
{
private:
public:
	mutable uint16 rc;
	Color		   colors[8888];

	static Colors* newColors(uint cnt) throws
	{
		Colors* z = reinterpret_cast<Colors*>(new char[sizeof(rc) + cnt * sizeof(Color)]);
		z->rc	  = 0;
		return z;
	}

	uint16 refcnt() const noexcept { return rc; }

	Color&		 operator[](uint i) noexcept { return colors[i]; }
	const Color& operator[](uint i) const noexcept { return colors[i]; }

private:
	NO_COPY_MOVE(Colors);
	Colors() noexcept  = delete;
	~Colors() noexcept = default;

	friend class kio::RCPtr<Colors>;

	void retain() const noexcept { pp_atomic(rc); }
	void release() const noexcept
	{
		if (mm_atomic(rc) == 0) delete[] ptr(this);
	}
};


/*	——————————————————————————————————————————————————————————————————————————
	A Shape defines the shape of a sprite.
	It provides functions to render the shape.
*/
template<Softening SOFT>
struct Shape
{
	using ColorMode = Graphics::ColorMode;
	template<ColorMode CM>
	using Pixmap = Graphics::Pixmap<CM>;

	static constexpr bool animated = false;
	static constexpr bool softened = SOFT == Softened;

	RCPtr<Colors> pixels;
	uint8		  _width  = 0;
	uint8		  _height = 0;
	int8		  _hot_x  = 0;
	int8		  _hot_y  = 0;

	uint8 width() const noexcept { return _width; }
	uint8 height() const noexcept { return _height; }
	int8  hot_x() const noexcept { return _hot_x; }
	int8  hot_y() const noexcept { return _hot_y; }

	//Size size() const noexcept { return Size(width(), height()); }
	Dist hotspot() const noexcept { return Dist(hot_x(), hot_y()); }

	template<Graphics::ColorMode CM>
	Shape(const Graphics::Pixmap<CM>& pm, uint transp, const Color* clut, int8 hot_x, int8 hot_y) throws;
	Shape() noexcept {}
};


template<typename Shape, ZPlane WZ>
struct HotShape
{
	const Color* pixels;
	int			 x;
	bool		 ghostly;
	char		 _padding[3];

	bool is_cmd() const noexcept { return pfx().dx == -128; }
	bool is_pfx() const noexcept { return pfx().dx != -128; }
	bool is_end() const noexcept { return cmd() == END; }
	bool is_skip() const noexcept { return cmd() == GAP; }

	int8  dx() const noexcept { return pfx().dx; }
	uint8 width() const noexcept { return pfx().width; }
	void  skip_cmd() noexcept { pixels += sizeof(CMD) / sizeof(*pixels); }
	void  skip_pfx() noexcept { pixels += sizeof(PFX) / sizeof(*pixels); }

	void skip_row() noexcept;
	bool render_row(Color* scanline) noexcept;


	struct PFX // raw pixels prefix
	{
		int8  dx;	 // initial offset
		uint8 width; // count of pixels that follow
	};

	enum CMD : uint16 { END = 0x0080, GAP = 0x0180 }; // little endian

	const PFX& pfx() const noexcept { return *reinterpret_cast<const PFX*>(pixels); }
	CMD		   cmd() const noexcept
	{
		if constexpr (sizeof(Color) >= sizeof(CMD)) return *reinterpret_cast<const CMD*>(pixels);
		return CMD(reinterpret_cast<const uchar*>(pixels)[0] + (reinterpret_cast<const uchar*>(pixels)[1] << 8));
	}
};

template<typename Shape>
struct HotShape<Shape, HasZ> : public HotShape<Shape, NoZ>
{
	uint z;
};


template<typename Shape>
struct ShapeWithDuration
{
	Shape shape;
	int16 duration = 0;
};

template<typename SHAPE>
struct AnimatedShape
{
	using Shape = SHAPE;

	static constexpr bool animated = true;

	ShapeWithDuration<Shape>* frames	 = nullptr;
	uint					  num_frames = 0;

	const ShapeWithDuration<Shape>& operator[](uint i) const noexcept { return frames[i]; }

	uint8 width(uint i = 0) const noexcept { return frames[i].width(); }
	uint8 height(uint i = 0) const noexcept { return frames[i].height(); }
	int8  hot_x(uint i = 0) const noexcept { return frames[i].hot_x(); }
	int8  hot_y(uint i = 0) const noexcept { return frames[i].hot_y(); }

	//Size size() const noexcept { return Size(width(), height()); }
	//Dist hotspot() const noexcept { return Dist(hot_x(), hot_y()); }

	AnimatedShape() noexcept = default;

	AnimatedShape(ShapeWithDuration<Shape>* frames, uint8 num_frames) noexcept : frames(frames), num_frames(num_frames)
	{}

	AnimatedShape(AnimatedShape&& q) noexcept : //
		AnimatedShape(q.frames, num_frames)
	{
		q.frames	 = nullptr;
		q.num_frames = 0;
	}

	AnimatedShape(const AnimatedShape& q) throws :
		AnimatedShape(new ShapeWithDuration<Shape>[ q.num_frames ], q.num_frames)
	{
		for (uint i = 0; i < num_frames; i++) frames[i] = q.frames[i];
	}

	AnimatedShape& operator=(AnimatedShape&& q) noexcept
	{
		assert(this != &q);
		delete[] frames;
		std::swap(num_frames, q.num_frames);
		std::swap(frames, q.frames);
		return *this;
	}

	AnimatedShape& operator=(const AnimatedShape& q) throws
	{
		if (num_frames != q.num_frames)
		{
			num_frames = 0;
			frames	   = nullptr;
			delete[] frames;
			frames	   = new ShapeWithDuration<Shape>[q.num_frames];
			num_frames = q.num_frames;
		}
		for (uint i = 0; i < num_frames; i++) frames[i] = q.frames[i];
		return *this;
	}

	~AnimatedShape() noexcept { delete[] frames; }

	void teardown() noexcept
	{
		while (num_frames) frames[--num_frames].teardown();
		delete[] frames;
		frames = nullptr;
	}
};


//
// ****************************** Implementations ************************************
//

template<>
inline void __attribute__((section(".scratch_x.next_row"))) HotShape<Shape<NotSoftened>, NoZ>::skip_row() noexcept
{
	while (1)
	{
		assert(is_pfx());
		x += dx();
		uint w = width();
		pixels += sizeof(PFX) / sizeof(*pixels) + width();
		if (!is_skip()) break;
		x += w;
		skip_cmd();
	}
}

template<>
inline void __attribute__((section(".scratch_x.next_row"))) HotShape<Shape<Softened>, NoZ>::skip_row() noexcept
{
	int hx = x << 1;
	while (1)
	{
		assert(is_pfx());
		hx += dx();
		uint w = width();
		pixels += sizeof(PFX) / sizeof(*pixels) + width();
		if (!is_skip()) break;
		hx += w;
		skip_cmd();
	}
	x = hx >> 1; // same as in render_one_row()
}

template<>
inline bool __attribute__((section(".scratch_x.render_one_row")))
HotShape<Shape<NotSoftened>, NoZ>::render_row(Color* scanline) noexcept
{
	for (;;)
	{
		assert(is_pfx());

		x += pfx().dx;
		int count = pfx().width;
		skip_pfx();
		const Color* q = pixels;
		pixels		   = q + count;

		int a = x;
		int e = a + count;
		if (a < 0)
		{
			q -= a;
			a = 0;
		}
		if (e > screen_width()) e = screen_width();

		if (!ghostly)
			while (a < e) scanline[a++] = *q++;
		else
			while (a < e) scanline[a++].blend_with(*q++);

		if (is_pfx()) return false;	 // this is the next line
		if (!is_skip()) return true; // end of shape

		// skip gap and draw more pixels
		skip_cmd();
		x += count;
	}
}

template<>
inline bool __attribute__((section(".scratch_x.render_one_row")))
HotShape<Shape<Softened>, NoZ>::render_row(Color* scanline) noexcept
{
	// "softening" is done by scaling down the image 2:1 horizontally.
	// => half-set pixels l+r of a stripe are blended with the underlying one.
	// => pfx.dx and pfx.width are measuered in 1/2 pixels.

	int hx = x << 1; // we work in "double width space"

	for (;;)
	{
		assert(is_pfx());

		int ha = hx += pfx().dx;
		int he = ha + pfx().width;
		skip_pfx();

		bool af = ha & 1; // 1: blend 1st pixel
		bool ef = he & 1; // 1: blend last pixel

		int a = ha >> 1;	   // incl. left blended pixel, if any
		int e = (he + 1) >> 1; // incl. right blended pixel, if any

		const Color* q = pixels;
		pixels		   = q + e - a; // skip over left-blended + center-full + right-blended pixels

		if (a < 0)
		{
			q -= a;
			a  = 0;
			af = 0;
		}
		if (e > screen_width())
		{
			e  = screen_width();
			ef = 0;
		}

		if (ghostly) // ghostly image => all pixels are blended
		{
			while (a < e) scanline[a++].blend_with(*q++);
		}
		else
		{
			if (af && a < e) { scanline[a++].blend_with(*q++); }
			while (a < e - ef) scanline[a++] = *q++;
			if (ef && a < e) { scanline[a].blend_with(*q); }
		}

		if (!is_skip()) break; // next line / end of shape

		// skip gap and draw more pixels
		skip_cmd();
		hx = he;
	}

	x = hx >> 1; // round down: same as in skip_row()
	return is_end();
}


// ****************************** Constructor ************************************

template<Softening SOFT>
template<Graphics::ColorMode CM>
Shape<SOFT>::Shape(
	const Graphics::Pixmap<CM>& pm, uint transparent_pixel, const Color* clut, int8 hot_x, int8 hot_y) throws
{
	// TODO SOFT

	using namespace Graphics;
	using Shape				 = Video::Shape<NotSoftened>;
	using HotShape			 = Video::HotShape<Shape, NoZ>;
	using PFX				 = HotShape::PFX;
	using CMD				 = HotShape::CMD;
	static constexpr CMD GAP = HotShape::GAP;
	static constexpr CMD END = HotShape::END;

	HotShape::PFX a_pfx;
	Color*		  pixels = nullptr;

	auto skip_transp = [&, transparent_pixel](int x, int y, int end) {
		while (x < end && pm.get_color(x, y) == transparent_pixel) x++;
		return x;
	};

	auto skip_pixels = [&, transparent_pixel](int x, int y, int end) {
		while (x < end && pm.get_color(x, y) != transparent_pixel) x++;
		return x;
	};

	auto store_pixels = [&, transparent_pixel](int x, int y, int end) {
		//TODO CLUT
		for (uint color; x < end && (color = pm.get_color(x, y)) != transparent_pixel; x++)
		{
			if constexpr (1) *pixels++ = Color(color);
			else *pixels++ = clut[color];
		}
		return x;
	};

	auto is_empty_row = [&, transparent_pixel](int y) {
		for (int x = 0; x < pm.width; x++)
			if (pm.get_color(x, y) != transparent_pixel) return false;
		return true;
	};

	auto is_empty_col = [&, transparent_pixel](int x) {
		for (int y = 0; y < pm.height; y++)
			if (pm.get_color(x, y) != transparent_pixel) return false;
		return true;
	};

	auto calc_bounding_rect = [&, transparent_pixel]() {
		Rect rect {0, 0, pm.width, pm.height};
		for (auto& y = rect.p1.y; y < pm.height && is_empty_row(y); y++) {}
		if (rect.p1.y == pm.height) return Rect {0, 0, 0, 1}; // w=0, h=1
		for (auto& y = rect.p2.y; is_empty_row(y - 1); y--) {}
		for (auto& x = rect.p1.x; is_empty_col(x); x++) {}
		for (auto& x = rect.p2.x; is_empty_col(x - 1); x--) {}
		return rect;
	};

	auto store_cmd = [&](CMD cmd) {
		uint8* p = reinterpret_cast<uint8*>(pixels);
		p[0]	 = uint8(cmd);
		p[1]	 = uint8(cmd >> 8);
	};

	for (bool hot = false;; hot = true)
	{
		Rect bbox = calc_bounding_rect();

		assert(bbox.height() <= 255);
		assert(bbox.width() <= 255);

		if (hot)
		{
			this->pixels = Colors::newColors(size_t(pixels));
			pixels		 = &this->pixels[0u];

			_width	= uint8(bbox.width());
			_height = uint8(bbox.height());
			_hot_x	= hot_x - int8(bbox.left());
			_hot_y	= hot_y - int8(bbox.top());
		}


		int x0 = bbox.left(); // left border adjusted by dx

		for (int y = bbox.top(); y < bbox.bottom(); y++)
		{
			int x = skip_transp(bbox.left(), y, bbox.right()); // find start of strip

			if (x == bbox.right()) // empty row
			{
				PFX* pfx = hot ? reinterpret_cast<PFX*>(pixels) : &a_pfx;
				pixels += sizeof(PFX) / sizeof(Color); // skip pfx

				pfx->dx	   = 0; // store empty strip
				pfx->width = 0;
				continue;
			}

			while (1)
			{
				PFX* pfx = hot ? reinterpret_cast<PFX*>(pixels) : &a_pfx;
				pixels += sizeof(PFX) / sizeof(Color);

				assert((x - x0) == int8(x - x0)); //TODO

				pfx->dx = int8(x - x0);
				x0		= x;

				x		   = hot ? store_pixels(x, y, bbox.right()) : skip_pixels(x, y, bbox.right());
				pfx->width = uint8(x - x0);

				x = skip_transp(x, y, bbox.right()); // find start of next strip
				if (x == bbox.right()) break;

				if (hot) store_cmd(GAP);
				pixels += sizeof(CMD) / sizeof(Color);
				x0 += pfx->width;
			}
		}

		if (hot) store_cmd(END);
		pixels += sizeof(CMD) / sizeof(Color);
		if constexpr (sizeof(Color) == 1) pixels += uint(pixels) & 1; // align

		if (hot) break;
	}
}


} // namespace kio::Video


/*




































*/
