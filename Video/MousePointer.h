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


template<Animation ANIM, Softening SOFT>
class MousePointer : public SingleSprite<ANIM, SOFT>
{
public:
	using super = SingleSprite<ANIM, SOFT>;
	using Shape = typename super::Shape;
	//using MouseEventHandler = USB::MouseEventHandler;

	MousePointer(MouseShapeID, const Point&);
	MousePointer(const Shape&, const Point&);
	~MousePointer() noexcept override;

	//virtual void setup(coord width) override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	//virtual void renderScanline(int row, uint32* scanline) noexcept override;

	using super::modify;
	using super::moveTo;
	using super::position;
	using super::replace;
};


// ========================== Implementations ===============================

template<Animation ANIM, Softening SOFT>
MousePointer<ANIM, SOFT>::MousePointer(const Shape& shape, const Point& position) : // ctor
	SingleSprite<ANIM, SOFT>(shape, position)
{}

template<>
MousePointer<NotAnimated, NotSoftened>::~MousePointer() noexcept // dtor
{
	delete[] shape.pixels;
}

template<>
MousePointer<NotAnimated, Softened>::~MousePointer() noexcept // dtor
{
	delete[] shape.pixels;
}

template<>
MousePointer<Animated, NotSoftened>::~MousePointer() noexcept // dtor
{
	animated_shape.teardown();
}

template<>
MousePointer<Animated, Softened>::~MousePointer() noexcept // dtor
{
	animated_shape.teardown();
}


// yes, we have them all!
extern template class MousePointer<NotAnimated, NotSoftened>;
extern template class MousePointer<NotAnimated, Softened>;
extern template class MousePointer<Animated, NotSoftened>;
extern template class MousePointer<Animated, Softened>;

} // namespace kio::Video


/*


































*/
