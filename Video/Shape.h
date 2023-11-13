// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Graphics/Pixmap_wAttr.h"
#include "Graphics/geometry.h"
#include "VideoBackend.h"
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


enum Animation : bool { NotAnimated = 0, Animated = 1 };
enum Softening : bool { NotSoftened = 0, Softened = 1 };


template<Animation>
struct FramePrefix
{
	uint8 width;
	uint8 height;
	int8  hot_x;
	int8  hot_y;

	static constexpr uint8 duration	  = 0x7f;
	static constexpr int16 next_frame = 0;
	static constexpr uint8 magic	  = 0x7e;
};

template<>
struct FramePrefix<Animated> : public FramePrefix<NotAnimated>
{
	// the additional fields must come first:
	int16 next_frame; // offset to next frame data measured in pixels
	uint8 duration;	  // frames
	uint8 magic;	  // for asserts

	uint8 width;
	uint8 height;
	int8  hot_x;
	int8  hot_y;
};


template<Animation ANIM, Softening SOFT>
struct Shape
{
	// ***** types *****

	using ColorMode = Graphics::ColorMode;
	template<ColorMode CM>
	using Pixmap   = Graphics::Pixmap<CM>;
	using Rect	   = Graphics::Rect;
	using Preamble = FramePrefix<ANIM>;

	enum CMD : uint16 { END = 0x0080, GAP = 0x0180 }; // little endian

	struct PFX // raw pixels prefix
	{
		int8  dx;	 // initial offset
		uint8 width; // count of pixels that follow
	};


	// *****data member *****

	const Color* pixels; // struct Shape just wraps this pointer


	// ***** constructors *****

	Shape(const Color* c = nullptr) noexcept : pixels(c) {}

	template<ColorMode CM>
	Shape(Pixmap<CM>& pm, uint transparent_pixel, const Color* clut, int8 hot_x = 0, int8 hot_y = 0) throws :
		pixels(new_frame(pm, transparent_pixel, clut, hot_x, hot_y))
	{}

	template<ColorMode CM, typename = std::enable_if_t<Graphics::is_true_color(CM)>>
	Shape(Pixmap<CM>& pm, uint transparent_pixel, int8 hot_x = 0, int8 hot_y = 0) throws :
		pixels(new_frame(pm, transparent_pixel, nullptr, hot_x, hot_y))
	{}


	// ***** utility functions *****

	const Preamble& preamble() const noexcept
	{
		assert(reinterpret_cast<const Preamble*>(pixels)->magic == 0x7e); // 90% if animated, 0% if not
		return *reinterpret_cast<const Preamble*>(pixels);
	}

	Shape					 next_frame() noexcept { return Shape(pixels + preamble().next_frame); }
	uint8					 duration() const noexcept { return preamble().duration; }
	Shape<NotAnimated, SOFT> get_static_part() noexcept; // get current frame as not-animated Shape
	int8					 hot_x() const noexcept { return preamble().hot_x; }
	int8					 hot_y() const noexcept { return preamble().hot_y; }
	void					 skip_preamble() noexcept { pixels += sizeof(Preamble) / sizeof(*pixels); }


	bool is_cmd() const noexcept { return reinterpret_cast<const PFX*>(pixels)->dx == -128; }
	bool is_pfx() const noexcept { return reinterpret_cast<const PFX*>(pixels)->dx != -128; }
	bool is_end() const noexcept
	{
		if constexpr (sizeof(Color) == 1) return pixels[0] == uint8(END) && pixels[1] == uint8(END >> 8);
		else return *reinterpret_cast<const CMD*>(pixels) == END;
	}
	bool is_skip() const noexcept { return *reinterpret_cast<const CMD*>(pixels) == GAP; }

	const CMD& cmd() const noexcept { return *reinterpret_cast<const CMD*>(pixels); }
	const PFX& pfx() const noexcept { return *reinterpret_cast<const PFX*>(pixels); }

	int8  dx() const noexcept { return pfx().dx; }
	uint8 width() const noexcept { return pfx().width; }
	void  skip_cmd() noexcept { pixels += sizeof(CMD) / sizeof(*pixels); }
	void  skip_pfx() noexcept { pixels += sizeof(PFX) / sizeof(*pixels); }

	void skip_row(int& x) noexcept;
	bool render_row(int& x, Color* scanline, bool blend) noexcept;

private:
	template<ColorMode CM>
	static Color*
	new_frame(Graphics::Pixmap<CM>& pm, uint transparent_pixel, const Color* clut, int8 hot_x, int8 hot_y) throws;
};


//
// ****************************** Implementations ************************************
//

template<Animation ANIM, Softening SOFT>
Shape<NotAnimated, SOFT> Shape<ANIM, SOFT>::get_static_part() noexcept
{
	// skip over .next_frame and .duration:
	const Color* p = pixels + (sizeof(FramePrefix<ANIM>) - sizeof(FramePrefix<NotAnimated>)) / sizeof(Color);
	return Shape<NotAnimated, SOFT>(p);
}

template<Animation ANIM, Softening SOFT>
inline void __attribute__((section(".scratch_x.next_row"))) Shape<ANIM, SOFT>::skip_row(int& x) noexcept
{
	if constexpr (SOFT == NotSoftened)
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
	else
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
}

template<Animation ANIM, Softening SOFT>
inline bool __attribute__((section(".scratch_x.render_one_row")))
Shape<ANIM, SOFT>::render_row(int& x, Color* scanline, bool blend) noexcept
{
	if constexpr (SOFT == NotSoftened)
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

			if (!blend)
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
	else // Softened
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

			if (blend) // ghostly image requested => all pixels are blended
			{
				while (a < e) scanline[a++].blend_with(*q++);
			}
			else
			{
				if (af && a < e) { scanline[a++].blend_with(*q++); }
				while (a < e - ef) scanline[a++] = *q++;
				if (ef && a < e) { scanline[a].blend_with(*q); }
			}

			if (is_pfx()) return false;	 // this is the next line
			if (!is_skip()) return true; // end of shape

			// skip gap and draw more pixels
			skip_cmd();
			hx = he;
		}

		x = hx >> 1; // round down: same as in skip_row()
	}
}


// ****************************** Constructor helpers ************************************

template<Graphics::ColorMode CM>
Color* create_frame(Graphics::Pixmap<CM>& pm, uint transparent_pixel, const Color* clut, int8 hot_x, int8 hot_y) throws
{
	// create a NotAnimated shape in the supplied pixels[] buffer
	// return pointer behind the created data.
	// used in the constructor

	using namespace Graphics;
	using Shape = Video::Shape<NotAnimated, NotSoftened>;

	Shape::PFX a_pfx;
	Color*	   pixels0 = nullptr;
	Color*	   pixels  = nullptr;

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

	auto store_cmd = [&](Shape::CMD cmd) {
		uint8* p = reinterpret_cast<uint8*>(pixels);
		p[0]	 = uint8(cmd);
		p[1]	 = uint8(cmd >> 8);
	};

	for (bool hot = false;; hot = true)
	{
		if (hot) pixels0 = new Color[size_t(pixels - pixels0)];

		Graphics::Rect bbox = calc_bounding_rect();

		assert(bbox.height() <= 255);
		assert(bbox.width() <= 255);

		if (hot)
		{
			Shape::Preamble& p = *reinterpret_cast<Shape::Preamble*>(pixels);
			p.width			   = uint8(bbox.width());
			p.height		   = uint8(bbox.height());
			p.hot_x			   = hot_x - int8(bbox.left());
			p.hot_y			   = hot_y - int8(bbox.top());
		}

		pixels += sizeof(Shape::Preamble) / sizeof(Color); // skip preamble

		int x0 = bbox.left(); // left border adjusted by dx

		for (int y = bbox.top(); y < bbox.bottom(); y++)
		{
			int x = skip_transp(bbox.left(), y, bbox.right()); // find start of strip

			if (x == bbox.right()) // empty row
			{
				Shape::PFX* pfx = hot ? reinterpret_cast<Shape::PFX*>(pixels) : &a_pfx;
				pixels += sizeof(Shape::PFX) / sizeof(Color); // skip pfx

				pfx->dx	   = 0; // store empty strip
				pfx->width = 0;
				continue;
			}

			while (1)
			{
				Shape::PFX* pfx = hot ? reinterpret_cast<Shape::PFX*>(pixels) : &a_pfx;
				pixels += sizeof(Shape::PFX) / sizeof(Color);

				assert((x - x0) == int8(x - x0)); //TODO

				pfx->dx = int8(x - x0);
				x0		= x;

				x		   = hot ? store_pixels(x, y, bbox.right()) : skip_pixels(x, y, bbox.right());
				pfx->width = uint8(x - x0);

				x = skip_transp(x, y, bbox.right()); // find start of next strip
				if (x == bbox.right()) break;

				if (hot) store_cmd(Shape::GAP);
				pixels += sizeof(Shape::CMD) / sizeof(Color);
				x0 += pfx->width;
			}
		}

		if (hot) store_cmd(Shape::END);
		pixels += sizeof(Shape::CMD) / sizeof(Color);
		if constexpr (sizeof(Color) == 1) pixels += uint(pixels) & 1; // align

		if (hot) return pixels0;
	}
}

template<Animation ANIM, Softening SOFT>
template<Graphics::ColorMode CM>
Color*
Shape<ANIM, SOFT>::new_frame(Pixmap<CM>& pm, uint transparent_pixel, const Color* clut, int8 hot_x, int8 hot_y) throws
{
	return create_frame(pm, transparent_pixel, clut, hot_x, hot_y);
}


} // namespace kio::Video


/*




































*/
