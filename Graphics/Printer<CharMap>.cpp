// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Printer<CharMap>.h"
#include "cstrings.h"


namespace kilipili::Graphics
{

Printer<CharMap<>>::Printer(CharMap<>* charmap) : //
	Printer<>(charmap->rows, charmap->cols),
	charmap(charmap)
{}


void Printer<CharMap<>>::reset(bool cls) noexcept
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

cstr attr_str(bool b, bool u, bool g)
{
	return (b + u + g) ? catstr(b ? "bold" : "-", u ? ",underline" : "-", g ? ",graphics" : "-") : "none";
}

cstr Printer<CharMap<>>::identify()
{
	//          10        20        30        40        50
	//  123456789012345678901234567890123456789012345678901234567890
	// "CharMap 50*25, attr=bold,-,-, colors=8+4"

	return usingstr(
		"CharMap %i*%i, attr=%s, colors=%i+%i", //
		cols, rows, attr_str(bold, underline, graphics), 1 << fg_bits, 1 << bg_bits);
}

inline void Printer<CharMap<>>::hide_cursor() noexcept
{
	cursor_visible		= false;
	charmap->cursor_ptr = nullptr;
}

void Printer<CharMap<>>::clearRect(int row, int col, int rows, int cols) noexcept
{
	hide_cursor();
	charmap->clearRect(row, col, rows, cols, ' ', attributes);
}

void Printer<CharMap<>>::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
{
	hide_cursor();
	charmap->copyRect(dest_row, dest_col, src_row, src_col, rows, cols);
}

void Printer<CharMap<>>::printChar(char c, int count) noexcept
{
	hide_cursor();

	while (count--)
	{
		if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
		if unlikely (uint(row) >= uint(rows)) validate_vpos();
		charmap->putChar(row, col, c, attributes);
	}
}

void Printer<CharMap<>>::print(cstr s) noexcept
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
		charmap->putChar(row, col, c, attributes);
	}
}

void Printer<CharMap<>>::show_cursor(bool f) noexcept
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


} // namespace kilipili::Graphics


/*





























*/
