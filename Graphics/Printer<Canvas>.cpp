// Copyright (c) 2012 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Font.h"
#include "Printer<Canvas>.h"
#include "cstrings.h"
#include <string.h>

namespace kilipili::Graphics
{

// ------------------------------------------------------------
// 				Const Data:
// ------------------------------------------------------------

// lsbit is left => revert bits
#define r(b)                                                                                                   \
	((b & 128) >> 7) + ((b & 64) >> 5) + ((b & 32) >> 3) + ((b & 16) >> 1) + ((b & 8) << 1) + ((b & 4) << 3) + \
		((b & 2) << 5) + ((b & 1) << 7)

// convert nibble -> double width byte:
//
static constexpr const uint8 dblw[16] = {
	r(0x00), r(0x03), r(0x0C), r(0x0F), r(0x30), r(0x33), r(0x3C), r(0x3F),
	r(0xC0), r(0xC3), r(0xCC), r(0xCF), r(0xF0), r(0xF3), r(0xFC), r(0xFF),
};


// =============================================================================
//							F U N C T I O N S
// =============================================================================

Printer<Canvas>::Printer(Canvas* pixmap, const Font* font1, const Font* font2) noexcept :
	Printer<>(pixmap->height / font1->char_height, pixmap->width / font1->char_width),
	pixmap(pixmap),
	font1(font1),
	font2(font2),
	colormode(pixmap->colormode),
	attrheight(pixmap->attrheight),
	colordepth(get_colordepth(colormode)),	// 0 .. 4  log2 of bits per color in attributes[]
	attrmode(get_attrmode(colormode)),		// -1 .. 1 log2 of bits per color in pixmap[]
	attrwidth(get_attrwidth(colormode)),	// 0 .. 3  log2 of width of tiles
	bits_per_color(uint8(1 << colordepth)), // bits per color in pixmap[] or attributes[]
	bits_per_pixel(is_attribute_mode(colormode) ? uint8(1 << attrmode) : bits_per_color) // bpp in pixmap[]
{
	assert(font1->fixed_width);
	assert(font2->fixed_width);

	assert(font1->char_width == 8);
	assert(font2->char_width == 8);

	assert(font1->char_width == font2->char_width);
	assert(font1->char_height == font2->char_height);

	if (attrmode != attrmode_none)
	{
		assert(font1->char_height % attrheight == 0);
		assert((font1->char_width & ((1 << attrwidth) - 1)) == 0);
	}

	cursor_visible = false;
	Printer::reset();
}

void Printer<Canvas>::reset(bool cls) noexcept
{
	// all settings = default, home cursor
	// clears screen if cls = true

	fgcolor = default_fgcolor; // black / dark
	bgcolor = default_bgcolor; // white / light
	fg_ink	= 1;
	bg_ink	= 0;

	if (cls) pixmap->clear(bgcolor);
	Printer<>::reset();
}

cstr Printer<Canvas>::identify()
{
	// size=400*300, text=50*25, char=8*12, colors=i8
	// size=400*300, text=50*25, char=8*12, colors=rgb, attr=8*12

	cstr colors = colordepth == colordepth_rgb ? "rgb" : tostr(colordepth);
	cstr attr	= attrmode == attrmode_none ? "" : usingstr(", attr=%u*%u", 1u << attrwidth, attrheight);

	return usingstr(
		"size=%i*%i, text=%i*%i, char=%i*%i, colors=%s%s", //
		pixmap->width, pixmap->height, cols, rows, font1->char_width, font1->char_height, colors, attr);
}

void Printer<Canvas>::show_cursor(bool show) noexcept
{
	// for all pixels: color ^= fgcolor ^ bgcolor

	if (show)
	{
		cursorXorColor = fgcolor ^ bgcolor; // TODO: ink vs. color everywhere!
		if (cursorXorColor == 0) cursorXorColor = ~0u;
	}

	int CHAR_HEIGHT = font1->char_height;
	pixmap->xorRect(col * CHAR_WIDTH, row * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT, cursorXorColor);
	cursor_visible = show;
}

void Printer<Canvas>::clearRect(int row, int col, int rows, int cols) noexcept
{
	// erase a rectangular area on the screen

	hideCursor();

	if (rows > 0 && cols > 0)
	{
		int CHAR_HEIGHT = font1->char_height;
		int x			= col * CHAR_WIDTH;
		int y			= row * CHAR_HEIGHT;
		pixmap->fillRect(Rect(x, y, cols * CHAR_WIDTH, rows * CHAR_HEIGHT), bgcolor, bg_ink);
	}
}

void Printer<Canvas>::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
{
	hideCursor();

	if (rows > 0 && cols > 0)
	{
		int char_height = font1->char_height;
		pixmap->copyRect(
			dest_col * CHAR_WIDTH, dest_row * char_height, src_col * CHAR_WIDTH, src_row * char_height,
			cols * CHAR_WIDTH, rows * char_height);
	}
}

void Printer<Canvas>::setAttributes(uint add, uint remove) noexcept
{
	attributes = Attributes((attributes & ~remove) | add);
	dx		   = attributes & double_width ? 2 : 1;
	dy		   = attributes & double_height ? 2 : 1;
}

void Printer<Canvas>::applyAttributes(CharMatrix bmp) noexcept
{
	// apply the simple attributes to a character matrix
	// - bold
	// - underline
	// - italic
	// - inverted

	uint8 a			  = attributes;
	int	  char_height = font1->char_height;

	if (int8(a) > 0) // any attr except graphics_char_mode set?
	{
		if (a & bold)
		{
			for (int i = 0; i < char_height; i++) bmp[i] |= bmp[i] >> 1;
		}
		if (a & underline)
		{
			int row	 = (char_height + font1->baseline) / 2;
			bmp[row] = 0xff;
		}
		if (a & italic)
		{
			int i = 0;
			while (3 * i < char_height) bmp[i++] >>= 1;
			while (3 * i < 2 * char_height) i++;
			while (i < char_height) bmp[i] <<= 1;
		}
		if (a & inverted)
		{
			for (int i = 0; i < char_height; i++) { bmp[i] ^= 0xff; }
		}
	}
}

void Printer<Canvas>::readBmp(CharMatrix bmp, bool use_fgcolor) noexcept
{
	// read BMP of character cell from screen.
	// read character cell at cursor position.
	// increment col (as for printing, except no double width/height attribute)
	// use_fgcolor=1: set bits for pixels in fgcolor
	// use_fgcolor=0: clr bits for pixels in bgcolor

	hideCursor();
	validate_hpos(false);
	assert(row >= 0 && row < rows);

	int char_height = font1->char_height;
	int x			= col++ * CHAR_WIDTH;
	int y			= row * char_height;
	pixmap->readBmp(x, y, bmp, 1 /*row_offset*/, CHAR_WIDTH, char_height, use_fgcolor ? fgcolor : bgcolor, use_fgcolor);
}

void Printer<Canvas>::writeBmp(CharMatrix bmp, uint8 attr) noexcept
{
	// write BMP to screen applying the 'late' attributes:
	//	+ double width
	//	+ double height
	//	+ overprint
	//  - bold, italic, underline, inverted and graphics must already be applied
	// at cursor position
	// increment col

	int char_height = font1->char_height;

	hideCursor();
	if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
	if unlikely (uint(row) >= uint(rows)) validate_vpos();

	if unlikely (attr & double_width)
	{
		CharMatrix bmp2;
		assert(uint(char_height) <= sizeof(bmp2));

		// if in last column, don't print 2 half characters:
		if (col == cols - 1)
		{
			memset(bmp2, 0, uint(char_height));
			uint8 attr2 = attr & ~double_width;

			// if in top-right corner don't scroll screen down:
			if (row == 0) attr2 &= ~double_height;

			// clear to eol and incr col:
			writeBmp(bmp2, attr2);
			validate_hpos(false);
			assert(col == 0);
		}

		for (int i = 0; i < char_height; i++) { bmp2[i] = dblw[bmp[i] >> 4]; }
		writeBmp(bmp2, attr &= ~double_width);

		for (int i = 0; i < char_height; i++) { bmp[i] = dblw[bmp[i] & 15]; }
	}

	if unlikely (attr & double_height)
	{
		CharMatrix bmp2;
		assert(uint(char_height) <= sizeof(bmp2));

		for (int i = 0; i < char_height; i++) { bmp2[i] = bmp[i / 2]; }
		row--;
		validate_vpos();
		writeBmp(bmp2, attr & ~double_height);
		col--;
		row++;

		for (int i = 0; i < char_height; i++) { bmp[i] = bmp[char_height / 2 + i / 2]; }
	}

	assert(col >= 0 && col < cols);
	assert_ge(row, 0);
	assert_lt(row, rows);

	int x = col++ * CHAR_WIDTH;
	int y = row * char_height;

	if (!(attr & transparent)) pixmap->fillRect(x, y, CHAR_WIDTH, char_height, bgcolor, bg_ink);
	static_assert(CHAR_WIDTH == 8);
	pixmap->drawChar(x, y, bmp, char_height, fgcolor, fg_ink);
}

void Printer<Canvas>::getCharMatrix(CharMatrix charmatrix, char cc) noexcept
{
	// get character matrix of character c
	// returns ASCII, UDG or LATIN-1 characters
	// or GRAPHICS CHARACTERS if attribute ATTR_GRAPHICS_CHARACTERS is set

	const Font*	 font = attributes & graphics ? font2 : font1;
	const uchar* p	  = font->data + uchar(cc) - font->first_glyph;
	for (int i = 0; i < font1->char_height; i++)
	{
		charmatrix[i] = *p;
		p += font->row_offset;
	}
}

void Printer<Canvas>::printCharMatrix(CharMatrix charmatrix, int count) noexcept
{
	applyAttributes(charmatrix);
	while (count--) { writeBmp(charmatrix, attributes); }
}

void Printer<Canvas>::printChar(char c, int count) noexcept
{
	CharMatrix charmatrix;
	assert(uint(font1->char_height) <= sizeof(charmatrix));

	getCharMatrix(charmatrix, c);
	printCharMatrix(charmatrix, count);
}

void Printer<Canvas>::print(cstr s) noexcept
{
	// print printable text string.
	// control characters: only \t and \n.

	CharMatrix charmatrix;
	assert(uint(font1->char_height) <= sizeof(charmatrix));

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

		getCharMatrix(charmatrix, c);
		printCharMatrix(charmatrix, 1);
	}
}


} // namespace kilipili::Graphics

/*


























*/
