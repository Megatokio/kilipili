// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "MousePointer.h"
#include "USBMouse.h"
#include "VideoBackend.h"
#include "VideoController.h"
#include "vga_types.h"
#include <memory>

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

constexpr uint8 transparent = 2;

#define b 0x0,
#define F 0x1,
#define _ transparent,

#define B(a, b, c, d)							 uint8((a << 0) + (a << 2) + (a << 4) + (a << 6))
#define X(a, b, c, d, e, f, g, h, i, j, k, l, m) B(a, b, c, d), B(e, f, g, h), B(i, j, k, l)
#define Y(a, b, c, d, e, f, g, h, i)			 B(a, b, c, d), B(e, f, g, h)
#define V(A)									 Y(A)
#define W(A)									 X(A)

// clang-format off

constexpr uint8 bitmap_pointerM[] =
{
	W( b _ _ _ _ _ _ _ _ _ _ _ 0 ),
	W( b b _ _ _ _ _ _ _ _ _ _ 0 ),
	W( b F b _ _ _ _ _ _ _ _ _ 0 ),
	W( b F F b _ _ _ _ _ _ _ _ 0 ),
	W( b F F F b _ _ _ _ _ _ _ 0 ),
	W( b F F F F b _ _ _ _ _ _ 0 ),
	W( b F F F F F b _ _ _ _ _ 0 ),
	W( b F F F F F F b _ _ _ _ 0 ),
	W( b F F F F F F F b _ _ _ 0 ),
	W( b F F F F F F F F b _ _ 0 ),
	W( b F F F F F b b b b b _ 0 ),
	W( b F F b F F b _ _ _ _ _ 0 ),
	W( b F b _ b F F b _ _ _ _ 0 ),
	W( b b _ _ b F F b _ _ _ _ 0 ),
	W( b _ _ _ _ b F F b _ _ _ 0 ),
	W( _ _ _ _ _ b F F b _ _ _ 0 ),
	W( _ _ _ _ _ _ b b _ _ _ _ 0 ),
};
__unused constexpr Pixmap<colormode_i2> pointer_m {11, 17, const_cast<uint8*>(bitmap_pointerM), 3};
__unused constexpr Point pointer_m_hot{1,2};

constexpr uint8 bitmap_pointerL[] =
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
__unused constexpr Point pointer_l_hot{1,1};

constexpr uint8 bitmap_crosshair[] =
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
__unused constexpr Point crosshair_hot{5,5};

constexpr uint8 bitmap_ibeam[] =
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
__unused constexpr Point ibeam_hot{3,9};

constexpr uint8 bitmap_busy1[] =
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
constexpr uint8 bitmap_busy2[] =
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
constexpr uint8 bitmap_busy3[] =
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
constexpr uint8 bitmap_busy4[] =
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
__unused constexpr Pixmap<colormode_i2> busy1{11, 11, const_cast<uint8*>(bitmap_busy1), 3};
__unused constexpr Pixmap<colormode_i2> busy2{11, 11, const_cast<uint8*>(bitmap_busy2), 3};
__unused constexpr Pixmap<colormode_i2> busy3{11, 11, const_cast<uint8*>(bitmap_busy3), 3};
__unused constexpr Pixmap<colormode_i2> busy4{11, 11, const_cast<uint8*>(bitmap_busy4), 3};
__unused constexpr Point busy_hot{5,5};

// clang-format on

#undef _
#undef b
#undef F
#undef V
#undef W
#undef X
#undef Y

constexpr const Pixmap<colormode_i2>* busys[4]	  = {&busy1, &busy2, &busy3, &busy4};
constexpr const Pixmap<colormode_i2>* pixmaps[4]  = {&pointer_l, &busy1, &crosshair, &ibeam};
constexpr Point						  hotspots[4] = {pointer_l_hot, busy_hot, crosshair_hot, ibeam_hot};
constexpr Color						  clut[4]	  = {white, black, 0, 0};


// ################################################################

template<Animation ANIM, Softening SOFT>
static typename MousePointer<ANIM, SOFT>::Shape shapeForID(MouseShapeID id);

template<>
Shape<NotSoftened> shapeForID<NotAnimated, NotSoftened>(MouseShapeID id)
{
	assert(uint(id) <= count_of(pixmaps));

	return Shape<NotSoftened>(*(pixmaps[id]), transparent, clut, hotspots[id]);
}

template<>
AnimatedShape<NotSoftened> shapeForID<Animated, NotSoftened>(MouseShapeID id)
{
	assert(uint(id) <= count_of(pixmaps));

	AnimatedShape<NotSoftened> shape;
	switch (id)
	{
	default:
		shape.num_frames   = 1;
		shape.frames	   = new Shape<NotSoftened>[1];
		shape.durations	   = new int16[1];
		shape.frames[0]	   = shapeForID<NotAnimated, NotSoftened>(id);
		shape.durations[0] = 0x7fff;
		break;
	case MOUSE_BUSY:
		shape.num_frames = 4;
		shape.frames	 = new Shape<NotSoftened>[4];
		shape.durations	 = new int16[4];
		for (uint i = 0; i < 4; i++)
		{
			shape.frames[i]	   = Shape<NotSoftened>(*(busys[i]), transparent, clut, busy_hot);
			shape.durations[i] = 15;
		}
		break;
	}
	return shape;
}

template<>
Shape<Softened> shapeForID<NotAnimated, Softened>(MouseShapeID)
{
	TODO(); //
}

template<>
AnimatedShape<Softened> shapeForID<Animated, Softened>(MouseShapeID)
{
	TODO(); //
}


template<Animation ANIM, Softening SOFT>
MousePointer<ANIM, SOFT>::MousePointer(MouseShapeID id, const Point& position) : // ctor
	SingleSprite<ANIM, SOFT>(shapeForID<ANIM, SOFT>(id), position)
{}

template<Animation ANIM, Softening SOFT>
void MousePointer<ANIM, SOFT>::teardown() noexcept
{}

template<Animation ANIM, Softening SOFT>
void MousePointer<ANIM, SOFT>::vblank() noexcept
{
	moveTo(getMousePosition());
}


// ################################################################


// the linker will know what we need:
template class MousePointer<NotAnimated, NotSoftened>;
template class MousePointer<NotAnimated, Softened>;
template class MousePointer<Animated, NotSoftened>;
template class MousePointer<Animated, Softened>;


} // namespace kio::Video