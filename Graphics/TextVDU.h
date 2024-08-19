// Copyright (c) 2012 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Canvas.h"
#include <functional>
#undef CHAR_WIDTH

namespace kio::Graphics
{


class TextVDU
{
public:
	static constexpr int CHAR_WIDTH	 = 8;
	static constexpr int CHAR_HEIGHT = 12;

	uint default_bgcolor = 0x0000ffff; // white paper (note: ic1 has inverted default colormap)
	uint default_fgcolor = 0;		   // black ink   (      => black paper & green ink)

	using CharMatrix = uint8[CHAR_HEIGHT];

	// print attributes:
	enum Attributes : uint8 {
		NORMAL		  = 0,
		BOLD		  = 1 << 0,
		UNDERLINE	  = 1 << 1,
		INVERTED	  = 1 << 2,
		ITALIC		  = 1 << 3,
		TRANSPARENT	  = 1 << 4,
		DOUBLE_WIDTH  = 1 << 5,
		DOUBLE_HEIGHT = 1 << 6,
		GRAPHICS	  = 1 << 7
	};

	CanvasPtr pixmap;

	const ColorMode	 colormode;
	const AttrHeight attrheight;
	const ColorDepth colordepth;	 // 0 .. 4  log2 of bits per color in attributes[]
	const AttrMode	 attrmode;		 // 0 .. 2  log2 of bits per color in pixmap[]
	const AttrWidth	 attrwidth;		 // 0 .. 3  log2 of width of tiles
	const uint8		 bits_per_color; // bits per color in pixmap[] or attributes[]
	const uint8		 bits_per_pixel; // bpp in pixmap[]
	char			 _padding = 0;

	// Screen size:
	int screen_width;  // [characters]
	int screen_height; // [characters]

	// foreground and background color:
	uint bgcolor; // paper color
	uint fgcolor; // text color
	uint fg_ink;  // for attribute pixmaps
	uint bg_ink;  // for attribute pixmaps

	// current print position:
	int		   row, col;
	uint8	   dx, dy; // 1 or 2, if double width & double height
	Attributes attributes;

	// cursor blob:
	bool   cursorVisible;  // currently visible?
	uint32 cursorXorColor; // value used to xor the colors

	TextVDU(CanvasPtr) noexcept;

	void reset() noexcept;
	void cls() noexcept;
	void identify() noexcept;
	void moveTo(int row, int col) noexcept;
	void moveToCol(int col) noexcept;
	void moveToRow(int row) noexcept;
	void setCharAttributes(uint add, uint remove = 0xff) noexcept;
	void addCharAttributes(uint a = 0xff) noexcept { setCharAttributes(a, 0); }
	void removeCharAttributes(uint a = 0xff) noexcept { setCharAttributes(0, a); }
	void printCharMatrix(CharMatrix, int count = 1) noexcept;
	void printChar(char c, int count = 1) noexcept;			// no ctl
	void print(cstr text) noexcept;							// supports \n and \t
	void printf(cstr fmt, ...) noexcept __printflike(2, 3); // supports \n and \t
	str	 inputLine(std::function<int()> getchar, str oldtext = nullptr, int epos = 0);
	void cursorLeft(int count = 1) noexcept;
	void cursorRight(int count = 1) noexcept;
	void cursorUp(int count = 1) noexcept;
	void cursorDown(int count = 1) noexcept;
	void cursorTab(int count = 1) noexcept;
	void cursorReturn() noexcept;
	void newLine() noexcept;
	void clearToStartOfLine(bool incl_cursorpos = 0) noexcept;
	void clearToStartOfScreen(bool incl_cursorpos = 0) noexcept;
	void clearToEndOfLine() noexcept;
	void clearToEndOfScreen() noexcept;
	void copyRect(int src_row, int src_col, int dest_row, int dest_col, int rows, int cols) noexcept;
	void showCursor(bool on = true) noexcept;
	void hideCursor() noexcept;
	void validateCursorPosition() noexcept;
	void scrollScreen(coord dx, coord dy) noexcept;
	void scrollScreenUp(int rows /*char*/) noexcept;
	void scrollScreenDown(int rows /*char*/) noexcept;
	void scrollScreenLeft(int cols /*char*/) noexcept;
	void scrollScreenRight(int cols /*char*/) noexcept;
	void readBmp(CharMatrix, bool use_fgcolor) noexcept;
	void writeBmp(CharMatrix, uint8 attr) noexcept;
	void getCharMatrix(CharMatrix, char c) noexcept;
	void getGraphicsCharMatrix(CharMatrix, char c) noexcept;
	void applyAttributes(CharMatrix) noexcept;
	void eraseRect(int row, int col, int rows, int cols) noexcept;

private:
	void show_cursor(bool f) noexcept;
};

} // namespace kio::Graphics
