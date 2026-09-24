// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "TaggedTextVideoFrame.h"
#include "Graphics/systemfont.h"
#include "basic_math.h"
#include <cstring>


static inline void* memcpy(void* __restrict z, const void* __restrict q, ssize_t n) noexcept
{
	return ::memcpy(z, q, size_t(n));
}
static inline void* memset(void* z, int c, ssize_t n) noexcept //
{
	return ::memset(z, c, size_t(n));
}
static inline void* memmove(void* z, const void* q, ssize_t n) noexcept //
{
	return ::memmove(z, q, size_t(n));
}


namespace kilipili::Video
{

// Tagged character video frame buffer
//
// Each line starts with the initial attribute and ends with 0x00.
// The screen ends with 0x00 (where the attr for the next row is expected).
// As no char(0) is allowed except for the trailing 0x00 each line is a cstring.
//
// In an expanded screen all lines are padded with spaces to the nominal size of cols+2 bytes.
// The size of expanded screen is: rows * (cols + 2) + 1.
// The expanded screen is intended for editing and use as a frame buffer.
//
// In a collapsed screen all lines are truncated at their trailing spaces.
// The collapsed screen may be const and in flash.
// The collapsed screen may be used for dialog boxes and menus.


TaggedTextVideoFrame::TaggedTextVideoFrame(int rows, int cols) : //
	rows(uint16(rows)),
	cols(uint16(cols))
{
	assert(rows > 0 && cols > 0);

	charmap = new char[uint(rows * (cols + 2) + 1)];
	clear();
}

TaggedTextVideoFrame::TaggedTextVideoFrame(int rows, int cols, const char* collapsed_text, bool make_copy) :
	rows(uint16(rows)),
	cols(uint16(cols)),
	charmap(const_cast<char*>(collapsed_text))
{
	is_const	 = true; // and not owned by us
	is_collapsed = true;

	if (make_copy)
	{
		cptr p = collapsed_text;
		while (*p++)		// until screen end marker found
			while (*p++) {} // until line end marker found
		size_t total_size = size_t(p - collapsed_text);
		charmap			  = new char[total_size];
		is_const		  = false; // owned by us
		memcpy(charmap, collapsed_text, total_size);
	}
}

TaggedTextVideoFrame::~TaggedTextVideoFrame()
{
	if (!is_const) delete[] charmap; //
}

static void clear_expanded_screen(ptr p, int rows, int cols)
{
	// clear screen to default_attr and all spaces
	// each row starts with default_attr,
	// then col spaces
	// then 0x00 as a cstring delimiter.
	// a final 0x00 terminates the screen.

	memset(p, ' ', rows * (cols + 2));
	for (int row = 0; row < rows; row++)
	{
		*p++ = TaggedTextVideoFrame::default_attr;
		p += cols;
		*p++ = 0x00; // line end
	}
	*p++ = 0x00; // screen end
}

static int length_of_collapsed_line(cptr expanded_line, int cols) noexcept
{
	assert(expanded_line[cols + 1] == 0x00);
	assert(expanded_line[0] != 0x00);

	cptr p = expanded_line;
	cptr e = p + cols; // e -> last char
	while (e > p && *e == ' ') e--;
	return e - p + 1;
}

static int size_of_collapsed_screen(cptr expanded_screen, int rows, int cols)
{
	cptr p	  = expanded_screen;
	int	 size = rows + 1; // line end and screen end char 0x00
	for (int row = 0; row < rows; row++)
	{
		size += length_of_collapsed_line(p, cols);
		p += cols + 2;
	}
	return size;
}

void TaggedTextVideoFrame::clear() noexcept
{
	// clear screen to default_attr and all spaces

	assert(is_collapsed == false);

	clear_expanded_screen(charmap, rows, cols);
}

void TaggedTextVideoFrame::collapse(bool f)
{
	// collapse or expand existing charmap

	if (f == is_collapsed) return;

	ptr newmap = nullptr;

	if (f) // collapse
	{
		int zsize = size_of_collapsed_screen(charmap, rows, cols);
		newmap	  = new char[uint(zsize)];

		cptr q = charmap;
		ptr	 z = newmap;

		for (int row = 0; row < rows; row++)
		{
			int len = length_of_collapsed_line(q, cols);
			memcpy(z, q, len);
			*z = 0;
			z += len + 2;
			q += cols + 2;
		}
		*z = 0x00;
	}
	else // expand
	{
		newmap = new char[rows * (cols + 2) + 1];
		clear_expanded_screen(newmap, rows, cols);

		cptr q = charmap;
		ptr	 z = newmap;

		for (int row = 0; row < rows && *q != 0x00;)
		{
			ptr qend = strchr(q, 0);
			int cnt	 = min(cols + 1, qend - q); // incl. attr, ohne 0x00
			memcpy(z, q, cnt);
			q = qend + 1;
			z += cols + 2;
		}
	}

	// TODO: handle race condition / swap in vblank / assert not visible
	if (!is_const) delete[] charmap;
	charmap		 = newmap;
	is_collapsed = f;
	is_const	 = false;
}

void TaggedTextVideoFrame::clearLine(int row) noexcept
{
	assert(is_collapsed == false);
	assert(is_const == false);

	if (uint(row) < uint(rows))
	{
		ptr p = charmap + (row) * (cols + 2);
		*p	  = default_attr;
		memset(p + 1, ' ', cols);
	}
}

void TaggedTextVideoFrame::scrollScreen(int dy) noexcept
{
	assert(is_collapsed == false);
	assert(is_const == false);

	limit(-rows, dy, +rows);

	for (; dy > 0; dy--) // scroll up (normal case)
	{
		memmove(charmap, charmap + cols + 2, (rows - 1) * (cols + 2));
		clearLine(rows - 1);
	}
	for (; dy < 0; dy++) // scroll down (reverse scroll)
	{
		memmove(charmap + cols + 2, charmap, (rows - 1) * (cols + 2));
		clearLine(0);
	}
}


void TaggedTextVideoFrame::clearRect(int row_a, int col_a, int n_rows, int n_cols) noexcept
{
	assert(is_collapsed == false);
	assert(is_const == false);

	// TODO: if col_a = -1 then also clear initial attr to default_attr

	int row_e = row_a + n_rows;
	int col_e = col_a + n_cols;

	row_a = max(row_a, 0);
	col_a = max(col_a, 0);
	row_e = min(row_e, rows);
	col_e = min(col_e, cols);

	if (row_e <= row_a || col_e <= col_a) return;

	ptr p = charmap + 1 + row_a * (cols + 2) + col_a;
	for (int row = row_a; row < row_e; row++)
	{
		memset(p, ' ', col_e - col_a);
		p += cols + 2;
	}
}


void TaggedTextVideoFrame::copyRect(int zy, int zx, int qy, int qx, int h, int w) noexcept
{
	assert(is_collapsed == false);
	assert(is_const == false);

	// TODO text whether qx=-1 and zx=-1 => also copy initial attr

	if (qx < 0)
	{
		w += qx;
		zx -= qx;
		qx -= qx;
	}
	if (qy < 0)
	{
		h += qy;
		zy -= qy;
		qy -= qy;
	}
	if (zx < 0)
	{
		w += zx;
		qx -= zx;
		zx -= zx;
	}
	if (zy < 0)
	{
		h += zy;
		qy -= zy;
		zy -= zy;
	}
	w = min(w, cols - zx, cols - qx);
	h = min(h, rows - zy, rows - qy);

	if (w <= 0 || h <= 0) return;

	ptr	 z = charmap + 1 + zx + zy * (cols + 2);
	cptr q = charmap + 1 + qx + qy * (cols + 2);

	if (zy < qy)
	{
		for (int i = 0; i < h; i++)
		{
			memcpy(z, q, w);
			q += cols + 2;
			z += cols + 2;
		}
	}
	else if (zy > qy)
	{
		z += h * (cols + 2);
		q += h * (cols + 2);
		for (int i = 0; i < h; i++)
		{
			q -= cols + 2;
			z -= cols + 2;
			memcpy(z, q, w);
		}
	}
	else
	{
		for (int i = 0; i < h; i++)
		{
			memmove(z, q, w);
			q += cols + 2;
			z += cols + 2;
		}
	}
}

void TaggedTextVideoFrame::vblank() noexcept {}

void TaggedTextVideoFrame::renderScanline(int row, int width, uint32* buffer) noexcept
{
	using namespace Graphics;

	constexpr int char_width  = 8;
	constexpr int char_height = 12;

	assert(row < rows * char_height);
	assert(width < cols * char_width);
	assert(width % char_height == 0);

	const uchar* font = systemfont256x12 + row % char_height;
	cptr		 q	  = charmap + row / char_height;
	Color*		 z	  = reinterpret_cast<Color*>(buffer);

	uchar attr	  = 0x02;
	Color bgcolor = bgcolors[0];
	Color fgcolor = fgcolors[1];
	uchar bold	  = false;

	for (int col = 0; col < width / char_width; col++)
	{
		uchar c = *q++;
		if (c > 0x20)
		{
		render_char:
			int pixels = font[c * char_height];
			if (bold) pixels += pixels << 1;
			for (int i = 0; i < 8; i++)
			{
				*z++ = pixels & (0x01 << i) ? fgcolor : bgcolor; //
			}
		}
		else if (c == attr) // no attr change: print space
		{
		render_space:
			for (int i = 0; i < char_width; i++) *z++ = bgcolor;
		}
		else // char 0x1X or attr change or 0x00
		{
			if constexpr (first_glyph < 0x20)
				if ((c & 0x1f) >= first_glyph) goto render_char;

			if (c == 0x00) // end of (collapsed) line
			{
				for (int i = col * char_width; i < width; i++) *z++ = bgcolor;
				return;
			}

			bold = c & 8;

			if (c & 7) // color change
			{
				int fg	   = (attr >> 1) & 3;
				int old_bg = (attr & 1) == 0 ? 0 : fg >= 2 ? fg : 1;

				attr = c;

				fg		   = (c >> 1) & 3;
				int new_bg = (c & 1) == 0 ? 0 : fg >= 2 ? fg : 1;

				fgcolor = fgcolors[fg];
				if (new_bg < old_bg) bgcolor = bgcolors[new_bg];
				for (int i = 0; i < char_width; i++) *z++ = bgcolor;
				if (new_bg > old_bg) bgcolor = bgcolors[new_bg];
			}
			else
			{
				attr = c;
				goto render_space;
			}
		}
	}
}

} // namespace kilipili::Video


/*

fgcolor =	0	1	2	3	0	1	2	3
bgcolor	=	0				1
-----------------------------------------
bgcolor	=	0	0	0	0	1	1	2	3



































*/
