// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Shape.h"
#include "geometry.h"
#include "no_copy_move.h"
#include <pico/sync.h>

namespace kio::Video
{

extern volatile int hot_row;		  // the currently displayed screen row
extern spin_lock_t* sprites_spinlock; // spinlock for all sprites


/*
	Sprites are ghostly images which hover above a regular video image.

	Displaying sprites is quite cpu intensive.  
	Eventually the most popular use of a sprite is the display of a mouse pointer.  
	Sprites don't manage the lifetime of the shape => they don't delete it in ~Sprite().  

	Shape must provide:
		animated, 
		width, height, hot_x, hot_y, 
		next_frame(),
		template<ZPlane> typedef HotShape
	HotShape must provide:
		skip_row()
		render_row()
*/

struct LinkedListElement
{
	LinkedListElement* next = nullptr; // linking in the internal displaylist
	LinkedListElement* prev = nullptr;
};


template<typename SHAPE, typename = void>
class Sprite : protected LinkedListElement
{
	template<typename Sprite, ZPlane WZ>
	friend class Sprites;

public:
	NO_COPY_MOVE(Sprite);

	using Shape					   = SHAPE;
	static constexpr bool animated = Shape::animated;
	static_assert(!animated);

	Sprite(Shape s, const Point& p, uint16 z = 0) noexcept : shape(s), pos(p - s.hotspot()), z(z) {}
	~Sprite() noexcept = default;

	int width() const noexcept { return shape.width(); }
	int height() const noexcept { return shape.height(); }
	int hot_x() const noexcept { return shape.hot_x(); }
	int hot_y() const noexcept { return shape.hot_y(); }

	coord get_xpos() const noexcept { return pos.x + hot_x(); }
	coord get_ypos() const noexcept { return pos.y + hot_y(); }
	void  set_xpos(int x) noexcept { pos.x = x - hot_x(); }
	void  set_ypos(int y) noexcept { pos.y = y - hot_y(); }

	Size  size() const noexcept { return Size(width(), height()); }
	Dist  hotspot() const noexcept { return Dist(hot_x(), hot_y()); }
	Point get_position() const noexcept { return pos + hotspot(); }
	void  set_position(const Point& p) noexcept { pos = p - hotspot(); }

	bool replace(const Shape& new_shape) noexcept // may also need re-linking
	{
		int dx = shape.hot_x() - new_shape.hot_x();
		int dy = shape.hot_y() - new_shape.hot_y();
		shape  = new_shape;
		pos.x += dx;
		pos.y += dy;
		return dy != 0;
	}
	void		  next_frame() noexcept {}
	constexpr int num_frames() const noexcept { return 1; }

	bool is_hot() const noexcept { return hot_row >= pos.y && hot_row < pos.y + height(); }
	void wait_while_hot() const noexcept
	{
		while (is_hot()) sleep_us(500);
	}


	Shape shape; // the compressed image of the sprite
	Point pos;	 // position of top left corner, adjusted by hotspot

	uint16 z;				  // if HasZ
	bool   ghostly	 = false; // translucent
	uint8  frame_idx = 0;	  // if animated
};


template<typename SHAPE>
class Sprite<SHAPE, typename std::enable_if_t<SHAPE::animated>> : public Sprite<typename SHAPE::Shape>
{
	template<typename Sprite, ZPlane WZ>
	friend class Sprites;

public:
	using super = Sprite<typename SHAPE::Shape>;
	using Shape = SHAPE;

	static constexpr bool animated = true;

	using super::frame_idx;

	Shape animated_shape;
	int	  countdown;

	Sprite(Shape shape, const Point& p, uint16 z = 0) noexcept : // ctor
		super(shape[0], p, z),
		animated_shape(std::move(shape)),
		countdown(shape[0].duration)
	{}
	~Sprite() noexcept = default;

	int num_frames() const noexcept { return animated_shape.num_frames; }

	bool replace(Shape q) noexcept // may also need re-linking
	{
		animated_shape = std::move(q);
		return super::replace(q[0]);
	}

	bool next_frame() noexcept // may also need re-linking
	{
		if constexpr (animated)
		{
			if (++frame_idx >= num_frames()) frame_idx = 0;
			countdown = animated_shape[frame_idx].duration;
			return super::replace(animated_shape[frame_idx]);
		}
	}
};

} // namespace kio::Video

/*


































*/
