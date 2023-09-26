// Copyright (c) 2012 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "PicoTerm.h"
#include "ColorMap.h"


namespace kio::Graphics
{


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


PicoTerm::PicoTerm(Canvas& pixmap, Color* colors) :
	pixmap(pixmap),
	colormap(colors),
	colormode(pixmap.colormode),
	attrheight(pixmap.attrheight),
	colordepth(get_colordepth(colormode)), // 0 .. 4  log2 of bits per color in attributes[]
	attrmode(get_attrmode(colormode)),	   // 0 .. 2  log2 of bits per color in pixmap[]
	attrwidth(get_attrwidth(colormode)),   // 0 .. 3  log2 of width of tiles
	bits_per_color(1 << colordepth),	   // bits per color in pixmap[] or attributes[]
	bits_per_pixel(is_attribute_mode(colormode) ? 1 << attrmode : bits_per_color) // bpp in pixmap[]
{
	reset();
}


void PicoTerm::showCursor()
{
	if (cursorVisible) return;

	validateCursorPosition();
	show_cursor(true);
}


inline void PicoTerm::hideCursor()
{
	if (unlikely(cursorVisible)) show_cursor(false);
}


void PicoTerm::scrollScreen(coord dx /*chars*/, coord dy /*chars*/)
{
	hideCursor();

	int w = (screen_width - abs(dx)) * CHAR_WIDTH;
	int h = (screen_height - abs(dy)) * CHAR_HEIGHT;

	if (w <= 0 || h <= 0) return pixmap.clear(bgcolor, bg_ink);

	dx *= CHAR_WIDTH;
	dy *= CHAR_HEIGHT;

	coord qx = dx >= 0 ? 0 : -dx;
	coord zx = dx >= 0 ? +dx : 0;
	coord qy = dy >= 0 ? 0 : -dy;
	coord zy = dy >= 0 ? +dy : 0;

	pixmap.copyRect(zx, zy, qx, qy, w, h);

	if (dx > 0) pixmap.fillRect(0, 0, +dx, screen_height * CHAR_HEIGHT, bgcolor, bg_ink);
	if (dx < 0) pixmap.fillRect(w, 0, -dx, screen_height * CHAR_HEIGHT, bgcolor, bg_ink);

	if (dy > 0) pixmap.fillRect(0, 0, screen_width * CHAR_WIDTH, +dy, bgcolor, bg_ink);
	if (dy < 0) pixmap.fillRect(0, h, screen_width * CHAR_WIDTH, -dy, bgcolor, bg_ink);
}


void PicoTerm::scrollScreenUp(int rows /*char*/)
{
	if (rows > 0) scrollScreen(0, -rows);
}


void PicoTerm::scrollScreenDown(int rows /*char*/)
{
	if (rows > 0) scrollScreen(0, +rows);
}


void PicoTerm::scrollScreenLeft(int cols)
{
	if (cols > 0) scrollScreen(-cols, 0);
}


void PicoTerm::scrollScreenRight(int cols)
{
	if (cols > 0) scrollScreen(+cols, 0);
}


void PicoTerm::validateCursorPosition()
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


void PicoTerm::readBmp(CharMatrix bmp, bool use_fgcolor)
{
	// read BMP of character cell from screen.
	// read character cell at cursor position.
	// increment col (as for printing, except no double width/height attribute)
	// use_fgcolor=1: set bits for pixels in fgcolor
	// use_fgcolor=0: clr bits for pixels in bgcolor

	validateCursorPosition();

	int x = col++ * CHAR_WIDTH;
	int y = row * CHAR_HEIGHT;
	pixmap.readBmp(x, y, bmp, 1 /*row_offset*/, CHAR_WIDTH, CHAR_HEIGHT, use_fgcolor ? fgcolor : bgcolor, use_fgcolor);
}


void PicoTerm::writeBmp(CharMatrix bmp, uint8 attr)
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

	if (!(attr & ATTR_OVERPRINT)) pixmap.fillRect(x, y, CHAR_WIDTH, CHAR_HEIGHT, bgcolor, bg_ink);
	//pixmap.drawBmp(x, y, bmp, 1 /*row_offset*/, CHAR_WIDTH, CHAR_HEIGHT, fgcolor, fg_ink);
	static_assert(CHAR_WIDTH == 8);
	pixmap.drawChar(x, y, bmp, CHAR_HEIGHT, fgcolor, fg_ink);
}


void PicoTerm::getCharMatrix(CharMatrix charmatrix, char cc)
{
	// get character matrix of character c
	// returns ASCII, UDG or LATIN-1 characters
	// or GRAPHICS CHARACTERS if attribute ATTR_GRAPHICS_CHARACTERS is set

	// valid range:
	// 0x20 .. 0x7F	ASCII
	// 0x80 .. 0x9F	UDG
	// 0xA0 .. 0xFF	LATIN-1

	uchar c = uchar(cc);

	if (attributes & ATTR_GRAPHICS_CHARACTERS) { getGraphicsCharMatrix(charmatrix, c); }
	else
	{
		uint o = 127 - 32; // default = pattern of 'delete'

		if (c < 0x20) {}
		else if (c < 0x80) { o = c - 32; }
		else if (c < 0xA0) {} // readUDG(c,charmatrix);		TODO:  LCD: read from eeprom
		else { o = c - 64; }

		const uchar* p = font + o * CHAR_HEIGHT;
		memcpy(charmatrix, p, CHAR_HEIGHT);
	}
}


void PicoTerm::getGraphicsCharMatrix(CharMatrix charmatrix, char cc)
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


void PicoTerm::applyAttributes(CharMatrix bmp)
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


void PicoTerm::eraseRect(int row, int col, int rows, int cols)
{
	// erase a rectangular area on the screen

	hideCursor();

	if (rows > 0 && cols > 0)
	{
		int x = col * CHAR_WIDTH;
		int y = row * CHAR_HEIGHT;
		pixmap.fillRect(Rect(x, y, cols * CHAR_WIDTH, rows * CHAR_HEIGHT), bgcolor, bg_ink);
	}
}


void PicoTerm::copyRect(int src_row, int src_col, int dest_row, int dest_col, int rows, int cols)
{
	hideCursor();

	if (rows > 0 && cols > 0)
	{
		pixmap.copyRect(
			src_col * CHAR_WIDTH, src_row * CHAR_HEIGHT, dest_col * CHAR_WIDTH, dest_row * CHAR_HEIGHT,
			cols * CHAR_WIDTH, rows * CHAR_HEIGHT);
	}
}


void PicoTerm::reset()
{
	// ALL SETTINGS := DEFAULT, CLS, HOME CURSOR

	screen_width  = pixmap.width / CHAR_WIDTH;
	screen_height = pixmap.height / CHAR_HEIGHT;

	bg_ink	= 0;
	fg_ink	= 1;
	bgcolor = default_bgcolor; // white / light
	fgcolor = default_fgcolor; // black / dark

	pushedRow  = 0;
	pushedCol  = 0;
	pushedAttr = 0;

	resetColorMap(colordepth, colormap);
	cls();
}


void PicoTerm::cls()
{
	// CLS, HOME CURSOR, RESET ATTR

	row = col = 0;
	dx = dy		  = 1;
	attributes	  = 0;
	cursorVisible = false;

	pixmap.clear(bgcolor, bg_ink);
}


void PicoTerm::moveToPosition(int row, int col) noexcept
{
	hideCursor();
	this->row = row;
	this->col = col;
}


void PicoTerm::moveToCol(int col) noexcept
{
	hideCursor();
	this->col = col;
}


void PicoTerm::pushCursorPosition()
{
	//TODO: visibility, window
	pushedRow  = row;
	pushedCol  = col;
	pushedAttr = attributes;
}


void PicoTerm::popCursorPosition()
{
	row = pushedRow;
	col = pushedCol;
	setPrintAttributes(pushedAttr);
}


void PicoTerm::setPrintAttributes(uint8 attr)
{
	attributes = attr;
	dx		   = attr & ATTR_DOUBLE_WIDTH ? 2 : 1;
	dy		   = attr & ATTR_DOUBLE_HEIGHT ? 2 : 1;
}


void PicoTerm::cursorLeft(int count) // BS
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


void PicoTerm::cursorRight(int count) // FF
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


void PicoTerm::cursorUp(int count)
{
	// scrolls

	hideCursor();
	if (count > 0) row -= dy * count;
}


void PicoTerm::cursorDown(int count) // NL
{
	// scrolls

	hideCursor();
	if (count > 0) row += dy * count;
}


void PicoTerm::cursorTab(int count)
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


void PicoTerm::cursorReturn()
{
	// COL := 0

	hideCursor();
	col = 0;
}


void PicoTerm::clearToEndOfLine()
{
	hideCursor();
	if (col < screen_width && row < screen_height) eraseRect(row, col, 1 /*rows*/, screen_width - col /*cols*/);
}


void PicoTerm::printCharMatrix(CharMatrix charmatrix, int count)
{
	applyAttributes(charmatrix);

	while (count--) { writeBmp(charmatrix, attributes); }
}


void PicoTerm::printChar(char c, int count)
{
	CharMatrix charmatrix;
	getCharMatrix(charmatrix, c);
	printCharMatrix(charmatrix, count);
}


void PicoTerm::printText(cstr s)
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


void PicoTerm::print(cstr s, bool auto_crlf)
{
	assert(s);

	auto getc = [&]() -> uchar { return *s ? *s++ : 0; };

loop:
	int repeat_count = 1;

loop_repeat:
	uchar c = uchar(*s++);

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


void PicoTerm::printf(cstr fmt, ...)
{
	constexpr int max_cnt = 127;
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


char* PicoTerm::identify(char* buffer)
{
	// PicoTerm gfx=400*300 txt=50*25 chr=8*12 cm=rgb
	// PicoTerm gfx=400*300 txt=50*25 chr=8*12 cm=i8 attr=8*12

	sprintf(
		buffer, "PicoTerm gfx=%u*%u txt=%u*%u chr=%u*%u cm=%s", pixmap.width, pixmap.height, screen_width,
		screen_height, CHAR_WIDTH, CHAR_HEIGHT, tostr(colordepth));

	if (attrmode != attrmode_none) sprintf(strchr(buffer, 0), " attr=%u*%u", 1 << attrwidth, attrheight);

	return buffer;
}


void PicoTerm::show_cursor(bool show)
{
	// for all pixels: color ^= fgcolor ^ bgcolor

	if (show)
	{
		cursorXorColor = fgcolor ^ bgcolor; // TODO: ink vs. color everywhere!
		if (cursorXorColor == 0) cursorXorColor = ~0u;
	}

	pixmap.xorRect(col * CHAR_WIDTH, row * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT, cursorXorColor);
	cursorVisible = show;
}


} // namespace kio::Graphics

/*





































*/
