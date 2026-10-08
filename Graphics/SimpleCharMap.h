// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Font.h"
#include "RCPtr.h"
#include "common/cdefs.h"
#include "common/no_copy_move.h"
#include "common/standard_types.h"

namespace kilipili::Graphics
{

/*	class SimpleCharMap is a pure 2-dimensional character array with no attributes.
	It is intended to be used as a backing store for FrameBuffer<SimpleCharMap>.

	All character codes incl. 0x00 are available.
	It supports only one foreground and one background color.
	One attribute, e.g. inverted colors, may be achieved by setting bit 7 and using a modified character set:
	--> latin1_256x12[][]  or
	--> ascii_256x12_inverse[][]

	Printing to a SimpleCharMap can be done using `Printer<SimpleCharMap>`.
	.font, .fgcolor and .bgcolor are provided for FrameBuffer<SimpleCharMap>.
*/

class SimpleCharMap
{
public:
	SimpleCharMap(int rows, int cols);
	~SimpleCharMap() noexcept { delete[] data; }
	NO_COPY_MOVE(SimpleCharMap);

	void clear(char c = ' ', uint8 attr = 0) noexcept;
	void clearRect(int row, int col, int rows, int cols, char = ' ', uint8 attr = 0) noexcept;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept;

	void putChar(int row, int col, char, uint8 attr = 0) noexcept;
	void putStr(int row, int col, cstr, uint8 attr = 0) noexcept;
	char getChar(int row, int col) noexcept;

	Id("CharMap");

	uint16 rc		= 0; // RCPtr<>
	uint16 _padding = 0;

	RCPtr<const Font>  font1 {&ascii_12x8_inverse};
	static const Font* font2; // n.ex.: for similarity with CharMap

	uchar* data		  = nullptr;
	uchar* cursor_ptr = nullptr;
	int	   rows;
	int	   cols;
	Color  fgcolor = black;
	Color  bgcolor = white;

	// attributes:
	// should be updated if another font is set:
	uint8 bold		= 0x80;
	uint8 inverted	= 0;
	uint8 underline = 0;
	uint8 graphics	= 0;
};


} // namespace kilipili::Graphics


/*

































*/
