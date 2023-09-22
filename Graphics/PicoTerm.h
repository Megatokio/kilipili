// Copyright (c) 2012 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "IPixmap.h"

namespace kio::Graphics
{


#ifndef USE_WIDECHARS
  #define USE_WIDECHARS 0
#endif

#if USE_WIDECHARS
using Char = uint16;
#else
using Char = uchar;
#endif


class PicoTerm
{
public:
	static constexpr int  CHAR_HEIGHT = 12;
	static constexpr int  CHAR_WIDTH  = 8;
	static constexpr bool INVERTED	  = true; // true => paper = 0xffff, pen = 0x0000

	using CharMatrix = uint8[CHAR_HEIGHT];

	// print attributes:
	enum : uint8 {
		ATTR_BOLD				 = 1 << 0,
		ATTR_UNDERLINE			 = 1 << 1,
		ATTR_INVERTED			 = 1 << 2,
		ATTR_ITALIC				 = 1 << 3,
		ATTR_OVERPRINT			 = 1 << 4,
		ATTR_DOUBLE_WIDTH		 = 1 << 5,
		ATTR_DOUBLE_HEIGHT		 = 1 << 6,
		ATTR_GRAPHICS_CHARACTERS = 1 << 7
	};

	// Control Characters:
	enum : uchar {
		RESET				 = 0,
		CLS					 = 1,
		MOVE_TO_POSITION	 = 2,
		MOVE_TO_COL			 = 3,
		PUSH_CURSOR_POSITION = 5,
		POP_CURSOR_POSITION	 = 6,
		SHOW_CURSOR			 = 7,  //				(BELL)
		CURSOR_LEFT			 = 8,  // scrolls		(BS)
		TAB					 = 9,  // scrolls
		CURSOR_DOWN			 = 10, // scrolls		(NL)
		CURSOR_UP			 = 11, // scrolls
		CURSOR_RIGHT		 = 12, // scrolls		(FF)
		RETURN				 = 13, // COL := 0
		CLEAR_TO_END_OF_LINE = 14,
		PRINT_INLINE_GLYPH	 = 15,
		SET_ATTRIBUTES		 = 16,
		XON					 = 17,
		REPEAT_NEXT_CHAR	 = 18,
		XOFF				 = 19,
		SCROLL_SCREEN		 = 21,
	};

	IPixmap& pixmap;
	Color*	 colormap;

	const ColorMode	 colormode;
	const AttrHeight attrheight;
	const ColorDepth colordepth;	 // 0 .. 4  log2 of bits per color in attributes[]
	const AttrMode	 attrmode;		 // 0 .. 2  log2 of bits per color in pixmap[]
	const AttrWidth	 attrwidth;		 // 0 .. 3  log2 of width of tiles
	const int		 bits_per_color; // bits per color in pixmap[] or attributes[]
	const int		 bits_per_pixel; // bpp in pixmap[]

	// Screen size:
	int screen_width;  // [characters]
	int screen_height; // [characters]

	// current print position:
	int	  row;
	int	  col;
	int	  dx, dy; // 1 or 2, if double width & double height
	uint8 attributes;

	// foreground and background color:
	uint bgcolor;	 // paper color
	uint fgcolor;	 // text color
	uint fg_ink = 1; // for attribute pixmaps
	uint bg_ink = 0; // for attribute pixmaps

	// a "cursor stack":
	int	  pushedRow;
	int	  pushedCol;
	uint8 pushedAttr;

	// cursor blob:
	bool   cursorVisible;  // currently visible?
	uint32 cursorXorColor; // value used to xor the colors

	PicoTerm(IPixmap&, Color* colors);

	void reset();
	void cls();
	void moveToPosition(int row, int col) noexcept;
	void moveToCol(int col) noexcept;
	void pushCursorPosition();
	void popCursorPosition();
	void setPrintAttributes(uint8 attr);
	void printCharMatrix(CharMatrix, int count = 1);
	void printChar(Char c, int count = 1);
	void printText(cstr text);
	//void print(std::function<char(void*data)>);
	void print(cstr text_w_controlcodes, bool auto_crlf = true);
	void printf(cstr fmt, ...) __printflike(2, 3);
	void cursorLeft(int count = 1);
	void cursorRight(int count = 1);
	void cursorUp(int count = 1);
	void cursorDown(int count = 1);
	void cursorTab(int count = 1);
	void cursorReturn();
	void clearToEndOfLine();
	void copyRect(int src_row, int src_col, int dest_row, int dest_col, int rows, int cols);
	void showCursor();
	void hideCursor();
	void validateCursorPosition();
	void scrollScreen(coord dx, coord dy);
	void scrollScreenUp(int rows /*char*/);
	void scrollScreenDown(int rows /*char*/);
	void scrollScreenLeft(int cols /*char*/);
	void scrollScreenRight(int cols /*char*/);

	char* identify(char* buffer);

	void readBmp(CharMatrix, bool use_fgcolor);
	void writeBmp(CharMatrix, uint8 attr);
	void getCharMatrix(CharMatrix, Char c);
	void getGraphicsCharMatrix(CharMatrix, Char c);
	void applyAttributes(CharMatrix);
	void eraseRect(int row, int col, int rows, int cols);

private:
	void show_cursor(bool f);
};


} // namespace kio::Graphics


/*



























*/
