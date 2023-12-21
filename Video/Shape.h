// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Pixmap_wAttr.h"
#include "RCPtr.h"
#include "VideoBackend.h"
#include "atomic.h"
#include "cdefs.h"
#include "geometry.h"
#include "video_types.h"


/* —————————————————————————————————————————————————————————
	A Shape defines the static shape of a sprite.
	A HotShape renders the shape.
	A Sprite contains a Shape and adds some state like x and y position.
	A AnimatedShape consists of many frames (Shapes).
	A AnimatedSprite contains a Shape and adds some state like position and frame state.
	A SingleSpritePlane is a VideoPlane which contains just one Sprite.
	A MultiSpritesPlane is a VideoPlane which can contain many Sprites.

	Hints for writing own variants: (if you need it)
	
	A HotShape must provide:	
		bool skip_row() noexcept;
		bool render_row(Color* out_pixels) noexcept;
		
	A Shape must provide:
		typedef HotShape;
		static constexpr bool isa_shape	  = true;  // debugging aid
		uint8 width() const noexcept;
		uint8 height() const noexcept;
		int8  hot_x() const noexcept;
		int8  hot_y() const noexcept;
		void  start(HotShape&, int x, bool ghostly) const noexcept;
 		
   ————————————————————————————————————————————————————————— */


namespace kio::Video
{

/* ——————————————————————————————————————————————————————————————————————————
	reference-counted array of pixels with intermixed commands.
*/
class Pixels
{
private:
public:
	mutable uint16 rc;
	uint16		   count;
	Color		   pixels[8888];

	static Pixels* newPixels(uint cnt) throws
	{
		Pixels* z = reinterpret_cast<Pixels*>(new char[cnt * sizeof(Color) + 2 * sizeof(uint16)]);
		z->count  = uint16(cnt);
		z->rc	  = 0;
		return z;
	}

	uint16 refcnt() const noexcept { return rc; }

	Color&		 operator[](uint i) noexcept { return pixels[i]; }
	const Color& operator[](uint i) const noexcept { return pixels[i]; }

private:
	NO_COPY_MOVE(Pixels);
	Pixels() noexcept  = delete;
	~Pixels() noexcept = default;

	friend class kio::RCPtr<Pixels>;
	friend class kio::RCPtr<const Pixels>;

	void retain() const noexcept { pp_atomic(rc); }
	void release() const noexcept
	{
		if (mm_atomic(rc) == 0) delete[] ptr(this);
	}
};


/* ——————————————————————————————————————————————————————————————————————————
	A HotShape provides the function to render the shape.
	
	A Shape is merely a string of true color pixels
	of which some are interpreted as commands to define how they are placed.

	layout of one row:
	
		{dx,width} pixels[width]  N*{ cmd:gap {dx,width} pixels[width] } 

	Each row starts with a HDR {dx,width} and then that number of colors follow.
	After that there is the HDR of the next row or a CMD.
	In case of a CMD handle this CMD as part of the current line:
		 END:  shape is finished.
		 SKIP: resume one more HDR at the current position: used to insert space.
*/
struct HotShape
{
	__always_inline bool skip_row() noexcept;
	__always_inline bool render_row(Color* scanline) noexcept;
	__always_inline void init(const Color* pixels, int x, bool ghostly) noexcept
	{
		this->pixels  = pixels;
		this->x		  = x;
		this->ghostly = ghostly;
	}

	struct PFX // raw pixels prefix
	{
		int8  dx;	 // initial offset
		uint8 width; // count of pixels that follow
	};

	enum CMD : uint16 { END = 0x0080, GAP = 0x0180 }; // little endian

protected:
	const Color* pixels = nullptr;
	int			 x;
	bool		 ghostly;
	char		 _padding[3];

	const __always_inline PFX& pfx() const noexcept { return *reinterpret_cast<const PFX*>(pixels); }
	__always_inline CMD		   cmd() const noexcept
	{
		if constexpr (sizeof(Color) >= sizeof(CMD)) return *reinterpret_cast<const CMD*>(pixels);
		return CMD(reinterpret_cast<const uchar*>(pixels)[0] + (reinterpret_cast<const uchar*>(pixels)[1] << 8));
	}

	__always_inline void  skip_cmd() noexcept { pixels += sizeof(CMD) / sizeof(*pixels); }
	__always_inline void  skip_pfx() noexcept { pixels += sizeof(PFX) / sizeof(*pixels); }
	__always_inline bool  is_cmd() const noexcept { return pfx().dx == -128; }
	__always_inline bool  is_pfx() const noexcept { return pfx().dx != -128; }
	__always_inline bool  is_end() const noexcept { return cmd() == END; }
	__always_inline bool  is_skip() const noexcept { return cmd() == GAP; }
	__always_inline int8  dx() const noexcept { return pfx().dx; }
	__always_inline uint8 width() const noexcept { return pfx().width; }
};


/*	——————————————————————————————————————————————————————————————————————————
	A Shape defines the shape of a sprite.
	It provides a typedef for a HotShape which can render this Shape.
	It provides methods to measure the Shape.
	It provides a method to setup a HotShape.
*/
struct Shape
{
	using HotShape = Video::HotShape;

	static constexpr bool isa_shape = true;

	__always_inline uint8 width() const noexcept { return _width; }
	__always_inline uint8 height() const noexcept { return _height; }
	__always_inline int8  hot_x() const noexcept { return _hot_x; }
	__always_inline int8  hot_y() const noexcept { return _hot_y; }
	Size				  size() const noexcept { return Size(_width, _height); }
	Dist				  hotspot() const noexcept { return Dist(_hot_x, _hot_y); }

	__always_inline void start(HotShape& hs, int x, bool ghostly) const noexcept
	{
		hs.init(pixels->pixels, x, ghostly);
	}

	template<typename Pixmap>
	Shape(const Pixmap& pm, int transp, const Dist& hotspot, const Color* clut) throws;
	Shape() noexcept {}

	template<typename Pixmap>
	static int calc_count(const Pixmap& pm, int transp, uint8* _height) noexcept;

	template<typename Pixmap>
	void create_shape(const Pixmap& pm, int transp, const Color* clut) noexcept;

private:
	RCPtr<Pixels> pixels;

	uint8 _width  = 0;
	uint8 _height = 0;
	int8  _hot_x  = 0;
	int8  _hot_y  = 0;
};


/* ——————————————————————————————————————————————————————————————————————————
	A HotShape provides the function to render the shape.
	A HotSoftenedShape renders a shape with softened l+r edges:
	This can be thought as having a double width shape compressed horizontally 2:1 
	and the half-set pixels at the edges are rendered half-transparent.
*/
struct HotSoftenedShape : public HotShape
{
	bool skip_row() noexcept;
	bool render_row(Color* scanline) noexcept;
};

struct SoftenedShape : public Shape
{
	// sprites are scaled 2:1 horizontally, odd pixels l+r are set using blend

	using HotShape = Video::HotSoftenedShape;
	// TODO ctor
};


//
// ************************************************************************
//

inline bool __section(".scratch_x.shape") HotShape::skip_row() noexcept
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
	return !is_pfx(); // true => end of shape
}

inline bool __section(".scratch_x.shape") HotSoftenedShape::skip_row() noexcept
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
	x = hx >> 1;	  // same as in render_one_row()
	return !is_pfx(); // true => end of shape
}

inline bool __section(".scratch_x.shape") HotShape::render_row(Color* scanline) noexcept
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

inline bool __section(".scratch_x.shape") HotSoftenedShape::render_row(Color* scanline) noexcept
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

//static
template<typename Pixmap>
int Shape::calc_count(const Pixmap& pm, int transparent_pixel, uint8* _height) noexcept
{
	// calculate exact number of pixels required for this shape:

	static constexpr int pixels_per_cmd = sizeof(HotShape::PFX) / sizeof(Color);

	int height		= 1;				  // 1 line is mandatory
	int total_count = 1 * pixels_per_cmd; // CMD::END

	for (int x0 = 0, x, w = pm.width, y = 0; y < pm.height; y++)
	{
		for (x = 0; x < w && pm.get_color(x, y) == transparent_pixel;) x++;

		if (x == w) // empty line
		{
			x0 = 127 + !!x0;
			continue;
		}

		height = y + 1;

		for (;;)
		{
			while (unlikely((x - x0) != int8(x - x0))) // dx too far?
			{
				x0 += x < x0 ? -128 : +127;
				total_count += 2 * pixels_per_cmd; // PFX + CMD::GAP
			}

			x0 = x;

			while (x < w && pm.get_color(x, y) != transparent_pixel) x++;
			total_count += x - x0;

			int gap = x;
			while (x < w && pm.get_color(x, y) == transparent_pixel) x++;
			if (x == w) break;

			x0 = gap;
			total_count += 2 * pixels_per_cmd; // PFX + CMD::GAP
		}
	}

	if (_height) *_height = uint8(height);
	return total_count + height * pixels_per_cmd; // 1 PFX per line
}

template<typename Pixmap>
void Shape::create_shape(const Pixmap& pm, int transparent_pixel, const Color* clut) noexcept
{
	using PFX = HotShape::PFX;
	using CMD = HotShape::CMD;

	assert(pm.width <= 255);
	assert(pm.height <= 255);
	assert(!clut == is_true_color(pm.colormode));

	static constexpr int pixels_per_cmd = sizeof(PFX) / sizeof(Color);

	auto store_cmd = [](Color*& dp, CMD cmd) {
		if constexpr (pixels_per_cmd == 1) //
			*dp++ = Color(cmd);
		else
		{
			*dp++ = Color(cmd & 0xff);
			*dp++ = Color(cmd >> 8);
		}
	};
	auto store_pfx = [](Color*& dp, int8 dx, uint8 w) {
		if constexpr (pixels_per_cmd == 1) //
			*dp++ = Color(uint(dx + (w << 8)));
		else
		{
			*dp++ = Color(uint8(dx));
			*dp++ = Color(w);
		}
	};

	Color* dp = pixels->pixels;

	for (int x0 = 0, x, y = 0; y < _height; y++)
	{
		for (x = 0; x < pm.width && pm.get_color(x, y) == transparent_pixel;) x++;

		if (x == pm.width) // empty line
		{
			uint8 z = pm.width / 2 + !!x0;
			store_pfx(dp, int8(z - x0), 0);
			x0 = z;
			continue;
		}

		for (;;)
		{
			while (unlikely((x - x0) != int8(x - x0))) // dx too far?
			{
				int8 dx = x < x0 ? -128 : +127;
				x0 += dx;
				store_pfx(dp, dx, 0);
				store_cmd(dp, CMD::GAP);
			}

			PFX* pfx = reinterpret_cast<PFX*>(dp);
			dp += pixels_per_cmd;
			pfx->dx = int8(x - x0);
			x0		= x;

			while (x < pm.width)
			{
				Color pixel = pm.get_color(x, y);
				if unlikely (pixel == transparent_pixel) break;
				*dp++ = clut ? clut[pixel] : pixel;
				x++;
			}
			pfx->width = uint8(x - x0);

			int gap = x;
			while (x < pm.width && pm.get_color(x, y) == transparent_pixel) x++;
			if (x == pm.width) break;

			x0 = gap;
			store_cmd(dp, CMD::GAP);
		}
	}

	store_cmd(dp, CMD::END);
	assert_le(dp, pixels->pixels + pixels->count);
}

template<typename Pixmap>
Shape::Shape(const Pixmap& pm, int transparent_pixel, const Dist& hotspot, const Color* clut) throws
{
	assert(pm.width <= 255);
	assert(pm.height <= 255);
	assert(!clut == is_true_color(pm.colormode));

	_width = uint8(pm.width);
	_hot_x = int8(hotspot.dx);
	_hot_y = int8(hotspot.dy);

	int count = calc_count(pm, transparent_pixel, &_height);
	pixels	  = Pixels::newPixels(uint(count));
	assert(pixels->rc == 1);
	create_shape(pm, transparent_pixel, clut);
}


} // namespace kio::Video


/*



































  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

*/
