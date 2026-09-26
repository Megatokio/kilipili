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

/*	template class CharMap provides a character buffer where each character is accompanied by an attribute byte.
	The attribute byte for each character follows immediately after the character.
	It is intended to be used as a backing store for a character-based FrameBuffer.

	All character codes 0x00 to 0xFF are allow.

	Attributes are configurable per template arguments:
	template parameters:
		fgbits:    foreground color bits
		bgbits:    background color bits
		bold:      enable bold attribute
		underline: enable underline attribute
	The total number of bits must not exceed 8.

	The special class CharMap<> (with all paramters set to default) serves as a common base class
	which contains everything except attribute related settings.

	Printing to a CharMap<…> can be done using `Printer<CharMap<>>`. (the base class!)
	Using a Printer for the exact class is possible but you may end up instantiating more Printers than needed.
*/


/*	____________________
	Template definition:
*/
template<int fg_bits = -1, int bg_bits = 0, bool bold = true, bool underline = true>
class CharMap;


/*	_______________
	The base class:
*/
template<>
class CharMap<>
{
public:
	~CharMap() noexcept { delete[] data; }
	NO_COPY_MOVE(CharMap);

	void reset() noexcept { attr = 0; }

	void clear(char c = ' ') noexcept { clearRect(0, 0, rows, cols, c); }
	void clearRect(int row, int col, int rows, int cols, char = ' ') noexcept;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept;

	void setAttr(uint8 a) noexcept { attr = a; }
	void putChar(int row, int col, char) noexcept;
	void putStr(int row, int col, cstr) noexcept;

	Id("CharMap");
	uint16 rc	= 0; // RCPtr<>
	uint16 attr = 0;

	cuptr	font = latin1_256x12[0];
	uint16* data = nullptr;
	int		rows;
	int		cols;

protected:
	CharMap(int rows, int cols) : rows(rows), cols(cols)
	{
		data = new uint16[uint(rows * cols)];
		clear();
	}

private:
	uint16 char_with_attr(char c) noexcept { return uchar(c) + uint16(attr << 8); } // LE!
};


/*	_____________________
	The concrete classes:
*/
template<int _fg_bits, int _bg_bits, bool _bold, bool _underline>
class CharMap : public CharMap<>
{
public:
	static constexpr int   bg_bits		  = _bg_bits;
	static constexpr int   fg_bits		  = _fg_bits;
	static constexpr bool  bold			  = _bold;
	static constexpr bool  underline	  = _underline;
	static constexpr int   bg_ss		  = 0;
	static constexpr int   fg_ss		  = bg_bits;
	static constexpr uchar fg_mask		  = ((1 << fg_bits) - 1) << fg_ss;
	static constexpr uchar bg_mask		  = ((1 << bg_bits) - 1) << bg_ss;
	static constexpr uchar bold_mask	  = bold << (fg_bits + bg_bits);
	static constexpr uchar underline_mask = underline << (fg_bits + bg_bits + bold);

	static_assert(fg_bits + bg_bits + bold + underline <= 8);
	static_assert(uint(fg_bits <= 8));
	static_assert(uint(bg_bits <= 8));

	CharMap(int rows, int cols) : CharMap<>(rows, cols) {}

	using CharMap<>::setAttr;
	void setAttr(int fg, int bg, bool bold, bool ul) noexcept;
	void set_fgcolor(int i) noexcept;
	void set_bgcolor(int i) noexcept;
	void set_bold(bool) noexcept;
	void set_underline(bool) noexcept;

	Color fgcolors[1 << fg_bits] = {black};
	Color bgcolors[1 << bg_bits] = {white};
};


//
// *****************************************************************************
//					I M P L E M E N T A T I O N S
// *****************************************************************************
//

template<int fg_bits, int bg_bits, bool bold, bool underline>
void CharMap<fg_bits, bg_bits, bold, underline>::set_fgcolor(int i) noexcept
{
	attr &= ~fg_mask;
	attr |= (i << fg_ss) & fg_mask;
}

template<int fg_bits, int bg_bits, bool bold, bool underline>
void CharMap<fg_bits, bg_bits, bold, underline>::set_bgcolor(int i) noexcept
{
	attr &= ~bg_mask;
	attr |= (i << bg_ss) & bg_mask;
}

template<int fg_bits, int bg_bits, bool bold, bool underline>
void CharMap<fg_bits, bg_bits, bold, underline>::set_bold(bool f) noexcept
{
	if (f) attr |= bold_mask;
	else attr &= ~bold_mask;
}

template<int fg_bits, int bg_bits, bool bold, bool underline>
void CharMap<fg_bits, bg_bits, bold, underline>::set_underline(bool f) noexcept
{
	if (f) attr |= underline_mask;
	else attr &= ~underline_mask;
}

template<int fg_bits, int bg_bits, bool bold, bool underline>
void CharMap<fg_bits, bg_bits, bold, underline>::setAttr(int fg, int bg, bool bold, bool ul) noexcept
{
	attr = ((fg << fg_ss) & fg_mask) | ((bg << bg_ss) & bg_mask);
	if (bold) attr |= bold_mask;
	if (ul) attr |= underline_mask;
}

} // namespace kilipili::Graphics


/*

































*/
