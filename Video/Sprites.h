// Copyright (c) 2022 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap.h"
#include "VideoPlane.h"
#include <tuple>


#ifndef MAX_HOT_SPRITES
  #define MAX_HOT_SPRITES 20
#endif

#ifndef SPRITES_USE_PERFECT_TRANSPARENCY
  #define SPRITES_USE_PERFECT_TRANSPARENCY 1
#endif


namespace kipili::Graphics
{
template<ColorMode CM, typename>
class Pixmap;
}

namespace kipili::Video
{
class Sprite;
using coord		= Graphics::coord;
using Point		= Graphics::Point;
using ColorMode = Graphics::ColorMode;
using Color		= Graphics::Color;


/*	==============================================================================
	Class Sprites is a VideoPlane which can be added to the Scanvideo engine
	to display sprites.
	Currently there can only be one Sprites instance.
*/
class Sprites : public VideoPlane
{
	friend class Sprite;

	static Sprite* displaylist;
	static int16   current_frame;

public:
	Sprites() noexcept					 = default;
	virtual ~Sprites() noexcept override = default;

	virtual void setup(uint plane, coord width, VideoQueue& vq) override;
	virtual void teardown(uint plane, VideoQueue& vq) noexcept override;
	virtual void vblank() noexcept override;
	virtual uint renderScanline(int row, uint32* out_data) noexcept override;

	static uint16* render_scanline(uint16* buffer, uint bu_max, int row);

	static void add(Sprite*) noexcept;
	static void remove(Sprite*, bool wait) noexcept;
	static void clear_displaylist() noexcept;
	static bool hotlist_is_empty() noexcept;
};


/*	==============================================================================
	Shapes are "compressed" pixel maps used for sprites.
	A Shape is a stream of 'compressed' Shape::Rows
	and a 'hot spot' used as offset when positioning the sprite.
*/
struct Shape
{
	struct Row
	{
		/*	Shapes are "compressed" pixel maps used for sprites.

		A Shape is a stream of rows.
		Each row starts with `dx` and `width`, followed by that much pixels.
		The Shape is ended by a stopper value 0x80 (or -128) for dx.

		=>	{ int8 dx, uint8 width, VgaColor[width] } * height, 0x80

		dx defines the horizontal shift of the start of this row relative to the previous x position.
		So if the left contour of the shape is a straight vertical line then all dx offsets are 0.
		As these offsets accumulate, weird shapes like long diagonal lines could be defined.

		The rows of the shape can contain transparent pixels, but this should only be used inside
		the row due to incomplete handling of overlapping sprites. The left and right contour
		should be shaped by trimming dx and width appropriately so that overlapping sprites can
		eclipse each other as expected.

		The shape can contain empty lines with width=0. In this case dx should be 0 to avoid
		unneccessary reordering of the sprite in the hotlist.
		Empty lines at the start of the shape should be avoided if this doesn't make maintaining
		the y position of the sprite too hard, because they simply cost cpu time.
		Empty lines at the end of the sprite should always be removed because that's easy to do.
	*/

		static constexpr int8 dxStopper = -128;

		int8  dx; // offset of pixel start compared to previous line
		uint8 width;
		Color pixels[0];

		bool	   is_at_end() const noexcept { return dx == dxStopper; }
		Row*	   next() noexcept { return reinterpret_cast<Row*>(pixels + width); }
		const Row* next() const noexcept { return reinterpret_cast<const Row*>(pixels + width); }

		Row*		construct(int width, int height, const Color* pixels) noexcept; // kind of placement new
		static Row* newShape(int width, int height, const Color* pixels);			// throws oomem
		static Row* newShape(const Graphics::Pixmap_rgb&);							// throws oomem

		static constexpr std::pair<int, int> calc_sz(int width, const Color* pixels) noexcept;
		static constexpr std::pair<int, int> calc_size(int width, int height, const Color* px) noexcept;
	};

	static Row row_with_stopper;

	int16 width	 = 0;
	int16 height = 0;
	int16 hot_x	 = 0;
	int16 hot_y	 = 0;
	Row*  rows	 = &row_with_stopper;

	Shape() noexcept = default;
	explicit Shape(const Graphics::Pixmap_rgb&, int16 hot_x = 0, int16 hot_y = 0);
	Shape(int16 w, int16 h, Row* rows, int16 hot_x = 0, int16 hot_y = 0) noexcept :
		width(w),
		height(h),
		rows(rows),
		hot_x(hot_x),
		hot_y(hot_y)
	{}
};


/*	==============================================================================
	Sprites are images which can be displayed hovering above a regular video image.
*/
class Sprite
{
	/*	Sprites are images which can be displayed hovering above a regular video image.
	Displaying sprites is quite cpu intensive.
	Eventually the most popular use of a sprite is the display of a mouse pointer.
	Sprites don't manage the lifetime of the shape => they don't delete it in ~Sprite().

	y: the top position of the sprite.
	x: the left position, but actual start of pixel rows is offset by dx in the shape.
	z: the layer position, higher z values overlap lower z values.

	Sprites can be subclassed because they have a virtual vblank() function
	which is called by the Scanvideo engine at the start of each frame.
*/

	NO_COPY_MOVE(Sprite);
	friend class Sprites;
	friend struct HotList;

	Sprite* prev = nullptr; // linking in the internal displaylist
	Sprite* next = nullptr;

	Shape shape; // the compressed image of the sprite.

	coord  x		  = 0;						// position of top left corner
	coord  y		  = 0;						// position of top left corner
	uint16 z		  = 0;						// layer
	int16  last_frame = Sprites::current_frame; // used during vblank

	// these expect the displaylist to be locked:
	void unlink() noexcept;
	void link_after(Sprite*) noexcept;
	void link_before(Sprite*) noexcept;
	void link() noexcept;
	void move_p0(coord x, coord y) noexcept;
	bool is_in_displaylist() const noexcept { return prev || this == Sprites::displaylist; }

public:
	static constexpr uint16 zMin	 = 0;
	static constexpr uint16 zStopper = 0xffffu;		 // stopper used in scanlineRenderer
	static constexpr uint16 zMouse	 = zStopper - 1; // z value for mouse pointer: above all
	static constexpr uint16 zMax	 = zStopper - 2;

	coord  xpos() const noexcept { return x + shape.hot_x; }
	coord  ypos() const noexcept { return y + shape.hot_x; }
	uint16 layer() const noexcept { return z; }
	coord  width() const noexcept { return shape.width; }
	coord  height() const noexcept { return shape.height; }

	Sprite() noexcept = default;
	Sprite(const Shape& s, coord x, coord y, uint16 z) noexcept : shape(s), x(x - s.hot_x), y(y - s.hot_y), z(z)
	{
		assert(z != zStopper);
	}
	virtual ~Sprite() noexcept; // does not delete the shape

	virtual void vblank() noexcept {}

	// modify the Sprite:

	void move(coord x, coord y) noexcept;
	void move(const Point& p) noexcept;
	void replace(const Shape& s, bool wait = 0) noexcept;
	void modify(const Shape& s, coord x, coord y, bool wait = 0) noexcept;
	void modify(const Shape& s, const Point& p, bool wait = 0) noexcept;

	void show() noexcept;
	void hide(bool wait = 0) noexcept;
	bool is_visible() const noexcept { return is_in_displaylist(); }

	bool is_hot() const noexcept;
	void wait() const noexcept; // wait while hot
};


// ============================================================================

using std::pair;
using std::tie;

constexpr pair<int, int> Shape::Row::calc_sz(int width, const Color* pixels) noexcept
{
	// calculate start offset dx and width of non-transparent pixels in pixel row
	// returns dx=0 and width=0 for fully transparent lines

	int dx = 0;
	while (width && pixels[width - 1].is_transparent()) { width--; }
	if (width)
		while (pixels[dx].is_transparent()) { dx++; }
	width -= dx;

	return {dx, width};
}

constexpr pair<int, int> Shape::Row::calc_size(int width, int height, const Color* px) noexcept
{
	// calculate size in uint16 words for Shape
	// and reduce height for empty bottom lines

	int cnt	 = 1; // +1 for the stopper
	int ymax = 0; // height without empty lines at bottom of shape

	for (int y = 0; y < height; y++, px += width)
	{
		int dx, sz;
		tie(dx, sz) = calc_sz(width, px);
		cnt += sz;
		if (sz) ymax = y;
	}

	height = ymax + 1;
	cnt += height * 1; // +1 for dx&width

	return {cnt, height};
}

inline void Sprites::add(Sprite* s) noexcept { s->show(); }

inline void Sprites::remove(Sprite* s, bool wait) noexcept { s->hide(wait); }


} // namespace kipili::Video
