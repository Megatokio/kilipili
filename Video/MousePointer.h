// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "SingleSprite.h"


namespace kio::Video
{

enum MouseShapeID {
	MOUSE_POINTER,
	MOUSE_BUSY,
	MOUSE_CROSSHAIR,
	MOUSE_IBEAM,
	//MOUSE_SLIDER_H,
	//MOUSE_SLIDER_V,
	//MOUSE_SLIDER_X,
};


template<typename Sprite>
class MousePointer : public SingleSprite<Sprite>
{
public:
	using super = SingleSprite<Sprite>;
	using Shape = typename super::Shape;

	MousePointer(MouseShapeID, const Point&);
	MousePointer(const Shape& s, const Point& p) : super(s, p) {}
	~MousePointer() noexcept override = default;

	//virtual void setup(coord width) override;
	//virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	//virtual void renderScanline(int row, uint32* scanline) noexcept override;

	using super::getPosition;
	using super::modify;
	using super::replace;
	using super::setPosition;
};


// =========================================================================

// yes, we have them all!
extern template class MousePointer<Sprite<Shape<NotSoftened>>>;
extern template class MousePointer<Sprite<Shape<Softened>>>;
extern template class MousePointer<Sprite<AnimatedShape<Shape<NotSoftened>>>>;
extern template class MousePointer<Sprite<AnimatedShape<Shape<Softened>>>>;

} // namespace kio::Video


/*


































*/
