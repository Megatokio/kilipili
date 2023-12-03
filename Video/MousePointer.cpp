// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "MousePointer.h"
#include "AnimatedSprite.h"
#include "USBMouse.h"
#include "VideoBackend.h"

// all hot video code should go into ram to allow video while flashing.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.mouse" XWRAP(__LINE__))))	  // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.mouse" XWRAP(__LINE__)))) // general ram


// =============================================================


namespace kio::Video
{

using namespace Graphics;

// =============================================================
//		MousePointer
// =============================================================

static constexpr uint8 transparent = 2;

#define b 0x0,
#define F 0x1,
#define _ transparent,

#define B(a, b, c, d)							 uint8((a << 0) + (b << 2) + (c << 4) + (d << 6))
#define X(a, b, c, d, e, f, g, h, i, j, k, l, m) B(a, b, c, d), B(e, f, g, h), B(i, j, k, l)
#define Y(a, b, c, d, e, f, g, h, i)			 B(a, b, c, d), B(e, f, g, h)
#define V(A)									 Y(A)
#define W(A)									 X(A)

// clang-format off

static constexpr uint8 bitmap_pointerM[] =
{
	W(b _ _ _ _ _ _ _ _ _ _ _ 0),
	W(b b _ _ _ _ _ _ _ _ _ _ 0),
	W(b F b _ _ _ _ _ _ _ _ _ 0),
	W(b F F b _ _ _ _ _ _ _ _ 0),
	W(b F F F b _ _ _ _ _ _ _ 0),
	W(b F F F F b _ _ _ _ _ _ 0),
	W(b F F F F F b _ _ _ _ _ 0),
	W(b F F F F F F b _ _ _ _ 0),
	W(b F F F F F F F b _ _ _ 0),
	W(b F F F F F F F F b _ _ 0),
	W(b F F F F F b b b b b _ 0),
	W(b F F b F F b _ _ _ _ _ 0),
	W(b F b _ b F F b _ _ _ _ 0),
	W(b b _ _ b F F b _ _ _ _ 0),
	W(b _ _ _ _ b F F b _ _ _ 0),
	W(_ _ _ _ _ b F F b _ _ _ 0),
	W(_ _ _ _ _ _ b b _ _ _ _ 0),
};
__unused constexpr Pixmap<colormode_i2> pointer_m {11, 17, const_cast<uint8*>(bitmap_pointerM), 3};
__unused constexpr Dist pointer_m_hot{1,2};

static constexpr uint8 bitmap_pointerL[] =
{
	W(b b _ _ _ _ _ _ _ _ _ _ 0),
	W(b F b _ _ _ _ _ _ _ _ _ 0),
	W(b F F b _ _ _ _ _ _ _ _ 0),
	W(b F F F b _ _ _ _ _ _ _ 0),
	W(b F F F F b _ _ _ _ _ _ 0),
	W(b F F F F F b _ _ _ _ _ 0),
	W(b F F F F F F b _ _ _ _ 0),
	W(b F F F F F F F b _ _ _ 0),
	W(b F F F F F F F F b _ _ 0),
	W(b F F F F F F F F F b _ 0),
	W(b F F F F F F F F F F b 0),
	W(b F F F F F F b b b b b 0),
	W(b F F F b F F b _ _ _ _ 0),
	W(b F F b _ b F F b _ _ _ 0),
	W(b F b _ _ b F F b _ _ _ 0),
	W(b b _ _ _ _ b F F b _ _ 0),
	W(_ _ _ _ _ _ b F F b _ _ 0),
	W(_ _ _ _ _ _ _ b b _ _ _ 0),
};
__unused constexpr Pixmap<colormode_i2> pointer_l {12, 18, const_cast<uint8*>(bitmap_pointerL), 3};
__unused constexpr Dist pointer_l_hot{1,1};

static constexpr uint8 bitmap_crosshair[] =
{
	W(_ _ _ _ b F b _ _ _ _ _ 0),
	W(_ _ _ _ b F b _ _ _ _ _ 0),
	W(_ _ _ _ b F b _ _ _ _ _ 0),
	W(_ _ _ _ b F b _ _ _ _ _ 0),
	W(b b b b b _ b b b b b _ 0),
	W(F F F F _ F _ F F F F _ 0),
	W(b b b b b _ b b b b b _ 0),
	W(_ _ _ _ b F b _ _ _ _ _ 0),
	W(_ _ _ _ b F b _ _ _ _ _ 0),
	W(_ _ _ _ b F b _ _ _ _ _ 0),
	W(_ _ _ _ b F b _ _ _ _ _ 0),
};
__unused constexpr Pixmap<colormode_i2> crosshair {11, 11, const_cast<uint8*>(bitmap_crosshair), 3};
__unused constexpr Dist crosshair_hot{5,5};

static constexpr uint8 bitmap_ibeam[] =
{
	V(b b b _ b b b _ 0),
	V(b F F b F F b _ 0),
	V(b b b F b b b _ 0),
	V(_ _ b F b _ _ _ 0),
	V(_ _ b F b _ _ _ 0),
	V(_ _ b F b _ _ _ 0),
	V(_ _ b F b _ _ _ 0),
	V(_ _ b F b _ _ _ 0),
	V(_ _ b F b _ _ _ 0),
	V(b b b F b b b _ 0),
	V(b F F b F F b _ 0),
	V(b b b _ b b b _ 0),
};
__unused constexpr Pixmap<colormode_i2> ibeam {7, 12, const_cast<uint8*>(bitmap_ibeam), 2};
__unused constexpr Dist ibeam_hot{3,9};

static constexpr uint8 bitmap_busy1[] =
{
	W(_ _ _ _ F F F _ _ _ _ _ 0),
	W(_ _ F F b b F F F _ _ _ 0),
	W(_ F b b b b F F F F _ _ 0),
	W(_ F b b b b F F F F _ _ 0),
	W(F b b b b b F F F F F _ 0),
	W(F b b b b b b b b b F _ 0),
	W(F F F F F b b b b b F _ 0),
	W(_ F F F F b b b b F _ _ 0),
	W(_ F F F F b b b b F _ _ 0),
	W(_ _ F F F b b F F _ _ _ 0),
	W(_ _ _ _ F F F _ _ _ _ _ 0),
};
static constexpr uint8 bitmap_busy2[] =
{
	W(_ _ _ _ F F F _ _ _ _ _ 0),
	W(_ _ F F b b b F F _ _ _ 0),
	W(_ F b b b b b b b F _ _ 0),
	W(_ F F b b b b b F F _ _ 0),
	W(F F F F b b b F F F F _ 0),
	W(F F F F F b F F F F F _ 0),
	W(F F F F b b b F F F F _ 0),
	W(_ F F b b b b b F F _ _ 0),
	W(_ F b b b b b b b F _ _ 0),
	W(_ _ F F b b b F F _ _ _ 0),
	W(_ _ _ _ F F F _ _ _ _ _ 0),
};
static constexpr uint8 bitmap_busy3[] =
{
	W(_ _ _ _ F F F _ _ _ _ _ 0),
	W(_ _ F F F b b F F _ _ _ 0),
	W(_ F F F F b b b b F _ _ 0),
	W(_ F F F F b b b b F _ _ 0),
	W(F F F F F b b b b b F _ 0),
	W(F b b b b b b b b b F _ 0),
	W(F b b b b b F F F F F _ 0),
	W(_ F b b b b F F F F _ _ 0),
	W(_ F b b b b F F F F _ _ 0),
	W(_ _ F F b b F F F _ _ _ 0),
	W(_ _ _ _ F F F _ _ _ _ _ 0),
};
static constexpr uint8 bitmap_busy4[] =
{
	W(_ _ _ _ F F F _ _ _ _ _ 0),
	W(_ _ F F F F F F F _ _ _ 0),
	W(_ F b F F F F F b F _ _ 0),
	W(_ F b b F F F b b F _ _ 0),
	W(F b b b b F b b b b F _ 0),
	W(F b b b b b b b b b F _ 0),
	W(F b b b b F b b b b F _ 0),
	W(_ F b b F F F b b F _ _ 0),
	W(_ F b F F F F F b F _ _ 0),
	W(_ _ F F F F F F F _ _ _ 0),
	W(_ _ _ _ F F F _ _ _ _ _ 0),
};
static constexpr Pixmap<colormode_i2> busy1{11, 11, const_cast<uint8*>(bitmap_busy1), 3};
static constexpr Pixmap<colormode_i2> busy2{11, 11, const_cast<uint8*>(bitmap_busy2), 3};
static constexpr Pixmap<colormode_i2> busy3{11, 11, const_cast<uint8*>(bitmap_busy3), 3};
static constexpr Pixmap<colormode_i2> busy4{11, 11, const_cast<uint8*>(bitmap_busy4), 3};
static constexpr Dist busy_hot{5,5};

// clang-format on

#undef _
#undef b
#undef F
#undef V
#undef W
#undef X
#undef Y

static constexpr const Pixmap<colormode_i2>* busys[4]	 = {&busy1, &busy2, &busy3, &busy4};
static constexpr const Pixmap<colormode_i2>* pixmaps[4]	 = {&pointer_l, &busy1, &crosshair, &ibeam};
static constexpr Dist						 hotspots[4] = {pointer_l_hot, busy_hot, crosshair_hot, ibeam_hot};
static constexpr Color						 clut[4]	 = {white, black, 0, 0};


// ################################################################


// helper to create a Shape for a MouseShapeID:
Shape shape_for_id(MousePointerID id)
{
	assert(uint(id) <= count_of(pixmaps));
	return Shape(*(pixmaps[id]), transparent, hotspots[id], clut);
}

template<typename Sprite>
MousePointer<Sprite>::MousePointer(MousePointerID id, const Point& position) : // ctor
	super(shape_for_id(id), position)
{
	if constexpr (Sprite::is_animated)
		if (id == MOUSE_BUSY)
		{
			Shape shapes[4];
			for (uint i = 0; i < 4; i++) shapes[i] = Shape(*(busys[i]), transparent, busy_hot, clut);
			replace(shapes, 6, 4);
		}
}

template<typename Shape>
void MousePointer<Shape>::vblank() noexcept
{
	setPosition(USB::getMousePosition());
	super::vblank();
}

template<typename Sprite>
void MousePointer<Sprite>::setup(coord width)
{
	USB::setScreenSize(width, screen_height());
}


// ################################################################


// the linker will know what we need:
template class MousePointer<Sprite<Shape>>;
//template class MousePointer<Sprite<SoftenedShape>>;
template class MousePointer<AnimatedSprite<Shape>>;
//template class MousePointer<AnimatedSprite<SoftenedShape>>;


} // namespace kio::Video
