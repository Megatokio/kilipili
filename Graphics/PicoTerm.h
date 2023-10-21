// Copyright (c) 2012 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Canvas.h"


namespace kio::Graphics
{

class PicoTerm
{
public:
	static constexpr int CHAR_HEIGHT = 12;
	static constexpr int CHAR_WIDTH	 = 8;

	static constexpr uint default_bgcolor = 0x0000ffff; // white paper (note: ic1 has inverted default colormap)
	static constexpr uint default_fgcolor = 0;			// black ink   (      => black paper & green ink)

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
		RESET				   = 0,
		CLS					   = 1,
		MOVE_TO_POSITION	   = 2,
		MOVE_TO_COL			   = 3,
		PUSH_CURSOR_POSITION   = 5,
		POP_CURSOR_POSITION	   = 6,
		SHOW_CURSOR			   = 7,	 //				(BELL)
		CURSOR_LEFT			   = 8,	 // scrolls		(BS)
		TAB					   = 9,	 // scrolls
		CURSOR_DOWN			   = 10, // scrolls		(NL)
		CURSOR_UP			   = 11, // scrolls
		CURSOR_RIGHT		   = 12, // scrolls		(FF)
		RETURN				   = 13, // COL := 0
		CLEAR_TO_END_OF_LINE   = 14,
		CLEAR_TO_END_OF_SCREEN = 15,
		SET_ATTRIBUTES		   = 16,
		XON					   = 17,
		PRINT_INLINE_GLYPH	   = 18,
		XOFF				   = 19,
		REPEAT_NEXT_CHAR	   = 20,
		SCROLL_SCREEN		   = 21,
		ESC					   = 27,
	};

	Canvas& pixmap;

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

	PicoTerm(Canvas&);

	void reset();
	void cls();
	void moveToPosition(int row, int col) noexcept;
	void moveToCol(int col) noexcept;
	void pushCursorPosition();
	void popCursorPosition();
	void setPrintAttributes(uint8 attr);
	void printCharMatrix(CharMatrix, int count = 1);
	void printChar(char c, int count = 1);
	void printText(cstr text);
	void print(cstr text_w_controlcodes, bool auto_crlf = true);
	void printf(cstr fmt, ...) __printflike(2, 3);
	void cursorLeft(int count = 1);
	void cursorRight(int count = 1);
	void cursorUp(int count = 1);
	void cursorDown(int count = 1);
	void cursorTab(int count = 1);
	void cursorReturn();
	void clearToEndOfLine();
	void clearToEndOfScreen();
	void copyRect(int src_row, int src_col, int dest_row, int dest_col, int rows, int cols);
	void showCursor();
	void hideCursor();
	void validateCursorPosition();
	void scrollScreen(coord dx, coord dy);
	void scrollScreenUp(int rows /*char*/);
	void scrollScreenDown(int rows /*char*/);
	void scrollScreenLeft(int cols /*char*/);
	void scrollScreenRight(int cols /*char*/);

	char* identify();

	void readBmp(CharMatrix, bool use_fgcolor);
	void writeBmp(CharMatrix, uint8 attr);
	void getCharMatrix(CharMatrix, char c);
	void getGraphicsCharMatrix(CharMatrix, char c);
	void applyAttributes(CharMatrix);
	void eraseRect(int row, int col, int rows, int cols);

private:
	void show_cursor(bool f);
};


} // namespace kio::Graphics


/*



























*/
