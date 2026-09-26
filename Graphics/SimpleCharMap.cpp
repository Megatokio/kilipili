// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "SimpleCharMap.h"
#include "common/basic_math.h"
#include <cstring>


static void* memcpy(void* __restrict z, const void* __restrict q, ssize_t n) noexcept
{
	return memcpy(z, q, size_t(n));
}
static void* memset(void* z, int c, ssize_t n) noexcept //
{
	return memset(z, c, size_t(n));
}
static void* memmove(void* z, const void* q, ssize_t n) noexcept //
{
	return memmove(z, q, size_t(n));
}


namespace kilipili::Graphics
{

SimpleCharMap::SimpleCharMap(int rows, int cols) : //
	data(nullptr),
	rows(rows),
	cols(cols)
{
	assert(rows > 0);
	assert(cols > 0);

	data = new uchar[uint(rows * cols)];
	clear();
}

void SimpleCharMap::clear(char c) noexcept
{
	memset(data, c | attr, rows * cols); //
}

void SimpleCharMap::clearRect(int row, int col, int rows, int cols, char c) noexcept
{
	// clang-format off
	if (row < 0) { rows += row; row = 0; }
	if (col < 0) { cols += col; col = 0; }
	// clang-format on
	rows = min(rows, this->rows - row);
	cols = min(cols, this->cols - col);
	if (rows <= 0 || cols <= 0) return;

	uptr p = data + row * this->cols + col;

	while (--rows >= 0)
	{
		memset(p, c | attr, cols);
		p += this->cols;
	}
}

void SimpleCharMap::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
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
		uptr  z = data + dest_row * this->cols + dest_col;
		cuptr q = data + src_row * this->cols + src_col;

		while (--rows >= 0)
		{
			memcpy(z, q, cols);
			q += this->cols;
			z += this->cols;
		}
	}
	else // same line or down in screen:
	{
		uptr  z = data + (dest_row + rows) * this->cols + dest_col;
		cuptr q = data + (src_row + rows) * this->cols + src_col;

		while (--rows >= 0)
		{
			z -= this->cols;
			q -= this->cols;
			memmove(z, q, cols);
		}
	}
}

void SimpleCharMap::putChar(int row, int col, char c) noexcept
{
	if (uint(row) >= uint(rows)) return;
	if (uint(col) >= uint(cols)) return;

	data[row * cols + col] = c | attr;
}

void SimpleCharMap::putStr(int row, int col, cstr s) noexcept
{
	if (uint(row) >= uint(rows)) return;
	limit(0, col, cols);

	uptr p = data + row * cols + col;	// print position
	int	 n = (rows - row) * cols - col; // remaining chars in screen

	while (--n >= 0 && *s) { *p++ = *s++ | attr; }
}


} // namespace kilipili::Graphics


/*






































*/
