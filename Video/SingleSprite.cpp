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


// ============================================================================
// NotAnimated:

template<Softening SOFT>
SingleSprite<NotAnimated, SOFT>::SingleSprite(Shape shape, coord x, coord y) : // ctor
	xpos(x),
	ypos(y),
	shape(shape),
	hot_shape(nullptr),
	ghostly(false),
	frame_idx(0),
	countdown(0)
{}

template<Softening SOFT>
SingleSprite<NotAnimated, SOFT>::SingleSprite(Shape shape, const Point& p) : // ctor
	SingleSprite(shape, p.x, p.y)
{}

template<Softening SOFT>
void SingleSprite<NotAnimated, SOFT>::setup(coord __unused width)
{
	hot_shape = nullptr;
	ghostly	  = false;
}

template<Softening SOFT>
void RAM SingleSprite<NotAnimated, SOFT>::vblank() noexcept
{
	hot_shape = nullptr;

	Shape s = shape;
	int	  y = ypos - s.preamble().hot_y;
	if (y >= 0) return;

	// sprite starts above screen:

	hot_shape_x = xpos - s.preamble().hot_x;
	s.skip_preamble();
	for (; y < 0; y++)
	{
		assert(s.is_pfx());
		s.skip_row(hot_shape_x);
		if unlikely (s.is_end()) return;
	}

	assert(s.is_pfx());
	hot_shape = s;
}

template<Softening SOFT>
void RAM SingleSprite<NotAnimated, SOFT>::renderScanline(int row, uint32* scanline) noexcept
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

template<Softening SOFT>
void SingleSprite<NotAnimated, SOFT>::wait_while_hot() const noexcept
{
	for (uint i = 0; i < 100000 / 60 && is_hot(); i++) { sleep_us(10); }
}


// ============================================================================
// Animated:

template<Softening SOFT>
SingleSprite<Animated, SOFT>::SingleSprite(const AnimatedShape& shape, coord x, coord y) : // ctor
	super(shape.frames[0], x, y),
	animated_shape(shape)
{
	if (!singlesprite_spinlock) singlesprite_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
}

template<Softening SOFT>
SingleSprite<Animated, SOFT>::SingleSprite(const AnimatedShape& shape, const Point& p) : // ctor
	SingleSprite(shape, p.x, p.y)
{}

template<Softening SOFT>
void SingleSprite<Animated, SOFT>::setup(coord width)
{
	super::setup(width);
	frame_idx	 = 0;
	countdown	 = animated_shape.durations[0];
	super::shape = animated_shape.frames[0];
}

template<Softening SOFT>
void RAM SingleSprite<Animated, SOFT>::vblank() noexcept
{
	if (--countdown <= 0) next_frame();
	super::vblank();
}


// the linker will know what we need:
template class SingleSprite<NotAnimated, NotSoftened>;
template class SingleSprite<NotAnimated, Softened>;
template class SingleSprite<Animated, NotSoftened>;
template class SingleSprite<Animated, Softened>;


} // namespace kio::Video


/*









































*/
