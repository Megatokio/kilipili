// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "AnimatedSprite.h"
#include "SingleSpritePlane.h"

namespace kio::Video
{

enum MousePointerID {
	MOUSE_POINTER,
	MOUSE_BUSY,
	MOUSE_CROSSHAIR,
	MOUSE_IBEAM,
	//MOUSE_SLIDER_H,
	//MOUSE_SLIDER_V,
	//MOUSE_SLIDER_X,
};


template<typename Sprite>
class MousePointer : public SingleSpritePlane<Sprite>
{
public:
	using super = SingleSpritePlane<Sprite>;
	using Shape = typename Sprite::Shape;
	static_assert(Shape::isa_shape);


	MousePointer(MousePointerID = MOUSE_POINTER);
	MousePointer(const Shape& s);
	~MousePointer() noexcept override = default;

	virtual void vblank() noexcept override;
	//virtual void renderScanline(int row, uint32* scanline) noexcept override;

	using super::getPosition;
	using super::replace;
	using super::setPosition;
};


// =========================================================================

// yes, we have them all!
extern template class MousePointer<Sprite<Shape>>;
extern template class MousePointer<Sprite<SoftenedShape>>;
extern template class MousePointer<AnimatedSprite<Shape>>;
extern template class MousePointer<AnimatedSprite<SoftenedShape>>;

} // namespace kio::Video


/*


































*/
