// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "common/cdefs.h"
#include "common/no_copy_move.h"
#include "common/standard_types.h"
#include "systemfont.h"

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

	void reset() noexcept { attr = 0; }

	void clear(char c = ' ') noexcept;
	void clearRect(int row, int col, int rows, int cols, char = ' ') noexcept;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept;

	void setAttr(uint8 a) noexcept { attr = a & 0x80; }
	void putChar(int row, int col, char) noexcept;
	void putStr(int row, int col, cstr) noexcept;

	Id("CharMap");
	uint16 rc		= 0; // RCPtr<>
	uint8  attr		= 0;
	uint8  _padding = 0;

	cuptr  font = ascii_256x12_inverse[0];
	uchar* data = nullptr;
	int	   rows;
	int	   cols;
	Color  fgcolor = black;
	Color  bgcolor = white;
};


} // namespace kilipili::Graphics


/*

































*/
