// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Printer<SimpleCharMap>.h"


namespace kilipili
{

Printer<SimpleCharMap>::Printer(SimpleCharMap* cm) : //
	Printer<>(cm->rows, cm->cols, 0, 0, w_sep_cmap),
	charmap(cm)
{}

void Printer<SimpleCharMap>::reset(bool cls) noexcept
{
	// does not reset the colors or font

	Printer<>::reset();
	if (cls) charmap->clear();
}

void Printer<SimpleCharMap>::cls() noexcept
{
	// CLS, HOME CURSOR, RESET ATTR

	Printer<>::reset();
	charmap->clear();
}

cstr Printer<SimpleCharMap>::identification()
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
	// any char, even char(0)

	hide_cursor();

	while (count--)
	{
		if unlikely (uint(col) >= uint(cols)) validate_hpos(false);
		if unlikely (uint(row) >= uint(rows)) validate_vpos();
		charmap->putChar(row, col++, c, attributes);
	}
}

void Printer<SimpleCharMap>::putc(char c) noexcept
{
	// SerialDevice:
	// supports \n, \r and \t

	if unlikely (uchar(c) < 32)
	{
		if (c == '\n') return newLine();
		if (c == '\t') return cursorTab();
		if (c == '\r') return cursorReturn();
	}

	Printer<SimpleCharMap>::printChar(c, 1);
}

SIZE Printer<SimpleCharMap>::write(const void* data, SIZE size, __unused bool partial) noexcept
{
	// SerialDevice:
	// any char, even char(0)

	cptr text = reinterpret_cast<cptr>(data);
	for (uint i = 0; i < size; i++)
	{
		Printer<SimpleCharMap>::printChar(text[i], 1); // any char, even chr(0)
	}

	return size;
}

void Printer<SimpleCharMap>::puts(cstr s) noexcept
{
	// SerialDevice:
	// print printable text string.
	// supports \n, \r and \t

	while (char c = *s++)
	{
		Printer<SimpleCharMap>::putc(c); // supports \n, \r and \t
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
