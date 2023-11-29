// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Shape.h"
#include "Sprite.h"
#include "VideoPlane.h"
#include <pico/sync.h>


namespace kio::Video
{

extern bool hotlist_overflow; // set by add_to_hotlist()


/*	==============================================================================
	class Sprites is a VideoPlane which can be added to the VideoController
	to display sprites.
*/
template<typename Sprite, ZPlane WZ>
class Sprites : public VideoPlane
{
public:
	using Shape	   = typename Sprite::Shape;
	using HotShape = typename Shape::template HotShape<WZ>;

	Sprites() noexcept = default;
	virtual ~Sprites() noexcept override;

	virtual void setup(coord width) override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* scanline) noexcept override;

	Sprite* add(Sprite*) throws;
	Sprite* remove(Sprite*) noexcept;
	void	moveTo(Sprite*, const Point& p) noexcept;
	void	replace(Sprite*, const Shape& new_shape) noexcept;


	bool __always_inline is_in_displaylist(Sprite* s) const noexcept { return s->prev || displaylist == s; }
	void				 clear_displaylist(bool delete_sprites = false) noexcept;

private:
	void _unlink(Sprite*) noexcept;
	void _link_after(Sprite*, Sprite* other) noexcept;
	void _link_before(Sprite*, Sprite* other) noexcept;
	void _link(Sprite*) noexcept;
	void _move(Sprite*) noexcept;

	struct Lock
	{
		uint32 status_register;
		Lock() noexcept { status_register = spin_lock_blocking(sprites_spinlock); }
		~Lock() noexcept { spin_unlock(sprites_spinlock, status_register); }
	};

	Sprite* displaylist			 = nullptr;
	Sprite* volatile next_sprite = nullptr;
	HotShape* hotlist			 = nullptr;
	uint	  max_hot			 = 0;
	uint	  num_hot			 = 0;

	void add_to_hotlist(const Sprite*) noexcept;
};


// ============================================================================
// Implementations

template<typename Sprite, ZPlane WZ>
Sprites<Sprite, WZ>::~Sprites() noexcept
{
	assert(displaylist == nullptr); // else teardown not called => plane still in planes[] ?
	assert(hotlist == nullptr);
}


// ============================================================================

// yes, we have them all!
extern template class Sprites<Sprite<Shape<NotSoftened>>, NoZ>;
extern template class Sprites<Sprite<Shape<NotSoftened>>, HasZ>;
extern template class Sprites<Sprite<Shape<Softened>>, NoZ>;
extern template class Sprites<Sprite<Shape<Softened>>, HasZ>;
extern template class Sprites<Sprite<AnimatedShape<Shape<NotSoftened>>>, NoZ>;
extern template class Sprites<Sprite<AnimatedShape<Shape<NotSoftened>>>, HasZ>;
extern template class Sprites<Sprite<AnimatedShape<Shape<Softened>>>, NoZ>;
extern template class Sprites<Sprite<AnimatedShape<Shape<Softened>>>, HasZ>;


} // namespace kio::Video


/*


































*/
