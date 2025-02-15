// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Frames.h"
#include "Shape.h"
#include "Sprite.h"
#include "VideoPlane.h"

#define XRAM __attribute__((section(".scratch_x.SSP" __XSTRING(__LINE__))))		// the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.SSP" __XSTRING(__LINE__)))) // general ram


namespace kio::Video
{

/*
	A VideoPlane for one single Sprite.
	Intended for mouse pointer or player character.
	
	Variants per template:
	- Animation = NotAnimated
	- Animation = Animated
	
	Other options:
	- ghostly:	  Shape can be rendered 50% transparent		
*/
template<typename Sprite>
class SingleSpritePlane : public VideoPlane
{
public:
	using Shape	   = typename Sprite::Shape;
	using HotShape = typename Shape::HotShape;
	using Frames   = Video::Frames<Shape>;
	using Frame	   = Video::Frame<Shape>;

	static constexpr bool is_animated = Sprite::is_animated;

	SingleSpritePlane(const Shape& s, const Point& position);
	SingleSpritePlane(Frames&& frames, const Point& position);

	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, int width, uint32* scanline) noexcept override;

	Sprite sprite;

	Point getPosition() const noexcept { return sprite.get_position(); }
	void  setPosition(const Point& p) noexcept { sprite.set_position(p); }
	void  moveTo(const Point& p) noexcept { sprite.set_position(p); }
	void  replace(const Shape& s) noexcept;
	void  replace(const Frames&) throws;					   // for AnimatedSprite
	void  replace(Frames&&) noexcept;						   // for AnimatedSprite
	void  replace(const Frame*, uint) throws;				   // for AnimatedSprite
	void  replace(const Shape* s, const uint16*, uint) throws; // for AnimatedSprite
	void  replace(const Shape* s, uint16, uint) throws;		   // for AnimatedSprite

protected:
	HotShape hot_shape;
	bool	 is_hot = false;
	char	 _padding[3];

	void next_frame() noexcept
	{
		if constexpr (is_animated)
		{
			Lock _;
			sprite.next_frame();
		}
	}

	struct Lock
	{
		uint32 sreg;
		Lock() noexcept
		{
			if constexpr (is_animated) sreg = spin_lock_blocking(sprites_spinlock);
		}
		~Lock() noexcept
		{
			if constexpr (is_animated) spin_unlock(sprites_spinlock, sreg);
		}
	};
};


// ============================================================================


template<typename Sprite>
SingleSpritePlane<Sprite>::SingleSpritePlane(const Shape& s, const Point& position) : // ctor
	sprite(s, position)
{
	if constexpr (is_animated)
	{
		if (!sprites_spinlock) sprites_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
		sprite.current_frame = 0xff; //
		sprite.next_frame();		 // needed?
	}
}

template<typename Sprite>
SingleSpritePlane<Sprite>::SingleSpritePlane(Frames&& frames, const Point& position) : // ctor
	sprite(std::move(frames), position)
{
	if constexpr (is_animated)
	{
		if (!sprites_spinlock) sprites_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
		sprite.current_frame = 0xff; //
		sprite.next_frame();		 // needed?
	}
}

template<typename Sprite>
void SingleSpritePlane<Sprite>::replace(const Shape& shape) noexcept
{
	sprite.replace(shape);
}

template<typename Sprite>
void SingleSpritePlane<Sprite>::replace(const Frames& frames) throws
{
	if constexpr (is_animated) sprite.replace(frames);
	else sprite.replace(frames[0].shape);
}

template<typename Sprite>
void SingleSpritePlane<Sprite>::replace(Frames&& frames) noexcept
{
	if constexpr (is_animated) sprite.replace(std::move(frames));
	else sprite.replace(frames[0].shape);
}

template<typename Sprite>
void SingleSpritePlane<Sprite>::replace(const Frame* frames, uint cnt) throws
{
	if constexpr (is_animated) sprite.replace(frames, cnt);
	else sprite.replace(frames[0].shape);
}

template<typename Sprite>
void SingleSpritePlane<Sprite>::replace(const Shape* shapes, uint16 dur, uint cnt) throws
{
	if constexpr (is_animated) sprite.replace(shapes, dur, cnt);
	else sprite.replace(shapes[0]);
}

template<typename Sprite>
void SingleSpritePlane<Sprite>::replace(const Shape* shapes, const uint16* dur, uint cnt) throws
{
	if constexpr (is_animated) sprite.replace(shapes, dur, cnt);
	else sprite.replace(shapes[0]);
}


// ============================================================================

template<typename Sprite>
void /*RAM*/ SingleSpritePlane<Sprite>::SingleSpritePlane::vblank() noexcept
{
	if constexpr (is_animated)
	{
		if (--sprite.countdown <= 0) next_frame();
	}

	is_hot = false;

	int y = sprite.pos.y;
	if (y >= 0) return;

	// sprite starts above screen:

	sprite.start(hot_shape);
	do is_hot = !hot_shape.skip_row();
	while (++y < 0 && is_hot);
}

template<typename Sprite>
void RAM SingleSpritePlane<Sprite>::renderScanline(int row, int __unused width, uint32* scanline) noexcept
{
	if (!is_hot)
	{
		if (row != sprite.pos.y) return;
		sprite.start(hot_shape);
	}

	bool finished = hot_shape.render_row(reinterpret_cast<Color*>(scanline));
	is_hot		  = !finished;
}


} // namespace kio::Video

#undef RAM
#undef XRAM


/*






































*/
