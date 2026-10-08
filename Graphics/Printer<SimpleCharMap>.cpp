// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Printer<SimpleCharMap>.h"


namespace kilipili::Graphics
{

Printer<SimpleCharMap>::Printer(SimpleCharMap* cm) : //
	Printer<>(cm->rows, cm->cols),
	charmap(cm)
{}


void Printer<SimpleCharMap>::reset(bool cls) noexcept
{
	// does not reset the colors or font

	super::reset();
	if (cls) charmap->clear();
}

cstr Printer<SimpleCharMap>::identify()
{
	return "simple charmap"; //
}

inline void Printer<SimpleCharMap>::hide_cursor() noexcept
{
	cursor_visible		= false;
	charmap->cursor_ptr = nullptr;
}

void Printer<SimpleCharMap>::clearRect(int row, int col, int rows, int cols) noexcept
{
	hide_cursor();
	charmap->clearRect(row, col, rows, cols, ' ', attributes);
}

void Printer<SimpleCharMap>::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
{
	hide_cursor();
	charmap->copyRect(dest_row, dest_col, src_row, src_col, rows, cols);
}

void Printer<SimpleCharMap>::printChar(char c, int count) noexcept
{
	hide_cursor();

	while (count--)
	{
		if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
		if unlikely (uint(row) >= uint(rows)) validate_vpos();
		charmap->putChar(row, col, c, attributes);
	}
}

void Printer<SimpleCharMap>::print(cstr s) noexcept
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

void Printer<SimpleCharMap>::show_cursor(bool f) noexcept
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
