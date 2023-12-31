// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Frames.h"
#include "geometry.h"
#include "no_copy_move.h"
#include <pico/sync.h>

namespace kio::Video
{

extern volatile int hot_row;		  // the currently displayed screen row
extern spin_lock_t* sprites_spinlock; // spinlock for all sprites


struct LinkedListElement
{
	LinkedListElement* next = nullptr; // linking in the internal displaylist
	LinkedListElement* prev = nullptr;
};


/* ——————————————————————————————————————————————————————————————————————
	Sprites are ghostly images which hover above a regular video image.

	Displaying sprites is quite cpu intensive.  
	Eventually the most popular use of a sprite is the display of a mouse pointer.  
	Sprites don't take ownership of the shape => they don't delete it in ~Sprite().  

	Shape must provide:
		animated
		width, height, hot_x, hot_y
		typedef Shape
	HotShape must provide:
		skip_row()
		render_row()
*/


template<typename SHAPE>
class Sprite : public LinkedListElement
{
public:
	NO_COPY_MOVE(Sprite);

	using Shape	 = SHAPE;
	using Frame	 = Video::Frame<Shape>;
	using Frames = Video::Frames<Shape>;

	static constexpr bool is_animated = false;
	static constexpr bool isa_sprite  = true;
	static_assert(Shape::isa_shape);

	Sprite(const Shape& s, const Point& p, uint16 z = 0) noexcept : shape(s), pos(p - s.hotspot()), z(z) {}
	Sprite(Shape&& s, const Point& p, uint16 z = 0) noexcept : shape(std::move(s)), pos(p - s.hotspot()), z(z) {}

	// the following ctors are mostly to support templates:
	Sprite(const Frames& frames, const Point& p, uint16 z = 0) noexcept : Sprite(frames[0].shape, p, z) {}
	Sprite(Frames&& frames, const Point& p, uint16 z = 0) noexcept : Sprite(frames[0].shape, p, z) {}
	Sprite(const Frame* frames, uint8, const Point& p, uint16 z = 0) noexcept : Sprite(frames[0].shape, p, z) {}
	Sprite(const Shape* s, const uint16*, uint8, const Point& p, uint16 z = 0) noexcept : Sprite(s[0], p, z) {}
	Sprite(const Shape* shapes, uint16, uint8, const Point& p, uint16 z = 0) noexcept : Sprite(shapes[0], p, z) {}

	~Sprite() noexcept = default;

	int	 width() const noexcept { return shape.width(); }
	int	 height() const noexcept { return shape.height(); }
	int	 hot_x() const noexcept { return shape.hot_x(); }
	int	 hot_y() const noexcept { return shape.hot_y(); }
	Size size() const noexcept { return shape.size(); }
	Dist hotspot() const noexcept { return shape.hotspot(); }

	coord get_xpos() const noexcept { return pos.x + hot_x(); }
	coord get_ypos() const noexcept { return pos.y + hot_y(); }
	void  set_xpos(int x) noexcept { pos.x = x - hot_x(); }
	void  set_ypos(int y) noexcept { pos.y = y - hot_y(); }

	Point get_position() const noexcept { return pos + hotspot(); }
	void  set_position(const Point& p) noexcept { pos = p - hotspot(); }

	bool replace(const Shape& new_shape) noexcept
	{
		// returns true if Sprite may need to be re-linked

		Dist d = shape.hotspot() - new_shape.hotspot();
		shape  = new_shape;
		pos += d;
		return d.dy != 0;
	}

	template<typename HotShape>
	void start(HotShape& hs) const noexcept
	{
		shape.start(hs, pos.x, ghostly);
	}

	Shape shape; // the compressed image of the sprite
	Point pos;

	uint16 z;					  // if HasZ
	bool   ghostly		 = false; // translucent
	uint8  current_frame = 0;	  // if animated
};


} // namespace kio::Video

/*


































*/
