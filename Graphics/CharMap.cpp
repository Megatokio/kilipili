// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "CharMap.h"
#include "common/basic_math.h"
#include <cstring>


static inline __unused void* memcpy(void* __restrict z, const void* __restrict q, int n) noexcept
{
	return ::memcpy(z, q, size_t(n));
}
static inline __unused void* memset(void* z, int c, int n) noexcept //
{
	return ::memset(z, c, size_t(n));
}
static inline __unused void* memmove(void* z, const void* q, int n) noexcept //
{
	return ::memmove(z, q, size_t(n));
}


namespace kilipili::Graphics
{

void CharMap<>::clearRect(int row, int col, int rows, int cols, char c) noexcept
{
	// clang-format off
	if (row < 0) { rows += row; row = 0; }
	if (col < 0) { cols += col; col = 0; }
	// clang-format on
	rows = min(rows, this->rows - row);
	cols = min(cols, this->cols - col);
	if (rows <= 0 || cols <= 0) return;

	uint16* p = data + row * this->cols + col;

	while (--rows >= 0)
	{
		for (int i = 0; i < cols; i++) p[i] = char_with_attr(c);
		p += this->cols;
	}
}

void CharMap<>::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
{
	// clang-format off
	if (dest_row < 0) { rows += dest_row; src_row -= dest_row; dest_row = 0; }
	if (dest_col < 0) { cols += dest_col; src_col -= dest_col; dest_col = 0; }
	if (src_row < 0)  { rows += src_row;  dest_row -= src_row; src_row = 0; }
	if (src_col < 0)  { cols += src_col;  dest_col -= src_col; src_col = 0; }
	// clang-format on
	rows = min(rows, this->rows - dest_row, this->rows - src_row);
	cols = min(cols, this->cols - dest_col, this->cols - src_col);
	if (rows <= 0 || cols <= 0) return;

	if (dest_row < src_row)
	{
		uint16* z = data + dest_row * this->cols + dest_col;
		uint16* q = data + src_row * this->cols + src_col;

		while (--rows >= 0)
		{
			memcpy(z, q, cols * 2);
			q += this->cols;
			z += this->cols;
		}
	}
	else // same line or down in screen:
	{
		uint16* z = data + (dest_row + rows) * this->cols + dest_col;
		uint16* q = data + (src_row + rows) * this->cols + src_col;

		while (--rows >= 0)
		{
			z -= this->cols;
			q -= this->cols;
			memmove(z, q, cols * 2);
		}
	}
}

void CharMap<>::putChar(int row, int col, char c) noexcept
{
	if (uint(row) >= uint(rows)) return;
	if (uint(col) >= uint(cols)) return;

	data[row * cols + col] = char_with_attr(c);
}

void CharMap<>::putStr(int row, int col, cstr s) noexcept
{
	if (uint(row) >= uint(rows)) return;
	limit(0, col, cols);

	uint16* p = data + row * cols + col;   // print position
	int		n = (rows - row) * cols - col; // remaining chars in screen

	while (--n >= 0 && *s) { *p++ = char_with_attr(*s++); }
}


} // namespace kilipili::Graphics


/*






































*/
