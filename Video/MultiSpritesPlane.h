// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AnimatedSprite.h"
#include "Shape.h"
#include "VideoPlane.h"
#include <pico/sync.h>


namespace kio::Video
{

extern bool hotlist_overflow; // set by add_to_hotlist()

enum ZPlane : bool { NoZ = 0, HasZ = 1 };


/*	==============================================================================
	class Sprites is a VideoPlane which can be added to the VideoController
	to display sprites.
	@tparam WZ with z coord ordering
*/
template<typename Sprite, ZPlane WZ>
class MultiSpritesPlane : public VideoPlane
{
public:
	using Shape = typename Sprite::Shape;


	template<typename HotShape, ZPlane>
	struct MyHotShape : public HotShape
	{};
	template<typename HotShape>
	struct MyHotShape<HotShape, HasZ> : public HotShape
	{
		uint z;
	};

	using HotShape = MyHotShape<typename Shape::HotShape, WZ>;

	MultiSpritesPlane() noexcept = default;
	virtual ~MultiSpritesPlane() noexcept override;

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
MultiSpritesPlane<Sprite, WZ>::~MultiSpritesPlane() noexcept
{
	assert(displaylist == nullptr); // else teardown not called => plane still in planes[] ?
	assert(hotlist == nullptr);
}


// ============================================================================

// yes, we have them all!
extern template class MultiSpritesPlane<Sprite<Shape>, NoZ>;
extern template class MultiSpritesPlane<Sprite<Shape>, HasZ>;
extern template class MultiSpritesPlane<Sprite<SoftenedShape>, NoZ>;
extern template class MultiSpritesPlane<Sprite<SoftenedShape>, HasZ>;
extern template class MultiSpritesPlane<AnimatedSprite<Shape>, NoZ>;
extern template class MultiSpritesPlane<AnimatedSprite<Shape>, HasZ>;
extern template class MultiSpritesPlane<AnimatedSprite<SoftenedShape>, NoZ>;
extern template class MultiSpritesPlane<AnimatedSprite<SoftenedShape>, HasZ>;


} // namespace kio::Video


/*


































*/
