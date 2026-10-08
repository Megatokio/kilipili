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

namespace kilipili::Video
{
template<class CharMap, typename>
class FrameBuffer;
}

namespace kilipili::Graphics
{

/*	template class CharMap provides a character buffer where each character is accompanied by an attribute byte.
	The attribute byte for each character follows immediately after the character.
	It is intended to be used as a backing store for a character-based FrameBuffer.

	All character codes 0x00 to 0xFF are allowed.

	Attributes are configurable per template arguments:
	template parameters:
		fgbits:    foreground color bits
		bgbits:    background color bits
		bold:      enable bold attribute
		underline: enable underline attribute
		graphics:  enable secondary font
		
	The total number of bits must not exceed 8.

	The special class CharMap<> (with all paramters set to default) serves as a common base class
	which contains everything except attribute related settings.

	Printing to a CharMap<…> can be done using `Printer<CharMap<>>`. (the base class!)
	Using a Printer for the exact class is possible but you may end up instantiating more Printers than needed.

	FrameBuffer<CharMap> is used for display.
	CharMaps are compute intensive to display.
	All modes display properly up to 800x600.
	In 1024x768 @ 260MHz only CharMap<0,0,0,0,0> and CharMap<0,0,0,1,0> display properly.
	All others display only if they contain a fair amount of spaces. (not recommended to use.)
*/


template<int fg_bits = -1, int bg_bits = 0, bool bold = true, bool underline = true, bool graphics = false>
class CharMap;

/*
	The base class:
*/
template<>
class CharMap<>
{
public:
	~CharMap() noexcept { delete[] data; }
	NO_COPY_MOVE(CharMap);

	void clear(char c = ' ', uint8 attr = 0) noexcept { clearRect(0, 0, rows, cols, c, attr); }
	void clearRect(int row, int col, int rows, int cols, char = ' ', uint8 attr = 0) noexcept;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept;

	//void setAttr(uint8 a) noexcept { attr = a; }
	void putChar(int row, int col, char, uint8 attr = 0) noexcept;
	void putStr(int row, int col, cstr, uint8 attr = 0) noexcept;
	char getChar(int row, int col) noexcept;

	Id("CharMap");

	uint16* data	   = nullptr;
	uint16* cursor_ptr = nullptr;
	int		rows;
	int		cols;

	RCPtr<const Font> font1 {&latin1_12x8};	  // should be copied into ram
	RCPtr<const Font> font2 {&graphics_12x8}; // only used if graphics = true

	uint8 rc = 0; // RCPtr<>

	// for Printer<CharMap<>>:
	uint8 fg_bits;
	uint8 bg_bits;
	uint8 fg_mask;
	uint8 bg_mask;
	uint8 bold;
	uint8 underline;
	uint8 graphics;

	static constexpr uint8 inverted = 0;

protected:
	CharMap(int rows, int cols, uint8 fg_bits, uint8 bg_bits, uint8 bold, uint8 ul, uint8 gra) : //
		rows(rows),
		cols(cols),
		fg_bits(fg_bits),
		bg_bits(bg_bits),
		fg_mask(uint8(((1 << fg_bits) - 1) << bg_bits)),
		bg_mask(uint8(((1 << bg_bits) - 1) << 0)),
		bold(bold),
		underline(ul),
		graphics(gra)
	{
		data = new uint16[uint(rows * cols)];
		clear();
	}

private:
	uint16 char_with_attr(char c, uint8 attr) noexcept { return uchar(c) + uint16(attr << 8); } // LE!
};


/*
	The concrete classes:
*/
template<int _fg_bits, int _bg_bits, bool _bold, bool _underline, bool _graphics>
class CharMap : public CharMap<>
{
public:
	static constexpr int   bg_bits	 = _bg_bits;
	static constexpr int   fg_bits	 = _fg_bits;
	static constexpr int   bg_ss	 = 0;
	static constexpr int   fg_ss	 = bg_bits;
	static constexpr uchar fg_mask	 = ((1 << fg_bits) - 1) << fg_ss;
	static constexpr uchar bg_mask	 = ((1 << bg_bits) - 1) << bg_ss;
	static constexpr uchar bold		 = _bold << (fg_bits + bg_bits);
	static constexpr uchar underline = _underline << (fg_bits + bg_bits + _bold);
	static constexpr uchar graphics	 = _graphics << (fg_bits + bg_bits + _bold + _underline);

	static_assert(fg_bits + bg_bits + _bold + _underline + _graphics <= 8);
	static_assert(uint(fg_bits <= 8));
	static_assert(uint(bg_bits <= 8));

	CharMap(int rows, int cols) : CharMap<>(rows, cols, fg_bits, bg_bits, bold, underline, graphics) {}

	static constexpr uint32 mul = sizeof(Color) == 1 ? 0x01010101 : sizeof(Color) == 2 ? 0x00010001 : 1;
	void					set_fgcolor(int i, Color c) noexcept { fgcolors[i & ((1 << fg_bits) - 1)] = c * mul; }
	void					set_bgcolor(int i, Color c) noexcept { bgcolors[i & ((1 << bg_bits) - 1)] = c * mul; }

private:
	// colors are stored as floodfilled uint32's, as needed by FrameBuffer<>:
	uint32 fgcolors[1 << fg_bits] = {black * mul};
	uint32 bgcolors[1 << bg_bits] = {white * mul};

	friend class Video::FrameBuffer<CharMap, void>;
};


} // namespace kilipili::Graphics


/*

































*/
