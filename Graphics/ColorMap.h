// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "common/no_copy_move.h"


namespace kilipili
{

/*
	class ColorMap provides a colormap for indexed color modes.

ColorMap(int count):
ColorMap(const Color* src, int count):
	The default constructor for each table size initializes the ColorMap with the respective default colors.
	These are black & white, 4 grey scales, vga4 colors and vga8 colors for the respective modes.
	the ctor can also copy data from a supplied Color array `src`.

ColorMap(Color* dest, const Color* src, int count, int rc = 0):
	the ctor can also use a supplied array `dest` as it's colors[] array. If this array is not allocated but static,
	then you should set the refcnt `rc` to -1 to disable reference counting in RCPtrs.
	then the ColorMap will not be deleted by RCPtrs and the supplied array will not be deleted when finally
	at some point the ColorTable is destroyed. That's to say, memory management is then completely up to you.

reset():
reset(Color*):
reset(Color*,count):
reset(ColorMap*):
	The color table can be reset to default colors, to a supplied Color array or copied from another ColorMap.

operator[]:
	operator[] is provided to access individual entries in the ColorMap.
	Be careful to not use operator[] on a naked pointer! (then it applies to the pointer)
	Using it on a RCPtr<ColorMap> will call ColorMap::operator[] as expected, though.

bestMatch(Color):
	find the best match for a Color in the ColorMap.

reverse():
complement():
roll(dist):
shuffle():
lighter():
darker():
	modify the Colors in the ColorMap in some way.
	return a ColorMap* to this for chained operation.

RCPtr<ColorMap>:
	ColorMaps should be passed around and stored in RCPtrs while in use by some object.

system_colormap:
	The `system_colormap` is intended for use with the primary FrameBuffer.
	It is a ColorMap[256] and can be used for any color depth, to show any .gif or a .ham image.
	The system_colormap is writable and can be modified by the program.
	The system_colormap is normally allocated in XRAM (core1 stack area),
	but you can #define GRAPHICS_SYSTEM_COLORMAP_SECTION to put it in any other section. (RAM, presumably.)
*/


// ------------------- the system Color table -----------------------

extern class ColorMap system_colormap;

// some prepared color tables in ROM to initialize or reset a ColorMap.
// these are the default colors if you don't supply your own table:

extern const Color default_i1_colors[2]; // black & white
extern const Color default_i2_colors[4]; // grey scale
extern const Color zx_colors[16];		 // 2x 8 RGB colors
extern const Color vga4_colors[16];		 // 2x 8 RGB colors
extern const Color vga8_colors[256];	 // vga4 + RGB[6,6,6] + greyscale


// ---------------------- class ColorMap --------------------------

class ColorMap
{
	NO_COPY_MOVE(ColorMap);

public:
	ColorMap(Color* dest, const Color* src, int count, int rc = 0);
	explicit ColorMap(int count) : ColorMap(nullptr, nullptr, count) {}
	ColorMap(const Color* colors, int count) : ColorMap(nullptr, colors, count) {}
	~ColorMap() noexcept;

	void reset(const Color* colors = nullptr, int count = 0) noexcept;
	void reset(const ColorMap* q) noexcept { reset(q->colors, q->count); }

	Color& operator[](int i) noexcept;
	Color  operator[](int i) const noexcept;

	uint16	  bestMatch(Color, int count = 0) noexcept;
	ColorMap* reverse() noexcept;
	ColorMap* complement() noexcept;
	ColorMap* roll(int dist) noexcept; // dist>0 => up
	ColorMap* shuffle() noexcept;
	ColorMap* lighter() noexcept;
	ColorMap* darker() noexcept;

	Id("ColorMap");
	mutable int16 rc;
	const uint16  count;
	Color* const  colors;
};


// ---------------------------------------------------------------------------
// inline definitions:

inline Color& ColorMap::operator[](int i) noexcept
{
	assert(i < count);
	return colors[i];
}
inline Color ColorMap::operator[](int i) const noexcept
{
	assert(i < count);
	return colors[i];
}


} // namespace kilipili


/*
































*/
