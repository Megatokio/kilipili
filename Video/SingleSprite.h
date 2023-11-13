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
class SingleSprite : public VideoPlane
{
	using Shape = Video::Shape<ANIM, SOFT>;
	using coord = Graphics::coord;
	using Point = Graphics::Point;

	SingleSprite(const Shape, const Point& position);
	SingleSprite(const Shape, coord x, coord y);

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

private:
	Shape shape; // the compressed image of the sprite.

	Shape hot_shape;	   // pointer into shape as rows are displayed
	int	  hot_shape_x;	   // updated x pos for current row
	int16 frame_countdown; // animation
	bool  ghostly;		   // render sprite semi transparent
	char  _padding = 0;

	void wait_while_hot() const noexcept;

	struct lock
	{
		uint32 status_register;
		lock() noexcept { status_register = spin_lock_blocking(singlesprite_spinlock); }
		~lock() noexcept { spin_unlock(singlesprite_spinlock, status_register); }
	};
};


// ============================================================================


template<Animation ANIM, Softening SOFT>
void SingleSprite<ANIM, SOFT>::replace(Shape s, bool wait) noexcept
{
	if constexpr (ANIM == NotAnimated) { shape = s; }
	else
	{
		lock _;
		shape			= s;
		frame_countdown = s.duration();
	}
	if (wait) wait_while_hot(); // => caller can delete old shape safely
}

template<Animation ANIM, Softening SOFT>
void SingleSprite<ANIM, SOFT>::modify(Shape s, const Point& new_p, bool wait) noexcept
{
	move(new_p);
	replace(s, wait);
}

template<Animation ANIM, Softening SOFT>
void SingleSprite<ANIM, SOFT>::modify(Shape s, coord new_x, coord new_y, bool wait) noexcept
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
