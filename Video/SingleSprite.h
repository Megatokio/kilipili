// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Shape.h"
#include "VideoPlane.h"
#include <pico/sync.h>

namespace kio::Video
{

extern spin_lock_t* singlesprite_spinlock;

/*
	A VideoPlane for one single Sprite.
	Intended for mouse pointer or player character.
	
	Variants per template:
	- Animation = NotAnimated
	- Animation = Animated
	- Softening = NotSoftened
	- Softening = Softened: 	 sprites are scaled 2:1 horizontally, odd pixels l+r are set using blend  
	
	Other options:
	- ghostly:	  Shape can be rendered 50% transparent		
*/
template<Animation ANIM, Softening SOFT>
class SingleSprite;


template<Softening SOFT>
class SingleSprite<NotAnimated, SOFT> : public VideoPlane
{
public:
	using Shape = Video::Shape<SOFT>;
	using coord = Graphics::coord;
	using Point = Graphics::Point;

	SingleSprite(Shape, const Point& position);
	SingleSprite(Shape, coord x, coord y);

	virtual void setup(coord width) override;
	virtual void teardown() noexcept override {}
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* scanline) noexcept override;

	coord xpos;
	coord ypos;

	bool is_hot() const noexcept { return hot_shape.pixels != nullptr; }

	void move(coord x, coord y) noexcept { void(xpos = x), ypos = y; }
	void move(const Point& p) noexcept { move(p.x, p.y); }

	void modify(Shape s, coord x, coord y, bool wait_while_hot = 0) noexcept;
	void modify(Shape s, const Point& p, bool wait_while_hot = 0) noexcept;
	void replace(Shape s, bool wait_while_hot = 0) noexcept;

	void wait_while_hot() const noexcept;

protected:
	using HotShape = Video::Shape<SOFT>;

	Shape	 shape;		  // the compressed image of the sprite.
	HotShape hot_shape;	  // pointer into shape as rows are displayed
	int		 hot_shape_x; // updated x pos for current row
	bool	 ghostly;	  // render sprite semi transparent
	uint8	 frame_idx;	  // if animated
	int16	 countdown;	  // if animated
};


template<Softening SOFT>
class SingleSprite<Animated, SOFT> : public SingleSprite<NotAnimated, SOFT>
{
public:
	using Shape			= Video::Shape<SOFT>;
	using AnimatedShape = Video::AnimatedShape<SOFT>;
	using coord			= Graphics::coord;
	using Point			= Graphics::Point;
	using super			= SingleSprite<NotAnimated, SOFT>;

	SingleSprite(const AnimatedShape&, const Point& position);
	SingleSprite(const AnimatedShape&, coord x, coord y);

	virtual void setup(coord width) override;
	//virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	//virtual void renderScanline(int row, uint32* scanline) noexcept override;

	using super::countdown;
	using super::frame_idx;
	using super::is_hot;
	using super::move;
	using super::wait_while_hot;

	void modify(const AnimatedShape&, coord x, coord y, bool wait_while_hot = 0) noexcept;
	void modify(const AnimatedShape&, const Point& p, bool wait_while_hot = 0) noexcept;
	void replace(const AnimatedShape&, bool wait_while_hot = 0) noexcept;

private:
	AnimatedShape animated_shape;

	void next_frame() noexcept
	{
		lock _;
		if (++frame_idx >= animated_shape.num_frames) frame_idx = 0;
		countdown	 = animated_shape.durations[frame_idx];
		super::shape = animated_shape.frames[frame_idx];
	}

	struct lock
	{
		uint32 status_register;
		lock() noexcept { status_register = spin_lock_blocking(singlesprite_spinlock); }
		~lock() noexcept { spin_unlock(singlesprite_spinlock, status_register); }
	};
};


// ============================================================================


template<Softening SOFT>
void SingleSprite<NotAnimated, SOFT>::replace(Shape s, bool wait) noexcept
{
	shape = s;
	if (wait) wait_while_hot(); // => caller can delete old shape safely
}

template<Softening SOFT>
void SingleSprite<NotAnimated, SOFT>::modify(Shape s, const Point& new_p, bool wait) noexcept
{
	move(new_p);
	replace(s, wait);
}

template<Softening SOFT>
void SingleSprite<NotAnimated, SOFT>::modify(Shape s, coord new_x, coord new_y, bool wait) noexcept
{
	move(new_x, new_y);
	replace(s, wait);
}

// ============================================================================

template<Softening SOFT>
void SingleSprite<Animated, SOFT>::replace(const AnimatedShape& s, bool wait) noexcept
{
	{
		lock _;
		animated_shape = s;
		frame_idx	   = 0;
		countdown	   = s.durations[0];
		super::shape   = s.frames[0];
	}
	if (wait) wait_while_hot(); // => caller can delete old shape safely
}

template<Softening SOFT>
void SingleSprite<Animated, SOFT>::modify(const AnimatedShape& s, const Point& new_p, bool wait) noexcept
{
	move(new_p);
	replace(s, wait);
}

template<Softening SOFT>
void SingleSprite<Animated, SOFT>::modify(const AnimatedShape& s, coord new_x, coord new_y, bool wait) noexcept
{
	move(new_x, new_y);
	replace(s, wait);
}


// yes, we have them all:
extern template class SingleSprite<NotAnimated, NotSoftened>;
extern template class SingleSprite<NotAnimated, Softened>;
extern template class SingleSprite<Animated, NotSoftened>;
extern template class SingleSprite<Animated, Softened>;


} // namespace kio::Video
