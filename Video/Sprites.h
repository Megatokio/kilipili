// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Shapes.h"
#include "VideoPlane.h"
#include <tuple>

#ifndef MAX_HOT_SPRITES
  #define MAX_HOT_SPRITES 20
#endif


namespace kio::Video
{
using coord = Graphics::coord;
using Point = Graphics::Point;


/**	Sprites are ghostly images which hover above a regular video image.

	Displaying sprites is quite cpu intensive.  
	Eventually the most popular use of a sprite is the display of a mouse pointer.  
	Sprites don't manage the lifetime of the shape => they don't delete it in ~Sprite().  

	Sprites can be subclassed because they have a virtual vblank() function
	which is called by the Video engine at the start of each frame.
*/
class Sprite
{
	NO_COPY_MOVE(Sprite);
	Sprite* prev = nullptr; // linking in the internal displaylist
	Sprite* next = nullptr;

public:
	static constexpr uint16 zMin = 0;
	static constexpr uint16 zMax = 0xffffu;

	Shape shape; // the compressed image of the sprite.

	uint8 width() const noexcept { return shape.preamble().width; }
	uint8 height() const noexcept { return shape.preamble().height; }
	int8  hot_x() const noexcept { return shape.preamble().hot_x; }
	int8  hot_y() const noexcept { return shape.preamble().hot_y; }

	coord		 x = 0; // position of top left corner, already adjusted by hot_x.
	coord		 y = 0; // position of top left corner, already adjusted by hot_y.
	const uint16 z = 0; // layer: higher z values overlap lower z values.

	int16 last_frame = 0; // used during vblank

	coord  xpos() const noexcept { return x + hot_x(); }
	coord  ypos() const noexcept { return y + hot_y(); }
	uint16 zpos() const noexcept { return z; }

	//Sprite() noexcept = default;
	Sprite(const Shape& shape, int x, int y, uint16 z) noexcept : shape(shape), x(x - hot_x()), y(y - hot_y()), z(z) {}
	virtual ~Sprite() noexcept; // does not delete the shape

	virtual void vblank() noexcept {}

	// modify the Sprite:

	void move(coord x, coord y) noexcept;
	void move(const Point& p) noexcept;
	void modify(const Shape& s, coord x, coord y, bool wait = 0) noexcept;
	void modify(const Shape& s, const Point& p, bool wait = 0) noexcept;
	void replace(const Shape& s, bool wait = 0) noexcept;

	void show() noexcept;
	void hide(bool wait = 0) noexcept;
	bool is_on_screen() const noexcept { return is_in_displaylist(); }

	bool is_hot() const noexcept;
	void wait_while_hot() const noexcept;

private:
	friend class Sprites;

	// these expect the displaylist to be locked:
	void _unlink() noexcept;
	void _link_after(Sprite*) noexcept;
	void _link_before(Sprite*) noexcept;
	void _link() noexcept;
	void _move(coord x, coord y) noexcept;
	bool is_in_displaylist() const noexcept;
	void add_to_hotlist(int dy) const noexcept;
};


/*	==============================================================================
	Class Sprites is a VideoPlane which can be added to the VideoController
	to display sprites.
*/
class Sprites : public VideoPlane
{
	friend class Sprite;

	static Sprite* displaylist;

public:
	Sprites() noexcept					 = default;
	virtual ~Sprites() noexcept override = default;

	virtual void setup(coord width) override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* scanline) noexcept override;

	static void add(Sprite* sprite) noexcept { sprite->show(); }
	static void remove(Sprite* sprite, bool wait) noexcept { sprite->hide(wait); }
	static void clear_displaylist() noexcept;
};


// ============================================================================

inline bool Sprite::is_in_displaylist() const noexcept { return prev || this == Sprites::displaylist; }


} // namespace kio::Video

/*


































*/
