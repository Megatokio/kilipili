// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Printer<CharMap>.h"
#include "cstrings.h"
#include "timing.h"

namespace kilipili
{

#define font1 fonts[0]
#define font2 fonts[1]

Printer<CharMapBase>::Printer(CharMapBase* charmap) : //
	Printer<>(charmap->rows, charmap->cols),
	charmap(charmap)
{}

void Printer<CharMapBase>::reset(bool cls) noexcept
{
	// does not reset the color tables or font

	fg_bits = charmap->fg_bits;
	bg_bits = charmap->bg_bits;
	fg_mask = charmap->fg_mask;
	bg_mask = charmap->bg_mask;

	Printer<>::reset();
	if (cls) charmap->clear();
}

void Printer<CharMapBase>::cls() noexcept
{
	// CLS, HOME CURSOR, RESET ATTR

	Printer<>::reset();
	charmap->clear();
}

void Printer<CharMapBase>::set_fgcolor(int i) noexcept
{
	int fg_ss = bg_bits;
	attributes &= ~fg_mask;
	attributes |= (i << fg_ss) & fg_mask;
}

void Printer<CharMapBase>::set_bgcolor(int i) noexcept
{
	int bg_ss = 0;
	attributes &= ~bg_mask;
	attributes |= (i << bg_ss) & bg_mask;
}

void Printer<CharMapBase>::set_bold(bool f) noexcept
{
	if (f) attributes |= charmap->bold;
	else attributes &= ~charmap->bold;
}

void Printer<CharMapBase>::set_underline(bool f) noexcept
{
	if (f) attributes |= charmap->underline;
	else attributes &= ~charmap->underline;
}

void Printer<CharMapBase>::set_graphics(bool f) noexcept
{
	if (f) attributes |= charmap->graphics;
	else attributes &= ~charmap->graphics;
}

void Printer<CharMapBase>::set_attr(int fg, int bg, bool bld, bool ul, bool gra) noexcept
{
	int fg_ss  = bg_bits;
	int bg_ss  = 0;
	attributes = ((fg << fg_ss) & fg_mask) | ((bg << bg_ss) & bg_mask);
	if (bld) attributes |= charmap->bold;
	if (ul) attributes |= charmap->underline;
	if (gra) attributes |= charmap->graphics;
}

void Printer<CharMapBase>::setAttributes(uint add, uint remove) noexcept
{
	if (remove & bold) attributes &= ~charmap->bold;
	if (remove & underline) attributes &= ~charmap->underline;
	if (remove & graphics) attributes &= ~charmap->graphics;

	if (add & bold) attributes |= charmap->bold;
	if (add & underline) attributes |= charmap->underline;
	if (add & graphics) attributes |= charmap->graphics;
}

uint8 Printer<CharMapBase>::getAttributes() const noexcept
{
	uint8 attr = 0;
	if (attributes & charmap->bold) attr = bold;
	if (attributes & charmap->underline) attr |= underline;
	if (attributes & charmap->graphics) attr |= graphics;
	return attr;
}

void Printer<CharMapBase>::setFont(const Font* font, bool gfx) noexcept
{
	// set a new font, either font1 or font2 (gfx)

	// caveat: if this CharMap is displayed,
	// then FrameBuffer<CharMap>::do_vblank() regularly reads font1 and font2:
	// => overwriting smart pointers accessed by others is not inherently safe!

	int delta_fh = font->char_height - charmap->font1->char_height;

	if (gfx)
	{
		if (delta_fh == 0) charmap->font2 = font;
		// else: silently ignore font setting
	}
	else
	{
		if (delta_fh > 0) // font > font1
		{
			charmap->font2 = font;
			// assert that do_vblank doesn't see new font1 if it didn't see new font2:
			for (int n = 0 / 5; n; --n) __asm volatile("nop");
		}
		charmap->font1 = font;
		if (delta_fh < 0) // font < font1
		{
			// assert that do_vblank doesn't see new font2 if it didn't see new font1:
			for (int n = 200 / 5; n; --n) __asm volatile("nop");
			charmap->font2 = font;
		}
	}
}

const Font* Printer<CharMapBase>::getFont(bool gfx) noexcept
{
	return charmap->fonts[gfx]; //
}


cstr attr_str(bool b, bool u, bool g)
{
	return (b + u + g) ? catstr(b ? "bold" : "-", u ? ",underline" : "-", g ? ",graphics" : "-") : "none";
}

cstr Printer<CharMapBase>::identify()
{
	//          10        20        30        40        50
	//  123456789012345678901234567890123456789012345678901234567890
	// "CharMap 50*25, attr=bold,-,-, colors=8+4"

	return usingstr(
		"CharMap %i*%i, attr=%s, colors=%i+%i", //
		cols, rows, attr_str(bold, underline, graphics), 1 << fg_bits, 1 << bg_bits);
}

inline void Printer<CharMapBase>::hide_cursor() noexcept
{
	cursor_visible		= false;
	charmap->cursor_ptr = nullptr;
}

void Printer<CharMapBase>::clearRect(int row, int col, int rows, int cols) noexcept
{
	hide_cursor();
	charmap->clearRect(row, col, rows, cols, ' ', attributes);
}

void Printer<CharMapBase>::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
{
	hide_cursor();
	charmap->copyRect(dest_row, dest_col, src_row, src_col, rows, cols);
}

void Printer<CharMapBase>::printChar(char c, int count) noexcept
{
	// any char, even char(0)

	hide_cursor();

	while (count--)
	{
		if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
		if unlikely (uint(row) >= uint(rows)) validate_vpos();
		charmap->putChar(row, col++, c, attributes);
	}
}

void Printer<CharMapBase>::putc(char c) noexcept
{
	// SerialDevice:
	// supports \n, \r and \t

	if unlikely (uchar(c) < 32)
	{
		if (c == '\n') return newLine();
		if (c == '\t') return cursorTab();
		if (c == '\r') return cursorReturn();
	}

	Printer<CharMapBase>::printChar(c, 1);
}

SIZE Printer<CharMapBase>::write(const void* data, SIZE size, __unused bool partial) noexcept
{
	// SerialDevice:
	// any char, even char(0)

	cptr text = reinterpret_cast<cptr>(data);
	for (uint i = 0; i < size; i++)
	{
		Printer<CharMapBase>::printChar(text[i], 1); // any char, even chr(0)
	}

	return size;
}

void Printer<CharMapBase>::puts(cstr s) noexcept
{
	// SerialDevice:
	// print printable text string.
	// supports \n, \r and \t
	// most used function

	hide_cursor();

	while (char c = *s++)
	{
		if unlikely (uchar(c) < 32)
		{
			putc(c);
			continue;
		}

		if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
		if unlikely (uint(row) >= uint(rows)) validate_vpos();
		charmap->putChar(row, col++, c, attributes);
	}
}

void Printer<CharMapBase>::show_cursor(bool f) noexcept
{
	//assert(f != cursor_visible);

	if (f)
	{
		assert(uint(col) < uint(cols));
		assert(uint(row) < uint(rows));

		cursor_visible		= true;
		charmap->cursor_ptr = charmap->data + row * cols + col;
	}
	else hide_cursor();
}


} // namespace kilipili


/*





























*/
