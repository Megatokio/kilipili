// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "SingleSprite.h"
#include <pico/stdlib.h>
#include <pico/sync.h>

namespace kio::Video
{
using namespace Graphics;

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.spr" XWRAP(__LINE__))))		// the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.spr" XWRAP(__LINE__)))) // general ram


spin_lock_t* singlesprite_spinlock = nullptr;


#define lock_display_list() SingleSpriteLocker _locklist


template<Animation ANIM, Softening SOFT>
SingleSprite<ANIM, SOFT>::SingleSprite(Shape&& shape, coord x, coord y) :
	xpos(x),
	ypos(y),
	shape(std::move(shape)),
	hot_shape(nullptr),
	frame_countdown(),
	ghostly(false)
{
	if constexpr (ANIM == Animated)
		if (!singlesprite_spinlock) singlesprite_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
}

template<Animation ANIM, Softening SOFT>
SingleSprite<ANIM, SOFT>::SingleSprite(Shape&& shape, const Point& p) : SingleSprite(std::move(shape), p.x, p.y)
{}

template<Animation ANIM, Softening SOFT>
void SingleSprite<ANIM, SOFT>::setup(coord __unused width)
{
	hot_shape		= nullptr;
	frame_countdown = shape.duration();
}

template<Animation ANIM, Softening SOFT>
void RAM SingleSprite<ANIM, SOFT>::vblank() noexcept
{
	hot_shape = nullptr;

	Shape hs;

	if constexpr (ANIM == NotAnimated) { hs = shape; }
	else
	{
		lock _;

		hs = shape;
		if (--frame_countdown <= 0)
		{
			shape = hs		= hs.next_frame();
			frame_countdown = hs.duration() - 1;
		}
	}

	int y = ypos - hs.preamble().hot_y;
	if (y >= 0) return;

	hot_shape_x = xpos - hs.preamble().hot_x;
	hs.skip_preamble();
	for (; y < 0; y++) // if sprite starts above screen:
	{
		assert(hs.is_pfx());
		hs.skip_row(hot_shape_x);
		if unlikely (hs.is_end()) return;
	}
	assert(hs.is_pfx());
	hot_shape = hs;
}

template<Animation ANIM, Softening SOFT>
void RAM SingleSprite<ANIM, SOFT>::renderScanline(int row, uint32* scanline) noexcept
{
	if (hot_shape.pixels == nullptr)
	{
		Shape s = shape;
		if (row != ypos - s.preamble().hot_y) return;
		hot_shape_x = xpos - s.preamble().hot_x;
		s.skip_preamble();
		assert(s.is_pfx());
		hot_shape = s;
	}

	bool finished = hot_shape.render_row(hot_shape_x, reinterpret_cast<Color*>(scanline), ghostly);
	if (finished) hot_shape = nullptr;
}

template<Animation ANIM, Softening SOFT>
void SingleSprite<ANIM, SOFT>::wait_while_hot() const noexcept
{
	for (uint i = 0; i < 100000 / 60 && is_hot(); i++) { sleep_us(10); }
}


// the linker will know what we need:
template class SingleSprite<NotAnimated, NotSoftened>;
template class SingleSprite<NotAnimated, Softened>;
template class SingleSprite<Animated, NotSoftened>;
template class SingleSprite<Animated, Softened>;


} // namespace kio::Video


/*









































*/
