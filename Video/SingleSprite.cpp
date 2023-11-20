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


#define lock_display_list() SingleSpriteLocker _locklist


// ============================================================================

template<typename Sprite>
void SingleSprite<Sprite>::SingleSprite::setup(coord __unused width)
{
	hot_shape.pixels = nullptr;

	if constexpr (animated)
	{
		sprite.frame_idx = 0xff;
		sprite.next_frame();
	}
}

template<typename Sprite>
void RAM SingleSprite<Sprite>::SingleSprite::vblank() noexcept
{
	if constexpr (animated) //
	{
		if (--sprite.countdown <= 0) next_frame();
	}

	hot_shape.pixels = nullptr;

	int y = sprite.pos_y;
	if (y >= 0) return;

	// sprite starts above screen:

	hot_shape.x		 = sprite.pos_x;
	hot_shape.pixels = &sprite.shape.pixels[0u];

	for (; y < 0; y++)
	{
		hot_shape.skip_row();
		if unlikely (hot_shape.is_end()) return;
	}

	assert(hot_shape.is_pfx());
}

template<typename Sprite>
void RAM SingleSprite<Sprite>::renderScanline(int row, uint32* scanline) noexcept
{
	if (hot_shape.pixels == nullptr)
	{
		if (row != sprite.pos_y) return;
		hot_shape.x = sprite.pos_x;
		assert(hot_shape.is_pfx());
		hot_shape.pixels  = &sprite.shape.pixels[0u];
		hot_shape.ghostly = sprite.ghostly;
	}

	bool finished = hot_shape.render_row(reinterpret_cast<Color*>(scanline));
	if (finished) hot_shape.pixels = nullptr;
}


// the linker will know what we need:
template class SingleSprite<Sprite<Shape<NotSoftened>>>;
template class SingleSprite<Sprite<Shape<Softened>>>;
template class SingleSprite<Sprite<AnimatedShape<Shape<NotSoftened>>>>;
template class SingleSprite<Sprite<AnimatedShape<Shape<Softened>>>>;


struct ShapeFoo
{
	Color*				  pixels;
	static constexpr bool isa_shape = true;
};

struct SpriteFoo
{
	using Shape					   = ShapeFoo;
	static constexpr bool animated = false;
	coord				  pos_x, pos_y;
	ShapeFoo			  shape;
	bool				  ghostly;

	SpriteFoo(ShapeFoo, Point);
};

void testFoo() { SingleSprite<SpriteFoo> foo {ShapeFoo(), Point()}; }


} // namespace kio::Video


/*









































*/
