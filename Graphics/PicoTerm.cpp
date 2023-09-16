// Copyright (c) 2012 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "PicoTerm.h"
#include "Graphics/DrawEngine.h"
#include "Utilities/utilities.h"
#include <pico/printf.h>
#include <pico/stdio.h>

// https://k1.spdns.de/Develop/Hardware/Projekte/LM641541%20-%20LCD%20Display%20640x480/


namespace kipili
{

using namespace kipili::Graphics;


// ------------------------------------------------------------
// 				Const Data:
// ------------------------------------------------------------

// lsbit is left => revert bits
#define r(b)                                                                                                 \
  ((b & 128) >> 7) + ((b & 64) >> 5) + ((b & 32) >> 3) + ((b & 16) >> 1) + ((b & 8) << 1) + ((b & 4) << 3) + \
	  ((b & 2) << 5) + ((b & 1) << 7)

// The Character Set and Font:
//
static constexpr const uchar font[(256 - 32 - 32) * 12] = {
#include "rsrc/font_12x8.h"
};

// convert nibble -> double width byte:
//
static constexpr const uint8 dblw[16] = {
	r(0x00), r(0x03), r(0x0C), r(0x0F), r(0x30), r(0x33), r(0x3C), r(0x3F),
	r(0xC0), r(0xC3), r(0xCC), r(0xCF), r(0xF0), r(0xF3), r(0xFC), r(0xFF),
};


// =============================================================================
//							F U N C T I O N S
// =============================================================================

template<ColorMode CM>
PicoTerm<CM>::PicoTerm(Pixmap<CM>& pixmap, Color* colors) :
	pixmap(pixmap),
	colormap(colors),
	draw_engine(pixmap, colors)
{
	reset();
}


template<ColorMode CM>
void PicoTerm<CM>::show_cursor(bool show)
{
	int x = col * CHAR_WIDTH;
	int y = row * CHAR_HEIGHT;

	if constexpr (is_direct_color(CM))
	{
		// direct color:
		// for all pixels: pixel ^= fgcolor ^ bgcolor

		uint8* start = pixmap.pixmap + y * pixmap.row_offset;
		if (show) cursorXorValue = fgcolor ^ bgcolor;
		Graphics::bitblit::xor_rect_of_bits(
			start, x << bits_per_color, pixmap.row_offset, CHAR_WIDTH << bits_per_color, CHAR_HEIGHT,
			Graphics::flood_filled_color<ColorDepth(CM)>(cursorXorValue));
	}
	else
	{
		// with attributes:
		// for all attr.colors: color ^= attr.color[fgcolor] ^ attr.color[bgcolor]
		// if those colors are the same then xor colors with 0xffff.
		// if tiles are smaller than a char then the xor is based on the first (top left) tile.

		// TODO: this should be easier to do...
		int x2 = pixmap.calc_ax(x + CHAR_WIDTH - 1) + (1 << bits_per_color) - 1 + 1;
		int y2 = pixmap.calc_ay(y + CHAR_HEIGHT - 1, pixmap.attrheight) + 1;
		int x1 = pixmap.calc_ax(x);
		int y1 = pixmap.calc_ay(y, pixmap.attrheight);

		if (show)
		{
			uint fg_color  = pixmap.attributes.get_pixel(x1 + int(fgcolor), y1);
			uint bg_color  = pixmap.attributes.get_pixel(x1 + int(bgcolor), y1);
			cursorXorValue = fg_color ^ bg_color;
			if (cursorXorValue == 0) cursorXorValue = ~0u;
		}

		uint8* start = pixmap.attributes.pixmap + y * pixmap.attributes.row_offset;
		Graphics::bitblit::xor_rect_of_bits(
			start, x << bits_per_color, pixmap.attributes.row_offset, (x2 - x1) << bits_per_color, (y2 - y1),
			Graphics::flood_filled_color<ColorDepth(CM)>(cursorXorValue));
	}

	cursorVisible = show;
}

template<ColorMode CM>
void PicoTerm<CM>::showCursor()
{
	if (cursorVisible) return;

	validateCursorPosition();
	show_cursor(true);
}

template<ColorMode CM>
void PicoTerm<CM>::scrollScreenUp(int rows /*char*/)
{
	// doesn't update the cursor position

	hideCursor();
	if (rows > 0) draw_engine.scrollScreen(0, -rows * CHAR_HEIGHT, bgcolor);
}

template<ColorMode CM>
void PicoTerm<CM>::scrollScreenDown(int rows /*char*/)
{
	// doesn't update the cursor position

	hideCursor();
	if (rows > 0) draw_engine.scrollScreen(0, +rows * CHAR_HEIGHT, bgcolor);
}

template<ColorMode CM>
void PicoTerm<CM>::scrollScreenLeft(int cols)
{
	// scroll screen left

	hideCursor();
	if (cols > 0) draw_engine.scrollScreen(-cols * int(CHAR_WIDTH), 0, bgcolor);
}

template<ColorMode CM>
void PicoTerm<CM>::scrollScreenRight(int cols)
{
	// scroll screen right

	hideCursor();
	if (cols > 0) draw_engine.scrollScreen(+cols * int(CHAR_WIDTH), 0, bgcolor);
}

template<ColorMode CM>
void PicoTerm<CM>::validateCursorPosition()
{
	// validate cursor position
	// moves cursor into previous/next line if the column is out of screen
	// scrolls the screen up or down if the row is out of screen
	// afterwards, the cursor is inside the screen

	// col := in range [0 .. [screen_width
	// row := in range [0 .. [screen_height

	hideCursor();

	if (unlikely(uint(col) >= uint(screen_width)))
	{
		col = int8(col - 0x40) + 0x40; // clamp to range -0x40 .. 0x80+0x40 (assuming 128 cols max)
		while (col < 0)
		{
			col += screen_width;
			row -= dy;
		}
		while (col >= screen_width)
		{
			col -= screen_width;
			row += dy;
		}
	}

	if (unlikely(uint(row) >= uint(screen_height)))
	{
		row = int8(row - 0x60) + 0x60; // clamp to range -0x60 .. 0x40+0x60 (assuming 64 rows max)
		if (row < 0)
		{
			scrollScreenDown(-row);
			row = 0;
		}
		else
		{
			scrollScreenUp(row - (screen_height - 1));
			row = screen_height - 1;
		}
	}
}

template<ColorMode CM>
void PicoTerm<CM>::readBmp(CharMatrix bmp)
{
	// read BMP of character cell from screen.
	// all pixels which are not the bgcolor will be set in the bmp[].
	// read character cell at cursor position.
	// increment col (as for printing, except no double width/height attribute)

	validateCursorPosition();

	int x = col++ * CHAR_WIDTH;
	int y = row * CHAR_HEIGHT;
	draw_engine.readBmpFromScreen(x, y, CHAR_WIDTH, CHAR_HEIGHT, bmp, bgcolor);
}

template<ColorMode CM>
void PicoTerm<CM>::writeBmp(CharMatrix bmp, uint8 attr)
{
	// write BMP to screen applying the 'late' attributes:
	//	+ double width
	//	+ double height
	//	+ overprint
	//  - bold, italic, underline, inverted and graphics must already be applied
	// at cursor position
	// increment col

	if (unlikely(attr & ATTR_DOUBLE_WIDTH))
	{
		CharMatrix bmp2;

		// if in last column, don't print 2 half characters:
		validateCursorPosition();
		if (col == screen_width - 1)
		{
			memset(bmp2, 0, CHAR_HEIGHT);
			uint8 attr2 = attr & ~ATTR_DOUBLE_WIDTH;

			// if in top-right corner don't scroll screen down:
			if (row == 0) attr2 &= ~ATTR_DOUBLE_HEIGHT;

			// clear to eol and incr col:
			writeBmp(bmp2, attr2);
		}

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp2[i] = dblw[bmp[i] >> 4]; }
		writeBmp(bmp2, attr & ~ATTR_DOUBLE_WIDTH);

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp[i] = dblw[bmp[i] & 15]; }
	}

	if (unlikely(attr & ATTR_DOUBLE_HEIGHT))
	{
		CharMatrix bmp2;

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp2[i] = bmp[i / 2]; }
		row--;
		writeBmp(bmp2, attr & ~ATTR_DOUBLE_HEIGHT);
		row++;
		col--;

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp[i] = bmp[CHAR_HEIGHT / 2 + i / 2]; }
	}

	validateCursorPosition();

	int x = col++ * CHAR_WIDTH;
	int y = row * CHAR_HEIGHT;
	draw_engine.writeBmpToScreen(
		x, y, CHAR_WIDTH, CHAR_HEIGHT, bmp, fgcolor, attr & ATTR_OVERPRINT ? DONT_CLEAR : bgcolor);
}

template<ColorMode CM>
void PicoTerm<CM>::getCharMatrix(CharMatrix charmatrix, Char c)
{
	// get character matrix of character c
	// returns ASCII, UDG or LATIN-1 characters
	// or GRAPHICS CHARACTERS if attribute ATTR_GRAPHICS_CHARACTERS is set

	// valid range:
	// 0x20 .. 0x7F	ASCII
	// 0x80 .. 0x9F	UDG
	// 0xA0 .. 0xFF	LATIN-1

	if (attributes & ATTR_GRAPHICS_CHARACTERS) { getGraphicsCharMatrix(charmatrix, c); }
	else
	{
		uint o = 127 - 32; // default = pattern of 'delete'

		if (c < 0x20) {}
		else if (c < 0x80) { o = c - 32; }
		else if (c < 0xA0)
		{
			// readUDG(c,charmatrix);		LCD: read from eeprom
		}
		else if (c <= 0xFF) { o = c - 64; }

		const uchar* p = font + o * CHAR_HEIGHT;
		memcpy(charmatrix, p, CHAR_HEIGHT);
	}
}

template<ColorMode CM>
void PicoTerm<CM>::getGraphicsCharMatrix(CharMatrix charmatrix, Char c)
{
	// calculate graphics character c
	// returns various block or line graphics

	uint8 i = 0, b, n;

	//	if(c<0x20)			// control codes
	//	{}
	// 	else

	if (c < 0x30) // 4/4 Blockgrafik schwarz/weiß
	{
		b = (c & 8 ? 0xF0 : 0) + (c & 4 ? 0x0F : 0);
		c = (c & 2 ? 0xF0 : 0) + (c & 1 ? 0x0F : 0);
		n = 6;
		goto b;
	}
	else if (c < 0x40) // 4/4 Blockgrafik grau/weiß
	{
		getGraphicsCharMatrix(charmatrix, c - 16);
		c = 0xAA;
		do {
			charmatrix[i++] &= c;
			c = ~c;
		}
		while (i < 12);
	}
	else if (c < 0x50) // 4/4 Blockgrafik schwarz/grau
	{
		getGraphicsCharMatrix(charmatrix, c - 32);
		c = 0xAA;
		do {
			charmatrix[i++] |= c;
			c = ~c;
		}
		while (i < 12);
	}
	else if (c < 0x58) // Balkengrafik von links	schwarz/weiß
	{
		c = uint8(0xff << (0x57 - c));
		goto c;
	}
	else if (c < 0x60) // Balkengrafik von rechts	schwarz/weiß
	{
		c = 0xff >> (0x5F - c);
		goto c;
	}
	else if (c < 0x6C) // Balkengrafik von unten
	{
		n = uint8(0x6B - c);
		b = 0x00;
		c = 0xFF;
		goto b;
	}
	else if (c < 0x78) // Balkengrafik von oben
	{
		n = uint8(c - 0x6B);
		b = 0xFF;
		c = 0x00;
	b:
		while (i < n) charmatrix[i++] = b;
	c:
		while (i < 12) charmatrix[i++] = uint8(c);
	}

	//	else if (c<0x80)	// 8 unused
	//	{}

	//	else if (c<0xB0)	// 48 unused
	//	{}

	else if (c < 0xF0) // 80 Liniengrafiken: dünne und dicke Linien
	{				   // total: 3^4 = 81, aber 4 x ohne => use 'space' oder Grafik '\0'
		uint8 a, d;
		c += 1 - 0xB0;
		a = uint8(c / 27); // a=0/1/2 => Strich links ohne/dünn/dick
		b = (c / 9) % 3;   // b				 oben
		d = c % 3;		   // d				 unten
		c = (c / 3) % 3;   // c				 rechts

		n = b == 1 ? 0x08 : b ? 0x18 : 0; // vert. Strich oben
		while (i < 6) charmatrix[i++] = n;

		n = d == 1 ? 0x08 : d ? 0x18 : 0; // vert. Strich unten
		while (i < 12) charmatrix[i++] = n;

		if (a) charmatrix[5] |= 0xF8; // hor. Strich links
		if (a == 2) charmatrix[6] |= 0xF8;

		if (c) charmatrix[5] |= 0x0F; // hor. Strich rechts
		if (c == 2)
		{
			charmatrix[6] |= 0x1F;
			charmatrix[5] |= 0x1F;
		}
	}
}

template<ColorMode CM>
void PicoTerm<CM>::applyAttributes(CharMatrix bmp)
{
	// apply the simple attributes to a character matrix
	// - BOLD
	// - UNDERLINE
	// - ITALIC
	// - INVERTED

	uint8 a = attributes;

	if (int8(a) > 0) // any attr except graphics_char_mode set?
	{
		if (a & ATTR_BOLD)
		{
			for (int i = 0; i < 12; i++) bmp[i] |= bmp[i] >> 1;
		}
		if (a & ATTR_UNDERLINE) { bmp[10] = 0xff; }
		if (a & ATTR_ITALIC)
		{
			for (int i = 0; i < 4; i++) bmp[i] >>= 1;
			for (int i = 8; i < 12; i++) bmp[i] <<= 1;
		}
		if (a & ATTR_INVERTED)
		{
			uint8 i = 12;
			while (i--) bmp[i] = ~bmp[i];
		}
	}
}

template<ColorMode CM>
void PicoTerm<CM>::eraseRect(int row, int col, int rows, int cols)
{
	// erase a rectangular area on the screen

	hideCursor();

	if (rows > 0 && cols > 0)
	{
		int x = col * CHAR_WIDTH;
		int y = row * CHAR_HEIGHT;
		draw_engine.fillRect(Rect(x, y, cols * CHAR_WIDTH, rows * CHAR_HEIGHT), bgcolor, 0);
	}
}

template<ColorMode CM>
void PicoTerm<CM>::setWindow(int row, int col, int rows, int cols)
{
	// limit terminal to a rectangular area on the screen
	// top,left = cursor position

	validateCursorPosition();

	(void)row;
	(void)col;
	if (rows > 0 && cols > 0) { TODO(); }
	else // reset to full screen
	{}
	this->row = 0;
	this->col = 0;
}

template<ColorMode CM>
void PicoTerm<CM>::copyRect(int src_row, int src_col, int dest_row, int dest_col, int rows, int cols)
{
	hideCursor();

	if (rows > 0 && cols > 0)
	{
		draw_engine.copyRect(
			Point(src_col * CHAR_WIDTH, src_row * CHAR_HEIGHT), Point(dest_col * CHAR_WIDTH, dest_row * CHAR_HEIGHT),
			Dist(cols * CHAR_WIDTH, rows * CHAR_HEIGHT));
	}
}

template<ColorMode CM>
void PicoTerm<CM>::reset()
{
	draw_engine.~DrawEngine();
	new (&draw_engine) DrawEngine<CM>(pixmap, colormap);

	// ALL SETTINGS := DEFAULT, CLS, HOME CURSOR

	screen_width  = pixmap.width / CHAR_WIDTH;
	screen_height = pixmap.height / CHAR_HEIGHT;

	bgcolor = INVERTED ? 0x00ffffff : 0;
	fgcolor = INVERTED ? 0 : 0x00ffffff;

	resetColorMap(get_colordepth(CM), colormap);
	cls();
}

template<ColorMode CM>
void PicoTerm<CM>::cls()
{
	// CLS, HOME CURSOR, RESET ATTR

	row = col = 0;
	dx = dy		  = 1;
	attributes	  = 0;
	cursorVisible = false;

	draw_engine.clearScreen(bgcolor);
}

template<ColorMode CM>
void PicoTerm<CM>::moveToPosition(int row, int col) noexcept
{
	hideCursor();
	this->row = row;
	this->col = col;
}

template<ColorMode CM>
void PicoTerm<CM>::moveToCol(int col) noexcept
{
	hideCursor();
	this->col = col;
}

template<ColorMode CM>
void PicoTerm<CM>::pushCursorPosition()
{
	//TODO: visibility, window
	pushedRow  = row;
	pushedCol  = col;
	pushedAttr = attributes;
}

template<ColorMode CM>
void PicoTerm<CM>::popCursorPosition()
{
	row = pushedRow;
	col = pushedCol;
	setPrintAttributes(pushedAttr);
}

template<ColorMode CM>
void PicoTerm<CM>::setPrintAttributes(uint8 attr)
{
	attributes = attr;
	dx		   = attr & ATTR_DOUBLE_WIDTH ? 2 : 1;
	dy		   = attr & ATTR_DOUBLE_HEIGHT ? 2 : 1;
}

template<ColorMode CM>
void PicoTerm<CM>::cursorLeft(int count) // BS
{
	// scrolls

	hideCursor();
	if (count > 0)
		while (count--)
		{
			col -= dx;
			if (int(col) < 0)
			{
				col += screen_width;
				row -= dy;
			}
		}
}

template<ColorMode CM>
void PicoTerm<CM>::cursorRight(int count) // FF
{
	// scrolls

	hideCursor();
	if (count > 0)
		while (count--)
		{
			col += dx;
			if (col > screen_width)
			{
				col -= screen_width;
				row += dy;
			}
		}
}

template<ColorMode CM>
void PicoTerm<CM>::cursorUp(int count)
{
	// scrolls

	hideCursor();
	if (count > 0) row -= dy * count;
}

template<ColorMode CM>
void PicoTerm<CM>::cursorDown(int count) // NL
{
	// scrolls

	hideCursor();
	if (count > 0) row += dy * count;
}

template<ColorMode CM>
void PicoTerm<CM>::cursorTab(int count)
{
	// scrolls

	hideCursor();
	if (count > 0)
		while (count--)
		{
			col = (col / 8 + 1) * 8;
			if (col > screen_width)
			{
				col -= screen_width;
				row += dy;
			}
		}
}

template<ColorMode CM>
void PicoTerm<CM>::cursorReturn()
{
	// COL := 0

	hideCursor();
	col = 0;
}

template<ColorMode CM>
void PicoTerm<CM>::clearToEndOfLine()
{
	hideCursor();
	if (col < screen_width && row < screen_height) eraseRect(row, col, 1 /*rows*/, screen_width - col /*cols*/);
}

template<ColorMode CM>
void PicoTerm<CM>::printCharMatrix(CharMatrix charmatrix, int count)
{
	applyAttributes(charmatrix);

	while (count--) { writeBmp(charmatrix, attributes); }
}

template<ColorMode CM>
void PicoTerm<CM>::printChar(Char c, int count)
{
	uint8 charmatrix[CHAR_HEIGHT];
	getCharMatrix(charmatrix, c);
	printCharMatrix(charmatrix, count);
}

template<ColorMode CM>
void PicoTerm<CM>::printText(cstr s)
{
	// print printable text string.
	// no control characters.

	while (*s)
	{
		CharMatrix charmatrix;
		getCharMatrix(charmatrix, *s++);
		printCharMatrix(charmatrix, 1);
	}
}

static constexpr bool is_fup(Char c) noexcept { return schar(c) < schar(0xc0); }

template<ColorMode CM>
void PicoTerm<CM>::print(cstr s, bool auto_crlf)
{
	assert(s);

	auto getc = [&]() -> uchar { return *s ? *s++ : 0; };

loop:
	int repeat_count = 1;

loop_repeat:

a:
	Char c = *s++;
b:
	if (USE_WIDECHARS && c >= 0x80) // decode utf-8
	{
		if (is_fup(c)) goto a; // ignore unexpected fup

		Char c1 = c;
		c		= getc();
		if (!is_fup(c)) goto b; // ignore broken char

		if (c1 < 0xE0) // 1 fup:
		{
			c = Char(((c1 & 0x1F) << 6) + (c & 0x3F));
		}
		else
		{
			Char c2 = c;
			c		= getc();
			if (!is_fup(c)) goto b; // ignore broken char

			if (c1 < 0xF0) // 2 fup:
			{
				c = Char(((c1 & 0x0F) << 12) + ((c2 & 0x3F) << 6) + (c & 0x3F));
			}
			else
			{
				c = '_'; // too large for UCS2
			}
		}
	}

	if (c >= 32) // printable char
	{
		if (c != 127) { printChar(c, repeat_count); }
		else // delete
		{
			cursorLeft(repeat_count);
			printChar(' ', repeat_count);
			cursorLeft(repeat_count);
		}

		goto loop;
	}

	switch (c)
	{
	case 0: return;
	case CLS: // CLS, HOME CURSOR, RESET ATTR
	{
		cls();
		break;
	}
	case SET_WINDOW: // SET_WINDOW <rows> <cols>
	{
		validateCursorPosition();
		uchar rows = getc();
		uchar cols = getc();
		setWindow(row, col, rows, cols);
		break;
	}
	case MOVE_TO_POSITION: // MOVE_TO_POSITION, <row>, <col>
	{
		hideCursor();
		row = getc();
		col = getc();
		break;
	}
	case MOVE_TO_COL: // MOVE_TO_COL, <row>
	{
		hideCursor();
		col = getc();
		break;
	}
	case PUSH_CURSOR_POSITION:
	{
		pushCursorPosition();
		break;
	}
	case POP_CURSOR_POSITION:
	{
		popCursorPosition();
		break;
	}
	case SHOW_CURSOR: // (BELL)
	{
		showCursor();
		break;
	}
	case CURSOR_LEFT: // (BS) scrolls
	{
		cursorLeft(repeat_count);
		break;
	}
	case TAB: //		scrolls
	{
		cursorTab(repeat_count);
		break;
	}
	case CURSOR_DOWN: // (NL) scrolls
	{
		if (auto_crlf) col = 0;
		cursorDown(repeat_count);
		break;
	}
	case CURSOR_UP: //		scrolls
	{
		cursorUp(repeat_count);
		break;
	}
	case CURSOR_RIGHT: // (FF)	scrolls
	{
		cursorRight(repeat_count);
		break;
	}
	case RETURN: // COL := 0
	{
		cursorReturn();
		break;
	}
	case CLEAR_TO_END_OF_LINE:
	{
		clearToEndOfLine();
		break;
	}
	case SCROLL_SCREEN: // SCROLL SCREEN u/d/l/r
	{
		uchar dir = getc();
		if (dir == 'u') scrollScreenUp(repeat_count);
		else if (dir == 'd') scrollScreenDown(repeat_count);
		else if (dir == 'l') scrollScreenLeft(repeat_count);
		else if (dir == 'r') scrollScreenRight(repeat_count);
		break;
	}
	case REPEAT_NEXT_CHAR:
	{
		repeat_count = getc();
		goto loop_repeat;
	}
	case SET_ATTRIBUTES:
	{
		setPrintAttributes(getc());
		break;
	}
	case PRINT_INLINE_GLYPH: // PRINT INLINE CHARACTER BMP
	{
		CharMatrix charmatrix;
		for (int i = 0; i < CHAR_HEIGHT; i++) charmatrix[i] = getc();
		printCharMatrix(charmatrix, repeat_count);
		break;
	}
	default:
	{
		char txt[8];
		sprintf(txt, "[$%02X]", c);
		printText(txt);
		break;
	}
	}

	goto loop;
}

template<ColorMode CM>
void PicoTerm<CM>::printf(cstr fmt, ...)
{
	constexpr int max_cnt = 79;
	char		  bu[max_cnt + 1];

	va_list va;
	va_start(va, fmt);

	va_list va2;
	va_copy(va2, va);
	int cnt = vsnprintf(bu, max_cnt + 1, fmt, va2);
	va_end(va2);
	assert(cnt >= 0);

	if (cnt > max_cnt)
	{
		char* xbu = new (std::nothrow) char[size_t(cnt + 1)];
		if (!xbu) goto p;
		vsnprintf(xbu, size_t(cnt + 1), fmt, va);
		print(xbu, true);
		delete[] xbu;
	}
	else
	{
	p:
		print(bu, true);
	}

	va_end(va);
}

template<ColorMode CM>
char* PicoTerm<CM>::identify(char* txt)
{
	sprintf(
		txt, "PicoTerm,gfx=%u*%u,txt=%u*%u,csz=%u*%u", pixmap.width, pixmap.height, screen_width, screen_height,
		CHAR_WIDTH, CHAR_HEIGHT);
	if (get_attrmode(CM) != attrmode_none)
		sprintf(strchr(txt, 0), ",asz=%u*%u", 1 << get_attrwidth(CM), pixmap.attrheight);
	sprintf(strchr(txt, 0), ",%s", tostr(get_colordepth(CM)));
	return txt;
}


// instantiate them all, unless modified they are only compiled once:
// otherwise we'll need to define which to implement which leads to idiotic problems.

#if 0
template class PicoTerm<DEFAULT_COLORMODE>;
//template class PicoTerm<colormode_i8>;
//template class PicoTerm<colormode_rgb>;
//template class PicoTerm<colormode_a1w8_i8>;
//template class PicoTerm<colormode_a1w8_rgb>;
//template class PicoTerm<colormode_a2w8_i8>;
//template class PicoTerm<colormode_a2w8_rgb>;
#else
template class PicoTerm<colormode_i1>;
template class PicoTerm<colormode_i2>;
template class PicoTerm<colormode_i4>;
template class PicoTerm<colormode_i8>;
template class PicoTerm<colormode_rgb>;
template class PicoTerm<colormode_a1w1_i4>;
template class PicoTerm<colormode_a1w1_i8>;
template class PicoTerm<colormode_a1w1_rgb>;
template class PicoTerm<colormode_a1w2_i4>;
template class PicoTerm<colormode_a1w2_i8>;
template class PicoTerm<colormode_a1w2_rgb>;
template class PicoTerm<colormode_a1w4_i4>;
template class PicoTerm<colormode_a1w4_i8>;
template class PicoTerm<colormode_a1w4_rgb>;
template class PicoTerm<colormode_a1w8_i4>;
template class PicoTerm<colormode_a1w8_i8>;
template class PicoTerm<colormode_a1w8_rgb>;
template class PicoTerm<colormode_a2w1_i4>;
template class PicoTerm<colormode_a2w1_i8>;
template class PicoTerm<colormode_a2w1_rgb>;
template class PicoTerm<colormode_a2w2_i4>;
template class PicoTerm<colormode_a2w2_i8>;
template class PicoTerm<colormode_a2w2_rgb>;
template class PicoTerm<colormode_a2w4_i4>;
template class PicoTerm<colormode_a2w4_i8>;
template class PicoTerm<colormode_a2w4_rgb>;
template class PicoTerm<colormode_a2w8_i4>;
template class PicoTerm<colormode_a2w8_i8>;
template class PicoTerm<colormode_a2w8_rgb>;
#endif


} // namespace kipili
