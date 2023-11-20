// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Shape.h"
#include "Sprite.h"
#include "VideoPlane.h"


namespace kio::Video
{

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
template<typename Sprite>
class SingleSprite : public VideoPlane
{
public:
	using Shape	   = typename Sprite::Shape;
	using HotShape = typename Video::HotShape<typename Sprite::Shape, NoZ>;

	static constexpr bool animated = Sprite::animated;

	SingleSprite(Shape s, const Point& position);

	virtual void setup(coord width) override;
	virtual void teardown() noexcept override {}
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* scanline) noexcept override;

	Sprite	 sprite;
	HotShape hot_shape {nullptr, 0};

	bool is_hot() const noexcept { return hot_shape.pixels != nullptr; }
	void wait_while_hot() const noexcept;

	void  moveTo(const Point& p) noexcept;
	void  modify(Shape s, const Point& p, bool wait_while_hot = 0) noexcept;
	void  replace(Shape s, bool wait_while_hot = 0) noexcept;
	Point getPosition() const noexcept { return sprite.get_position(); }
	void  setPosition(const Point& p) noexcept { moveTo(p); }


protected:
	void next_frame() noexcept
	{
		if constexpr (animated)
		{
			Lock _;
			sprite.next_frame();
		}
	}

	struct Lock
	{
		uint32 status_register;
		Lock() noexcept { status_register = spin_lock_blocking(sprites_spinlock); }
		~Lock() noexcept { spin_unlock(sprites_spinlock, status_register); }
	};
};


// ============================================================================


template<typename Sprite>
SingleSprite<Sprite>::SingleSprite(Shape s, const Point& position) : // ctor
	sprite(std::move(s), position)
{
	if constexpr (animated)
		if (!sprites_spinlock) sprites_spinlock = spin_lock_init(uint(spin_lock_claim_unused(true)));
}


template<typename Sprite>
void SingleSprite<Sprite>::moveTo(const Point& p) noexcept
{
	// no re-linking needed as there is only one sprite:
	sprite.set_position(p);
}

template<typename Sprite>
void SingleSprite<Sprite>::replace(Shape s, bool wait) noexcept
{
	if constexpr (Sprite::animated) //
	{
		Lock _;
		sprite.replace(std::move(s));
	}
	else { sprite.replace(std::move(s)); }

	if (wait) wait_while_hot(); // => caller can delete old shape safely
}

template<typename Sprite>
void SingleSprite<Sprite>::SingleSprite::modify(Shape s, const Point& new_p, bool wait) noexcept
{
	moveTo(new_p);
	replace(std::move(s), wait);
}

template<typename Sprite>
void SingleSprite<Sprite>::SingleSprite::wait_while_hot() const noexcept
{
	for (uint i = 0; i < 100000 / 60 && is_hot(); i++) { sleep_us(10); }
}

// ============================================================================

// yes, we have them all:
extern template class SingleSprite<Sprite<Shape<NotSoftened>>>;
extern template class SingleSprite<Sprite<Shape<Softened>>>;
extern template class SingleSprite<Sprite<AnimatedShape<Shape<NotSoftened>>>>;
extern template class SingleSprite<Sprite<AnimatedShape<Shape<Softened>>>>;


} // namespace kio::Video


/*






































*/
