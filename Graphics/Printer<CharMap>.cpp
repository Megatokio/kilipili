// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Printer<CharMap>.h"
#include "cstrings.h"


namespace kilipili
{

Printer<CharMapBase>::Printer(CharMapBase* charmap) : //
	Printer<>(charmap->rows, charmap->cols),
	charmap(charmap)
{}


void Printer<CharMapBase>::reset(bool cls) noexcept
{
	// does not reset the colors or font

	fg_bits	  = charmap->fg_bits;
	bg_bits	  = charmap->bg_bits;
	fg_mask	  = charmap->fg_mask;
	bg_mask	  = charmap->bg_mask;
	bold	  = charmap->bold;
	underline = charmap->underline;
	graphics  = charmap->graphics;

	Printer<>::reset();
	if (cls) charmap->clear();
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
	if (f) attributes |= bold;
	else attributes &= ~bold;
}

void Printer<CharMapBase>::set_underline(bool f) noexcept
{
	if (f) attributes |= underline;
	else attributes &= ~underline;
}

void Printer<CharMapBase>::set_graphics(bool f) noexcept
{
	if (f) attributes |= graphics;
	else attributes &= ~graphics;
}

void Printer<CharMapBase>::set_attr(int fg, int bg, bool bld, bool ul, bool gra) noexcept
{
	int fg_ss  = bg_bits;
	int bg_ss  = 0;
	attributes = ((fg << fg_ss) & fg_mask) | ((bg << bg_ss) & bg_mask);
	if (bld) attributes |= bold;
	if (ul) attributes |= underline;
	if (gra) attributes |= graphics;
}

void Printer<CharMapBase>::setAttributes(uint add, uint remove) noexcept
{
	remove &= bold | underline | graphics;
	add &= bold | underline | graphics;
	attributes &= ~remove;
	attributes |= add;
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
	hide_cursor();

	while (count--)
	{
		if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
		if unlikely (uint(row) >= uint(rows)) validate_vpos();
		charmap->putChar(row, col, c, attributes);
	}
}

void Printer<CharMapBase>::write(cptr text, int count) noexcept
{
	// any char, even chr(0)

	hide_cursor();

	for (int i = 0; i < count; i++)
	{
		if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
		if unlikely (uint(row) >= uint(rows)) validate_vpos();
		charmap->putChar(row, col++, text[i], attributes);
	}
}

void Printer<CharMapBase>::print(cstr s) noexcept
{
	hide_cursor();

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
