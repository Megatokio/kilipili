// Copyright (c) 2012 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/DrawEngine.h"

namespace kio
{

#ifndef USE_WIDECHARS
  #define USE_WIDECHARS 0
#endif

#if USE_WIDECHARS
using Char = uint16;
#else
using Char = uchar;
#endif


template<Graphics::ColorMode CM>
class PicoTerm
{
public:
	using ColorDepth = Graphics::ColorDepth;
	using ColorMode	 = Graphics::ColorMode;
	using AttrMode	 = Graphics::AttrMode;
	using AttrWidth	 = Graphics::AttrWidth;
	template<ColorMode cm>
	using Pixmap = Graphics::Pixmap<cm>;
	template<ColorMode cm>
	using DrawEngine = Graphics::DrawEngine<cm>;

	static constexpr ColorDepth CD			   = get_colordepth(CM); // 0 .. 4  log2 of bits per color in attributes[]
	static constexpr AttrMode	AM			   = get_attrmode(CM);	 // 0 .. 2  log2 of bits per color in pixmap[]
	static constexpr AttrWidth	AW			   = get_attrwidth(CM);	 // 0 .. 3  log2 of width of tiles
	static constexpr int		bits_per_color = 1 << CD; // bits per color in pixmap[] (wAttr: in attributes[])
	static constexpr int bits_per_pixel = is_attribute_mode(CM) ? 1 << AM : 1 << CD; // bits per pixel in pixmap[]

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
		SET_WINDOW			 = 20,
		SCROLL_SCREEN		 = 21,
	};

	Pixmap<CM>&		 pixmap;
	Graphics::Color* colormap;
	DrawEngine<CM>	 draw_engine;

	// Screen size:
	int screen_width;  // [characters]
	int screen_height; // [characters]

	// current print position:
	int	  row;
	int	  col;
	int	  dx, dy; // 1 or 2, if double width & double height
	uint8 attributes;

	// foreground and background color:
	uint bgcolor;	  // paper color
	uint fgcolor;	  // text color
	uint fgpixel = 1; // value used for pixels[] in tiled mode
	uint bgpixel = 0; // value used for pixels[] in tiled mode

	// a "cursor stack":
	int	  pushedRow;
	int	  pushedCol;
	uint8 pushedAttr;

	// cursor blob:
	bool   cursorVisible;  // currently visible?
	uint32 cursorXorValue; // value used to xor the colors

	explicit PicoTerm(Pixmap<CM>&, Graphics::Color* colors);

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
	void hideCursor()
	{
		if (unlikely(cursorVisible)) show_cursor(false);
	}
	void validateCursorPosition();
	void scrollScreenUp(int rows /*char*/);
	void scrollScreenDown(int rows /*char*/);
	void scrollScreenLeft(int cols /*char*/);
	void scrollScreenRight(int cols /*char*/);

	void print_sm(cstr text, int cnt = 0);
	uint print_sm_state = 0;
	char print_sm_bu[16];
	uint print_sm_bu_idx = 0;

	char* identify(char* bu);

	void readBmp(CharMatrix);
	void writeBmp(CharMatrix, uint8 attr);
	void getCharMatrix(CharMatrix, Char c);
	void getGraphicsCharMatrix(CharMatrix, Char c);
	void applyAttributes(CharMatrix);
	void eraseRect(int row, int col, int rows, int cols);
	void setWindow(int row, int col, int rows, int cols);

private:
	void show_cursor(bool f);
};


extern template class PicoTerm<Graphics::colormode_i1>;
extern template class PicoTerm<Graphics::colormode_i2>;
extern template class PicoTerm<Graphics::colormode_i4>;
extern template class PicoTerm<Graphics::colormode_i8>;
extern template class PicoTerm<Graphics::colormode_rgb>;
extern template class PicoTerm<Graphics::colormode_a1w1_i4>;
extern template class PicoTerm<Graphics::colormode_a1w1_i8>;
extern template class PicoTerm<Graphics::colormode_a1w1_rgb>;
extern template class PicoTerm<Graphics::colormode_a1w2_i4>;
extern template class PicoTerm<Graphics::colormode_a1w2_i8>;
extern template class PicoTerm<Graphics::colormode_a1w2_rgb>;
extern template class PicoTerm<Graphics::colormode_a1w4_i4>;
extern template class PicoTerm<Graphics::colormode_a1w4_i8>;
extern template class PicoTerm<Graphics::colormode_a1w4_rgb>;
extern template class PicoTerm<Graphics::colormode_a1w8_i4>;
extern template class PicoTerm<Graphics::colormode_a1w8_i8>;
extern template class PicoTerm<Graphics::colormode_a1w8_rgb>;
extern template class PicoTerm<Graphics::colormode_a2w1_i4>;
extern template class PicoTerm<Graphics::colormode_a2w1_i8>;
extern template class PicoTerm<Graphics::colormode_a2w1_rgb>;
extern template class PicoTerm<Graphics::colormode_a2w2_i4>;
extern template class PicoTerm<Graphics::colormode_a2w2_i8>;
extern template class PicoTerm<Graphics::colormode_a2w2_rgb>;
extern template class PicoTerm<Graphics::colormode_a2w4_i4>;
extern template class PicoTerm<Graphics::colormode_a2w4_i8>;
extern template class PicoTerm<Graphics::colormode_a2w4_rgb>;
extern template class PicoTerm<Graphics::colormode_a2w8_i4>;
extern template class PicoTerm<Graphics::colormode_a2w8_i8>;
extern template class PicoTerm<Graphics::colormode_a2w8_rgb>;

} // namespace kio
