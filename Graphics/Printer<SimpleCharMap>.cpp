// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Printer<SimpleCharMap>.h"


namespace kilipili
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

void Printer<SimpleCharMap>::write(cptr text, int count) noexcept
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


void Printer<SimpleCharMap>::setAttributes(uint add, uint remove) noexcept
{
	if (remove & bold) attributes &= ~charmap->bold;
	if (remove & inverted) attributes &= ~charmap->inverted;
	if (remove & underline) attributes &= ~charmap->underline;
	if (remove & graphics) attributes &= ~charmap->graphics;

	if (add & bold) attributes |= charmap->bold;
	if (add & inverted) attributes |= charmap->inverted;
	if (add & underline) attributes |= charmap->underline;
	if (add & graphics) attributes |= charmap->graphics;
}

uint8 Printer<SimpleCharMap>::getAttributes() const noexcept
{
	uint8 attr = 0;
	if (attributes & charmap->bold) attr = bold;
	if (attributes & charmap->inverted) attr |= inverted;
	if (attributes & charmap->underline) attr |= underline;
	if (attributes & charmap->graphics) attr |= graphics;
	return attr;
}

void Printer<SimpleCharMap>::setFont(const Font* font, bool gfx) noexcept
{
	// set a new font
	// setting a gfx font is silently ignored

	// caveat: if this SimpleCharMap is displayed,
	// then FrameBuffer<SimpleCharMap>::do_vblank() regularly reads font1:
	// => overwriting smart pointers accessed by others is not inherently safe!

	if (gfx == 0) charmap->fonts[0] = font;
}

const Font* Printer<SimpleCharMap>::getFont(bool gfx) noexcept
{
	if (gfx == 0) return charmap->fonts[0];
	else return nullptr;
}


} // namespace kilipili


/*





























*/
