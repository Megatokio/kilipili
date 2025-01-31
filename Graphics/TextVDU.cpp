// Copyright (c) 2012 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "TextVDU.h"
#include "USBHost/USBKeyboard.h"
#include "cstrings.h"
#include "utilities/Trace.h"
#include <memory>
#include <stdarg.h>
#include <string.h>

namespace kio::Graphics
{

// ------------------------------------------------------------
// 				Const Data:
// ------------------------------------------------------------

// lsbit is left => revert bits
#define r(b)                                                                                                   \
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


TextVDU::TextVDU(CanvasPtr _pixmap) noexcept :
	pixmap(std::move(_pixmap)),
	colormode(pixmap->colormode),
	attrheight(pixmap->attrheight),
	colordepth(get_colordepth(colormode)),	// 0 .. 4  log2 of bits per color in attributes[]
	attrmode(get_attrmode(colormode)),		// -1 .. 1 log2 of bits per color in pixmap[]
	attrwidth(get_attrwidth(colormode)),	// 0 .. 3  log2 of width of tiles
	bits_per_color(uint8(1 << colordepth)), // bits per color in pixmap[] or attributes[]
	bits_per_pixel(is_attribute_mode(colormode) ? uint8(1 << attrmode) : bits_per_color), // bpp in pixmap[]
	cols(pixmap->width / CHAR_WIDTH),
	rows(pixmap->height / CHAR_HEIGHT)
{
	cursorVisible = false;
	reset();
}

void TextVDU::reset() noexcept
{
	// all settings = default, home cursor
	// does not clear screen

	hideCursor();

	bgcolor = default_bgcolor; // white / light
	fgcolor = default_fgcolor; // black / dark
	bg_ink	= 0;
	fg_ink	= 1;

	row = col	 = 0;
	scroll_count = 0;
	dx = dy	   = 1;
	attributes = NORMAL;
}

void TextVDU::cls() noexcept
{
	// CLS, HOME CURSOR, RESET ATTR

	row = col	 = 0;
	scroll_count = 0;
	dx = dy		  = 1;
	attributes	  = NORMAL;
	cursorVisible = false;

	pixmap->clear(bgcolor);
}

void TextVDU::identify() noexcept
{
	// size=400*300, text=50*25, char=8*12, colors=rgb
	// size=400*300, text=50*25, char=8*12, colors=i8, attr=8*12

	cstr colors = colordepth == colordepth_rgb ? "rgb" : tostr(colordepth);

	printf("size=%i*%i, text=%i*%i, ", pixmap->width, pixmap->height, cols, rows);
	printf("char=%i*%i, colors=%s", CHAR_WIDTH, CHAR_HEIGHT, colors);
	if (attrmode != attrmode_none) printf(", attr=%u*%u", 1u << attrwidth, attrheight);
	newLine();
}

void TextVDU::show_cursor(bool show) noexcept
{
	// for all pixels: color ^= fgcolor ^ bgcolor

	if (show)
	{
		cursorXorColor = fgcolor ^ bgcolor; // TODO: ink vs. color everywhere!
		if (cursorXorColor == 0) cursorXorColor = ~0u;
	}

	pixmap->xorRect(col * CHAR_WIDTH, row * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT, cursorXorColor);
	cursorVisible = show;
}

void TextVDU::showCursor(bool on) noexcept
{
	if (cursorVisible == on) return;
	if (on) validate_hpos(false);
	show_cursor(on);
}

void TextVDU::hideCursor() noexcept
{
	if (cursorVisible) show_cursor(false);
}

void TextVDU::validate_hpos(bool col80ok) noexcept
{
	// wraps & scrolls

	assert(!cursorVisible);
	if unlikely (uint(col) >= uint(cols) + col80ok)
	{
		while (col < 0)
		{
			col += cols;
			row -= dy;
		}
		while (col >= cols + col80ok)
		{
			col -= cols;
			row += dy;
		}
		validate_vpos();
	}
}

void TextVDU::validate_vpos() noexcept
{
	// scrolls

	assert(!cursorVisible);
	if unlikely (uint(row) >= uint(rows))
	{
		if (row < 0)
		{
			scroll_count += row;
			scrollScreenDown(-row);
			row = 0;
		}
		else
		{
			scroll_count += row - (rows - 1);
			scrollScreenUp(row - (rows - 1));
			row = rows - 1;
		}
	}
}

void TextVDU::validateCursorPosition(bool col80ok) noexcept
{
	// validate cursor position
	// moves cursor into previous/next line if the column is out of screen
	// scrolls the screen up or down if the row is out of screen
	// afterwards, the cursor is inside the screen
	// except if col80ok then allow the cursor in col = screen_width

	// col := in range [0 .. [screen_width
	// row := in range [0 .. [screen_height

	if (cursorVisible) return;
	validate_hpos(col80ok);
	validate_vpos();
}

void TextVDU::limitCursorPosition() noexcept
{
	// limit cursor position: don't wrap, don't scroll

	if (cursorVisible) return;
	limit(0, col, cols - 1);
	limit(0, row, rows - 1);
}

void TextVDU::moveTo(int row, int col, AutoWrap auto_wrap) noexcept
{
	// auto_wrap=0: stop at border.
	// auto_wrap=1: wrap & scroll, col80ok.

	hideCursor();
	this->row = row;
	this->col = col;
	if (auto_wrap) validateCursorPosition(true);
	else limitCursorPosition();
}

void TextVDU::moveToCol(int col, AutoWrap auto_wrap) noexcept
{
	// auto_wrap=0: stop at border.
	// auto_wrap=1: wrap & scroll, col80ok.

	hideCursor();
	this->col = col;
	if (auto_wrap) validate_hpos(true);
	else limit(0, this->col, cols - 1);
}

void TextVDU::moveToRow(int row, AutoWrap auto_wrap) noexcept
{
	// auto_wrap=0: stop at border.
	// auto_wrap=1: wrap & scroll.

	hideCursor();
	this->row = row;
	if (auto_wrap) validate_vpos();
	else limit(0, this->row, rows - 1);
}

void TextVDU::cursorLeft(int count, AutoWrap auto_wrap) noexcept
{
	moveToCol(col - max(count * dx, 0), auto_wrap); //
}

void TextVDU::cursorRight(int count, AutoWrap auto_wrap) noexcept
{
	// if auto_wrap then col80ok

	moveToCol(col + max(count * dx, 0), auto_wrap);
}

void TextVDU::cursorUp(int count, AutoWrap auto_wrap) noexcept
{
	moveToRow(row - max(count * dx, 0), auto_wrap); //
}

void TextVDU::cursorDown(int count, AutoWrap auto_wrap) noexcept
{
	moveToRow(row + max(count * dx, 0), auto_wrap); //
}

void TextVDU::cursorTab(int count) noexcept
{
	// scrolls, allows col = cols
	// note: if cols%8 != 0 then there is a last tab stop at cols
	// TODO: auto_wrap flag

	hideCursor();
	if (count > 0)
	{
		if (col >= cols)
		{
			col = 0;
			row += dy;
		}

		col = ((col >> 3) + count) << 3;

		int xcols = (cols + 7) & ~7;
		while (col > xcols)
		{
			row += dy;
			col -= xcols;
		}

		if (col > cols) col = cols;

		validate_vpos();
	}
}

void TextVDU::cursorReturn() noexcept
{
	// COL := 0

	hideCursor();
	col = 0;
}

void TextVDU::newLine() noexcept
{
	hideCursor();
	col = 0;
	row += dy;
	validate_vpos();
}

void TextVDU::clearRect(int row, int col, int rows, int cols) noexcept
{
	// erase a rectangular area on the screen

	hideCursor();

	if (rows > 0 && cols > 0)
	{
		int x = col * CHAR_WIDTH;
		int y = row * CHAR_HEIGHT;
		pixmap->fillRect(Rect(x, y, cols * CHAR_WIDTH, rows * CHAR_HEIGHT), bgcolor, bg_ink);
	}
}

void TextVDU::scrollRect(int row, int col, int rows, int cols, int dy, int dx) noexcept
{
	if unlikely (row < 0) { rows += row, row = 0; }
	if unlikely (row + rows > this->rows) rows = this->rows - row;
	if unlikely (col < 0) { cols += col, col = 0; }
	if unlikely (col + cols > this->cols) cols = this->cols - col;

	int h = rows - abs(dy);
	int w = cols - abs(dx);

	if (w <= 0 || h <= 0) return clearRect(row, col, rows, cols);

	coord qx = dx >= 0 ? 0 : -dx;
	coord zx = dx >= 0 ? +dx : 0;
	coord qy = dy >= 0 ? 0 : -dy;
	coord zy = dy >= 0 ? +dy : 0;

	copyRect(row + zy, col + zx, row + qy, col + qx, h, w);

	if (dx > 0) clearRect(row, col, rows, +dx);
	if (dx < 0) clearRect(row, col + w, rows, -dx);
	if (dy > 0) clearRect(row, col, +dy, cols);
	if (dy < 0) clearRect(row + h, col, -dy, cols);
}

void TextVDU::scrollRectLeft(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, 0, -dist);
}

void TextVDU::scrollRectRight(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, 0, +dist);
}

void TextVDU::scrollRectUp(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, -dist, 0);
}

void TextVDU::scrollRectDown(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, +dist, 0);
}

void TextVDU::insertRows(int n) noexcept { scrollRectDown(row, 0, rows - row, cols, n); }

void TextVDU::deleteRows(int n) noexcept { scrollRectUp(row, 0, rows - row, cols, n); }

void TextVDU::insertColumns(int n) noexcept { scrollRectRight(0, col, rows, cols - col, n); }

void TextVDU::deleteColumns(int n) noexcept { scrollRectLeft(0, col, rows, cols - col, n); }

void TextVDU::insertChars(int n) noexcept { scrollRectRight(row, col, 1, cols - col, n); }

void TextVDU::deleteChars(int n) noexcept { scrollRectLeft(row, col, 1, cols - col, n); }

void TextVDU::clearToStartOfLine(bool incl_cpos) noexcept
{
	// allow col80, even if incl_cpos = true

	clearRect(row, 0, 1, col + incl_cpos);
}

void TextVDU::clearToStartOfScreen(bool incl_cpos) noexcept
{
	// allow col80, even if incl_cpos = true

	clearToStartOfLine(incl_cpos);
	clearRect(0, 0, row, cols);
}

void TextVDU::clearToEndOfLine() noexcept
{
	// allow col80
	// this allows to print an arbitrary string up to the last char and clear to eol.

	clearRect(row, col, 1, cols - col);
}

void TextVDU::clearToEndOfScreen() noexcept
{
	// allow col80
	// this allows to print an arbitrary string up to the last char and clear to end of screen
	// without scrolling and inserting a new empty line.

	clearToEndOfLine();
	clearRect(row + 1, 0, rows - (row + 1), cols);
}

void TextVDU::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
{
	hideCursor();

	if (rows > 0 && cols > 0)
	{
		pixmap->copyRect(
			dest_col * CHAR_WIDTH, dest_row * CHAR_HEIGHT, src_col * CHAR_WIDTH, src_row * CHAR_HEIGHT,
			cols * CHAR_WIDTH, rows * CHAR_HEIGHT);
	}
}

void TextVDU::scrollScreen(int dy /*chars*/, int dx /*chars*/) noexcept
{
	int w = (cols - abs(dx));
	int h = (rows - abs(dy));

	if (w <= 0 || h <= 0) return clearRect(0, 0, rows, cols);

	coord qx = dx >= 0 ? 0 : -dx;
	coord zx = dx >= 0 ? +dx : 0;
	coord qy = dy >= 0 ? 0 : -dy;
	coord zy = dy >= 0 ? +dy : 0;

	copyRect(zy, zx, qy, qx, h, w);

	if (dx > 0) clearRect(0, 0, rows, +dx);
	if (dx < 0) clearRect(0, w, rows, -dx);
	if (dy > 0) clearRect(0, 0, +dy, cols);
	if (dy < 0) clearRect(h, 0, -dy, cols);
}

void TextVDU::setAttributes(uint add, uint remove) noexcept
{
	attributes = Attributes((attributes & ~remove) | add);
	dx		   = attributes & DOUBLE_WIDTH ? 2 : 1;
	dy		   = attributes & DOUBLE_HEIGHT ? 2 : 1;
}

void TextVDU::applyAttributes(CharMatrix bmp) noexcept
{
	// apply the simple attributes to a character matrix
	// - BOLD
	// - UNDERLINE
	// - ITALIC
	// - INVERTED

	uint8 a = attributes;

	if (int8(a) > 0) // any attr except graphics_char_mode set?
	{
		if (a & BOLD)
		{
			for (int i = 0; i < 12; i++) bmp[i] |= bmp[i] >> 1;
		}
		if (a & UNDERLINE) { bmp[10] = 0xff; }
		if (a & ITALIC)
		{
			for (int i = 0; i < 4; i++) bmp[i] >>= 1;
			for (int i = 8; i < 12; i++) bmp[i] <<= 1;
		}
		if (a & INVERTED)
		{
			uint8 i = 12;
			while (i--) bmp[i] = ~bmp[i];
		}
	}
}

void TextVDU::readBmp(CharMatrix bmp, bool use_fgcolor) noexcept
{
	// read BMP of character cell from screen.
	// read character cell at cursor position.
	// increment col (as for printing, except no double width/height attribute)
	// use_fgcolor=1: set bits for pixels in fgcolor
	// use_fgcolor=0: clr bits for pixels in bgcolor

	hideCursor();
	validate_hpos(false);
	assert(row >= 0 && row < rows);

	int x = col++ * CHAR_WIDTH;
	int y = row * CHAR_HEIGHT;
	pixmap->readBmp(x, y, bmp, 1 /*row_offset*/, CHAR_WIDTH, CHAR_HEIGHT, use_fgcolor ? fgcolor : bgcolor, use_fgcolor);
}

void TextVDU::writeBmp(CharMatrix bmp, uint8 attr) noexcept
{
	// write BMP to screen applying the 'late' attributes:
	//	+ double width
	//	+ double height
	//	+ overprint
	//  - bold, italic, underline, inverted and graphics must already be applied
	// at cursor position
	// increment col

	hideCursor();
	validate_hpos(false);

	if unlikely (attr & DOUBLE_WIDTH)
	{
		CharMatrix bmp2;

		// if in last column, don't print 2 half characters:
		if (col == cols - 1)
		{
			memset(bmp2, 0, CHAR_HEIGHT);
			uint8 attr2 = attr & ~DOUBLE_WIDTH;

			// if in top-right corner don't scroll screen down:
			if (row == 0) attr2 &= ~DOUBLE_HEIGHT;

			// clear to eol and incr col:
			writeBmp(bmp2, attr2);
			validate_hpos(false);
			assert(col == 0);
		}

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp2[i] = dblw[bmp[i] >> 4]; }
		writeBmp(bmp2, attr &= ~DOUBLE_WIDTH);

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp[i] = dblw[bmp[i] & 15]; }
	}

	if unlikely (attr & DOUBLE_HEIGHT)
	{
		CharMatrix bmp2;

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp2[i] = bmp[i / 2]; }
		row--;
		validate_vpos();
		writeBmp(bmp2, attr & ~DOUBLE_HEIGHT);
		col--;
		row++;

		for (int i = 0; i < CHAR_HEIGHT; i++) { bmp[i] = bmp[CHAR_HEIGHT / 2 + i / 2]; }
	}

	assert(col >= 0 && col < cols);
	assert(row >= 0 && row < rows);

	int x = col++ * CHAR_WIDTH;
	int y = row * CHAR_HEIGHT;

	if (!(attr & TRANSPARENT)) pixmap->fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, bgcolor, bg_ink);
	static_assert(CHAR_WIDTH == 8);
	pixmap->drawChar(x, y, bmp, CHAR_HEIGHT, fgcolor, fg_ink);
}

void TextVDU::getCharMatrix(CharMatrix charmatrix, char cc) noexcept
{
	// get character matrix of character c
	// returns ASCII, UDG or LATIN-1 characters
	// or GRAPHICS CHARACTERS if attribute ATTR_GRAPHICS_CHARACTERS is set

	// valid range:
	// 0x20 .. 0x7F	ASCII
	// 0x80 .. 0x9F	UDG
	// 0xA0 .. 0xFF	LATIN-1

	if (attributes & GRAPHICS) { getGraphicsCharMatrix(charmatrix, cc); }
	else
	{
		uint  o = 127 - 32; // default = pattern of 'delete'
		uchar c = uchar(cc);
		if (c < 0x20) {}
		else if (c < 0x80) { o = c - 32; }
		else if (c < 0xA0) {} // readUDG(c,charmatrix);		TODO:  LCD: read from eeprom
		else { o = c - 64; }

		const uchar* p = font + o * CHAR_HEIGHT;
		memcpy(charmatrix, p, CHAR_HEIGHT);
	}
}

void TextVDU::getGraphicsCharMatrix(CharMatrix charmatrix, char cc) noexcept
{
	// calculate graphics character c
	// returns various block or line graphics

	uchar c = uchar(cc);
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

	else // if (c < 0xF0)   // 80 Liniengrafiken: dünne und dicke Linien
	{	 // total: 3^4 = 81, aber 4 x ohne => use 'space' oder Grafik '\0'
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

	// else {} // 16 unused
}

void TextVDU::printCharMatrix(CharMatrix charmatrix, int count) noexcept
{
	applyAttributes(charmatrix);
	while (count--) { writeBmp(charmatrix, attributes); }
}

void TextVDU::printChar(char c, int count) noexcept
{
	CharMatrix charmatrix;
	getCharMatrix(charmatrix, c);
	printCharMatrix(charmatrix, count);
}

void TextVDU::print(cstr s) noexcept
{
	// print printable text string.
	// control characters: only \t and \n.

	while (char c = *s++)
	{
		if unlikely (uchar(c) < 32)
		{
			if (c == '\n')
			{
				newLine();
				continue;
			}
			if (c == '\t')
			{
				cursorTab();
				continue;
			}
			if (c == '\r')
			{
				cursorReturn();
				continue;
			}
		}

		CharMatrix charmatrix;
		getCharMatrix(charmatrix, c);
		printCharMatrix(charmatrix, 1);
	}
}

void TextVDU::printf(cstr fmt, va_list va) noexcept
{
	// note: caller must call va_start() before and va_end() afterwards.

	constexpr uint bsz = 200;
	char		   bu[bsz];

	va_list va2;
	va_copy(va2, va);
	uint size = uint(vsnprintf(bu, sizeof(bu), fmt, va2));
	va_end(va2);

	if (size < bsz) return print(bu);			   // success
	if (int(size) < 0) return print("{format?!}"); // format error

	// very long text:

	std::unique_ptr<char[]> bp {new (std::nothrow) char[size + 1]};
	if (bp)
	{
		vsnprintf(bp.get(), size + 1, fmt, va);
		print(bp.get());
	}
	else // out of memory! print what we have:
	{
		bu[bsz - 1] = 0;
		print(bu);
	}
}

void TextVDU::printf(cstr fmt, ...) noexcept
{
	va_list va;
	va_start(va, fmt);
	printf(fmt, va);
	va_end(va);
}

str TextVDU::inputLine(std::function<int()> getc, str oldtext, int epos)
{
	// enter a new line or edit an existing line of text by the user.
	// supports multiple types of cursor control.
	// note: getc() may combine multiple sources and run a state machine.
	// todo: positioning is still a little bit buggy.
	// returns a tempstr

	using namespace USB;
	trace(__func__);

	enum {
		BACKSPACE = 8,
		RETURN	  = 13,
		ESC		  = 0x1b,
		CSI		  = 0x9b, // C1 version of ESC[
	};

	if (oldtext == nullptr) oldtext = emptystr;
	assert(epos <= int(strlen(oldtext)));

	int col0 = col; // todo: this assumes that the screen won't scroll!
	int row0 = row + scroll_count;

	print(oldtext);

	for (;;)
	{
		moveTo(row0 - scroll_count, col0 + epos, wrap);
		showCursor();
		int c = getc();

		if (c <= 0xff)
		{
			if (is_printable(char(c)))
			{
				oldtext = catstr(leftstr(oldtext, epos), charstr(char(c)), oldtext + epos);
				print(oldtext + epos++);
				continue;
			}

			// else it's a control code:
			switch (c)
			{
			case RETURN:
				print(oldtext + epos);
				newLine();
				return oldtext;
			case BACKSPACE: c = KEY_BACKSPACE; break;
			case ESC:
			case CSI:
				if (c == ESC) c = getc();
				if (c == '[') c = getc();
				switch (c)
				{
				case '3': // ESC[3~
					if (getc() != '~') continue;
					c = KEY_DELETE;
					break;
				case 'A': c = KEY_ARROW_UP; break;	  // ESC[A
				case 'B': c = KEY_ARROW_DOWN; break;  // ESC[B
				case 'C': c = KEY_ARROW_RIGHT; break; // ESC[C
				case 'D': c = KEY_ARROW_LEFT; break;  // ESC[D
				default: printf("{ESC,0x%02x}", uint(c)); continue;
				}
				break;
			default: printf("{0x%02x}", uint(c)); continue;
			}
		}

		// it's a USB key code:

		if (c == HID_KEY_OTHER + KEY_BACKSPACE + (LEFTSHIFT << 16)) //
		{
			c = KEY_DELETE;
		}

		if (c >> 16) // with modifiers
		{
			printf("{%s+%s}", tostr(HIDKey(c & 0xff)), tostr(Modifiers(c >> 16), true));
			continue;
		}

		switch (c & 0xff) // the USB keycode
		{
		case KEY_BACKSPACE:
			if (epos == 0) break;
			epos--;
			cursorLeft();
			[[fallthrough]];
		case KEY_DELETE:
			if (oldtext[epos] == 0) break;
			oldtext = catstr(leftstr(oldtext, epos), oldtext + epos + 1);
			print(oldtext + epos);
			printChar(' ');
			break;
		case KEY_ARROW_LEFT: epos = max(epos - 1, 0); break;
		case KEY_ARROW_RIGHT:
			if (oldtext[epos] != 0) printChar(oldtext[epos++]);
			break;
		case KEY_ARROW_UP: epos = max(epos - cols, 0); break;
		case KEY_ARROW_DOWN: epos = min(epos + cols, int(strlen(oldtext))); break;
		default: printf("{%s}", tostr(HIDKey(c & 0xff)));
		}
	}
}
} // namespace kio::Graphics

/*


























*/
