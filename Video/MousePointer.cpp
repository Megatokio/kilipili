// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "MousePointer.h"
#include "AnimatedSprite.h"

// all hot video code should go into ram to allow video while flashing.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define XRAM __attribute__((section(".scratch_x.MP" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.MP" __XSTRING(__LINE__)))) // general ram


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

static constexpr Color clut[4] = {white, black, 0, 0};

// clang-format off

__unused static Pixmap<colormode_i2>* pointer_m(Dist* hotspot)
{
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
	*hotspot = Dist {1,2};
	return new Pixmap<colormode_i2> {11, 17, const_cast<uint8*>(bitmap_pointerM), 3};
}

__unused static Pixmap<colormode_i2>* pointer_l(Dist* hotspot)
{
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
	*hotspot = Dist(1,1);
	return new Pixmap<colormode_i2> {12, 18, const_cast<uint8*>(bitmap_pointerL), 3};
}

__unused Pixmap<colormode_i2>* crosshair (Dist* hotspot)
{
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
	*hotspot = Dist(5,5);
	return new Pixmap<colormode_i2> {11, 11, const_cast<uint8*>(bitmap_crosshair), 3};
}

__unused Pixmap<colormode_i2> *ibeam(Dist* hotspot)
{
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
	*hotspot = Dist {3,9};
	return new Pixmap<colormode_i2>  {7, 12, const_cast<uint8*>(bitmap_ibeam), 2};
}

static Pixmap<colormode_i2> *busy(uint i,Dist* hotspot)
{
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
	*hotspot = Dist(5,5);
	static constexpr const uint8* bitmap_busy[4] = {bitmap_busy1, bitmap_busy2, bitmap_busy3, bitmap_busy4};
	return new Pixmap<colormode_i2> {11, 11, const_cast<uint8*>(bitmap_busy[i]), 3};
};

// clang-format on

#undef _
#undef b
#undef F
#undef V
#undef W
#undef X
#undef Y


// ################################################################


Shape shape_for_id(MousePointerID id)
{
	// helper to create a Shape for a MouseShapeID

	Dist				  hotspot;
	Pixmap<colormode_i2>* pixmap;

	switch (uint(id))
	{
	case MOUSE_BUSY: pixmap = busy(0, &hotspot); break;
	case MOUSE_CROSSHAIR: pixmap = crosshair(&hotspot); break;
	case MOUSE_IBEAM: pixmap = ibeam(&hotspot); break;
	default:
	case MOUSE_POINTER: pixmap = pointer_m(&hotspot); break;
	}

	RCPtr<RCObject> rcpixmap = pixmap;
	return Shape(*pixmap, transparent, hotspot, clut);
}

template<typename Sprite>
MousePointer<Sprite>::MousePointer(MousePointerID id) : //
	super(shape_for_id(id), USB::getMousePosition())
{
	if constexpr (Sprite::is_animated)
		if (id == MOUSE_BUSY)
		{
			Shape shapes[4];
			for (uint i = 0; i < 4; i++)
			{
				Dist						hotspot;
				RCPtr<Pixmap<colormode_i2>> pixmap = busy(i, &hotspot);
				shapes[i]						   = Shape(*pixmap, transparent, hotspot, clut);
			}
			replace(shapes, 6, 4);
		}
}

template<typename Sprite>
MousePointer<Sprite>::MousePointer(const Shape& s) : //
	super(s, USB::getMousePosition())
{}

template<typename Shape>
void MousePointer<Shape>::vblank() noexcept
{
	setPosition(USB::getMousePosition());
	super::vblank();
}


// ################################################################


// the linker will know what we need:
template class MousePointer<Sprite<Shape>>;
//template class MousePointer<Sprite<SoftenedShape>>;
template class MousePointer<AnimatedSprite<Shape>>;
//template class MousePointer<AnimatedSprite<SoftenedShape>>;


} // namespace kio::Video
