// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "common/no_copy_move.h"


namespace kilipili
{

/*
	template class ColorMap<bits> provides a colormap for indexed color modes.
	The ColorMap is sized to contain 2^bits Colors.
	Legal values are from 0 to 8 resulting in a ColorMap with 1 to 256 Colors.

ColorMap<bits>():
ColorMap<bits>(const Color* src):
	The default constructor for each table size initializes the ColorMap with the respective default colors.
	These are black & white, 4 grey scales, vga4 colors and vga8 colors for the respective modes.
	the ctor can also copy data from a supplied Color array `src`.

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
*/


// ------------------- the system Color table -----------------------

// some prepared color tables in ROM to initialize or reset a ColorMap.
// these are the default colors if you don't supply your own table:

extern const Color default_i1_colors[2]; // black & white
extern const Color default_i2_colors[4]; // grey scale
extern const Color zx_colors[16];		 // 2x 8 RGB colors
extern const Color vga4_colors[16];		 // 2x 8 RGB colors
extern const Color vga8_colors[256];	 // vga4 + RGB[6,6,6] + greyscale


// ---------------------- class ColorMap --------------------------

template<int bits = 0>
class ColorMap;

using ColorMap_i1 = ColorMap<1>;
using ColorMap_i2 = ColorMap<2>;
using ColorMap_i4 = ColorMap<4>;
using ColorMap_i8 = ColorMap<8>;


template<>
class ColorMap<0>
{
	NO_COPY_MOVE(ColorMap);

public:
	explicit ColorMap(const Color* src = nullptr) noexcept : ColorMap(src, 0) {}

	void reset(const Color* src = nullptr, int count = 0) noexcept;
	void reset(const ColorMap* q) noexcept { reset(q->colors, q->count); }

	Color& operator[](int i) noexcept;
	Color  operator[](int i) const noexcept;

	uint16 bestMatch(Color, int count = 0) noexcept;
	void   reverse() noexcept;
	void   complement() noexcept;
	void   roll(int dist) noexcept; // dist>0 => up
	void   shuffle() noexcept;
	void   lighter() noexcept;
	void   darker() noexcept;

	Id("ColorMap");
	mutable uint16 rc = 0;
	const uint16   count;
	Color		   colors[1];

protected:
	ColorMap(const Color* src, int bits) noexcept;
};


// ---------------------------------------------------------------------------
// ColorMap<1> to ColorMap<8>:

template<int bits>
class ColorMap : public ColorMap<bits - 1>
{
public:
	ColorMap(const Color* q = nullptr) noexcept : ColorMap<bits - 1>(q, bits) {}

protected:
	ColorMap(const Color* q, int nbits) noexcept : ColorMap<bits - 1>(q, nbits) {}
	Color::uRGB more_colors[(1 << bits) - (1 << (bits - 1))];
};

template<>
class ColorMap<16> : public ColorMap<8>
{};
template<>
class ColorMap<32> : public ColorMap<8>
{};


// ---------------------------------------------------------------------------
// inline definitions:

inline Color& ColorMap<0>::operator[](int i) noexcept
{
	assert_lt(i, count);
	return colors[i];
}
inline Color ColorMap<0>::operator[](int i) const noexcept
{
	assert_lt(i, count);
	return colors[i];
}


} // namespace kilipili


/*
































*/
