// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "RCPtr.h"
#include "graphics_types.h"

namespace kilipili
{

/*
	template class ColorMap<ColorMode> provides a colormap for indexed colormode_i1, i2, i4 and i8.
	For easier use with templates also an empty color map for colormode_rgb is provided.
	The concrete ColorMap classes are based on a common base class `ColorMap<>` (with default template argument).
	Most functions are implemented in the base class, so you can write more generic by using the base class only.

ctor:
	a default constructor for each ColorMode initializes the ColorMap with the respective default colors.
	These are black&green, 4 grey scales, vga4 colors and vga8 colors for the respective modes.
	the ctor can also copy data from a supplied Color array.
	a copy constructor is normally not used but provided.
operator=
	therefore the assignment operator is provided as well.
	a smaller ColorMap can be copied into a larger one.
reset():
reset(Color*):
reset(Color*,count):
	The color table can be reset to default colors, to a supplied Color array of the correct size
	or to a possibly smaller Color table with a given size.
operator[]:
	operator[] is provided to access individual entries in the ColorMap.
	Be careful to not use operator[] on a naked pointer! (then it applies to the pointer)
	Using it on a RCPtr<ColorMap> behaves as expected, though.

ColorMap<colormode_i8>::as<ColorMode>():
	Member function `as()` of class `ColorMap<colormode_i8>` accesses the ColorMap as a ColorMap of a smaller size.
	Most useful for the system_colormap which is used for all ColorModes.
		```system_colormap.as<colormode_i2>().reset();```
	The macros `system_colormap_i1` etc. are a handy way to access the system_colormap that way.
		```system_colormap_i2.reset();```

RCPtr<ColorMap>:
	ColorMaps should be stored in a RCPtr while in use by some object.
	The deduction guide allows you to omit the template parameters if you construct it with argument, e.g.:
		```RCPtr cmap = new ColorMap<colormode_i4>;```
	For a static or const ColorMap you can set .rc = -1 to disable reference counting and destruction by RCPtr. 

system_colormap:
	The `system_colormap` is intended for use with the primary FrameBuffer.
	It is a ColorMap<colormode_i8> and can be used for any color depth, to show any .gif or a .ham image.
	The macros `system_colormap_i1` etc. are a handy way to access the system_colormap in a different ColorMode.
	The system_colormap is writable and can be modified by the program.
	The system_colormap is normally allocated in XRAM (core1 stack area),
	but you can #define GRAPHICS_SYSTEM_COLORMAP_SECTION to put it in any other section. (RAM, presumably.)
*/


// ---------------------- system Color tables --------------------------
/*
	some prepared color tables to initialize or reset a ColorMap.
	these are in flash.
*/
extern const Color zx_colors[16];
extern const Color vga4_colors[16];
extern const Color vga8_colors[256];
extern const Color default_i1_colors[2];
extern const Color default_i2_colors[4];


// ---------------------- struct ColorMap --------------------------
template<ColorMode CM = ColorMode(-1)>
struct ColorMap;

extern ColorMap<colormode_i8> system_colormap;

#define system_colormap_i1 system_colormap.as<colormode_i1>()
#define system_colormap_i2 system_colormap.as<colormode_i2>()
#define system_colormap_i4 system_colormap.as<colormode_i4>()
#define system_colormap_i8 system_colormap


template<>
struct ColorMap<>
{
	void   reset(const Color* cmap, uint count) noexcept;
	Color& operator[](int i) noexcept;
	Color  operator[](int i) const noexcept;
	int	   bestMatch(Color, int count = 0) noexcept;

	ColorMap& operator=(const ColorMap&) noexcept;

	Id("ColorMap");
	mutable int16 rc;
	const uint16  count;
	Color		  colors[0];

protected:
	ColorMap(const Color* cmap, uint count, int16 rc = 0) noexcept;
	ColorMap(uint count, int16 rc = 0) noexcept : rc(rc), count(uint16(count)) {}
};


//
// ---------------------------------------------------------------------------
//				Subclasses for colormode_i1, i2, i4, i8 and rgb
// ---------------------------------------------------------------------------
//

template<>
struct ColorMap<colormode_rgb> : public ColorMap<>
{
	ColorMap(const Color* = nullptr) noexcept : ColorMap<>(0) {}
	ColorMap(const ColorMap& q) noexcept : ColorMap<>(0) {}
	ColorMap& operator=(const ColorMap& q) noexcept
	{
		ColorMap<>::operator=(q);
		return *this;
	}
	void reset(const Color* = nullptr, uint = 0) noexcept {}

	static constexpr Color* colors {nullptr};
};

template<>
struct ColorMap<colormode_i1> : public ColorMap<>
{
	ColorMap(const Color* colors = default_i1_colors) noexcept : ColorMap<>(colors, 2) {}
	ColorMap(const ColorMap& q) noexcept : ColorMap(q.colors) {}
	ColorMap& operator=(const ColorMap& q) noexcept
	{
		ColorMap<>::operator=(q);
		return *this;
	}
	using ColorMap<>::reset;
	void reset(const Color* cmap = default_i1_colors) noexcept { reset(cmap, 2); }

	Color colors[2];
};

template<>
struct ColorMap<colormode_i2> : public ColorMap<>
{
	ColorMap(const Color* colors = default_i2_colors) noexcept : ColorMap<>(colors, 4) {}
	ColorMap(const ColorMap& q) noexcept : ColorMap(q.colors) {}
	ColorMap& operator=(const ColorMap& q) noexcept
	{
		ColorMap<>::operator=(q);
		return *this;
	}
	using ColorMap<>::reset;
	void reset(const Color* cmap = default_i2_colors) noexcept { reset(cmap, 4); }

	Color colors[4];
};

template<>
struct ColorMap<colormode_i4> : public ColorMap<>
{
	ColorMap(const Color* colors = vga4_colors) noexcept : ColorMap<>(colors, 16) {}
	ColorMap(const ColorMap& q) noexcept : ColorMap(q.colors) {}
	ColorMap& operator=(const ColorMap& q) noexcept
	{
		ColorMap<>::operator=(q);
		return *this;
	}
	using ColorMap<>::reset;
	void reset(const Color* cmap = vga4_colors) noexcept { reset(cmap, 16); }

	Color colors[16];
};

template<>
struct ColorMap<colormode_i8> : public ColorMap<>
{
	ColorMap(const Color* colors = vga8_colors, int16 rc = 0) noexcept : ColorMap<>(colors, 256, rc) {}
	ColorMap(const ColorMap& q) noexcept : ColorMap(q.colors) {}
	ColorMap& operator=(const ColorMap& q) noexcept
	{
		ColorMap<>::operator=(q);
		return *this;
	}
	using ColorMap<>::reset;
	void reset(const Color* cmap = vga8_colors) noexcept { reset(cmap, 256); }

	// easy access system_colormap as smaller cmap:
	template<ColorMode CM2>
	std::enable_if_t<CM2 <= colormode_i8, ColorMap<CM2>&> as() noexcept
	{
		assert(count >= 1 << (1 << CM2));
		return *static_cast<ColorMap<CM2>*>(static_cast<ColorMap<>*>(this));
	}
	template<ColorMode CM2>
	std::enable_if_t<CM2 >= colormode_rgb, ColorMap<colormode_rgb>&> as() noexcept
	{
		return *static_cast<ColorMap<colormode_rgb>*>(static_cast<ColorMap<>*>(this));
	}

	Color colors[256];
};


//
// ---------------------------------------------------------------------------
//				Implementations
// ---------------------------------------------------------------------------
//

inline void ColorMap<>::reset(const Color* cmap, uint count) noexcept
{
	assert(cmap != nullptr || count == 0);
	assert(count <= this->count);
	for (uint i = 0; i < count; i++) colors[i] = cmap[i];
}

inline Color& ColorMap<>::operator[](int i) noexcept
{
	assert(i < count);
	return colors[i];
}
inline Color ColorMap<>::operator[](int i) const noexcept
{
	assert(i < count);
	return colors[i];
}

inline ColorMap<>::ColorMap(const Color* cmap, uint count, int16 rc) noexcept : rc(rc), count(uint16(count))
{
	for (uint i = 0; i < count; i++) colors[i] = cmap[i];
}

inline ColorMap<>& ColorMap<>::operator=(const ColorMap<>& q) noexcept
{
	reset(q.colors, q.count);
	return *this;
}


// -------------------------------------------
// deduction guide:

template<ColorMode CM>
RCPtr(ColorMap<CM>*) -> RCPtr<ColorMap<CM>>;


} // namespace kilipili


/*
































*/
