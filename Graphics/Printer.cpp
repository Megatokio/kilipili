// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Printer.h"
#include "USBHost/USBKeyboard.h"
#include "common/basic_math.h"
#include "common/trace.h"
#include "cstrings.h"
#include <cstdio>
#include <memory>


namespace kilipili::Graphics
{

Printer<>::Printer(int rows, int cols) noexcept : rows(rows), cols(cols) {}

void Printer<>::reset(bool cls) noexcept
{
	hideCursor();
	row = col	 = 0;
	scroll_count = 0;
	dx = dy	   = 1;
	attributes = 0; // NORMAL;
}

void Printer<>::showCursor(bool on) noexcept
{
	if (cursor_visible == on) return;
	if (on)
	{
		validate_hpos(false);
		validate_vpos();
	}
	show_cursor(on);
}

void Printer<>::hideCursor() noexcept
{
	if (cursor_visible) show_cursor(false);
}

void Printer<>::validate_hpos(bool col80ok) noexcept
{
	// wraps & scrolls

	assert(!cursor_visible);
	if unlikely (uint(col) >= uint(cols) + col80ok)
	{
		while (col < 0)
		{
			col += cols;
			row -= dy;
		}
		while (col >= cols + col80ok)
		{
			col -= cols;
			row += dy;
		}
		validate_vpos();
	}
}

void Printer<>::validate_vpos() noexcept
{
	// scrolls

	assert(!cursor_visible);
	if unlikely (uint(row) >= uint(rows))
	{
		if (row < 0)
		{
			scroll_count += row;
			scrollScreenDown(-row);
			row = 0;
		}
		else
		{
			scroll_count += row - (rows - 1);
			scrollScreenUp(row - (rows - 1));
			row = rows - 1;
		}
	}
}

void Printer<>::validateCursorPosition(bool col80ok) noexcept
{
	// validate cursor position
	// moves cursor into previous/next line if the column is out of screen
	// scrolls the screen up or down if the row is out of screen
	// afterwards, the cursor is inside the screen
	// except if col80ok then allow the cursor in col = screen_width

	// col := in range [0 .. [screen_width
	// row := in range [0 .. [screen_height

	if (cursor_visible) return;
	validate_hpos(col80ok);
	validate_vpos();
}

void Printer<>::limitCursorPosition() noexcept
{
	// limit cursor position: don't wrap, don't scroll

	if (cursor_visible) return;
	limit(0, col, cols - 1);
	limit(0, row, rows - 1);
}

void Printer<>::moveTo(int row, int col, AutoWrap auto_wrap) noexcept
{
	// auto_wrap=0: stop at border.
	// auto_wrap=1: wrap & scroll, col80ok.

	hideCursor();
	this->row = row;
	this->col = col;
	if (auto_wrap) validateCursorPosition(true);
	else limitCursorPosition();
}

void Printer<>::moveToCol(int col, AutoWrap auto_wrap) noexcept
{
	// auto_wrap=0: stop at border.
	// auto_wrap=1: wrap & scroll, col80ok.

	hideCursor();
	this->col = col;
	if (auto_wrap) validate_hpos(true);
	else limit(0, this->col, cols - 1);
}

void Printer<>::moveToRow(int row, AutoWrap auto_wrap) noexcept
{
	// auto_wrap=0: stop at border.
	// auto_wrap=1: wrap & scroll.

	hideCursor();
	this->row = row;
	if (auto_wrap) validate_vpos();
	else limit(0, this->row, rows - 1);
}

void Printer<>::cursorLeft(int count, AutoWrap auto_wrap) noexcept
{
	moveToCol(col - max(count * dx, 0), auto_wrap); //
}

void Printer<>::cursorRight(int count, AutoWrap auto_wrap) noexcept
{
	// if auto_wrap then col80ok

	moveToCol(col + max(count * dx, 0), auto_wrap);
}

void Printer<>::cursorUp(int count, AutoWrap auto_wrap) noexcept
{
	moveToRow(row - max(count * dx, 0), auto_wrap); //
}

void Printer<>::cursorDown(int count, AutoWrap auto_wrap) noexcept
{
	moveToRow(row + max(count * dx, 0), auto_wrap); //
}

void Printer<>::cursorTab(int count) noexcept
{
	// scrolls, allows col = cols
	// note: if cols%8 != 0 then there is a last tab stop at cols
	// TODO: auto_wrap flag

	hideCursor();
	if (count > 0)
	{
		if (col >= cols)
		{
			col = 0;
			row += dy;
		}

		col = ((col >> 3) + count) << 3;

		int xcols = (cols + 7) & ~7;
		while (col > xcols)
		{
			row += dy;
			col -= xcols;
		}

		if (col > cols) col = cols;

		validate_vpos();
	}
}

void Printer<>::cursorReturn() noexcept
{
	// COL := 0

	hideCursor();
	col = 0;
}

void Printer<>::newLine() noexcept
{
	hideCursor();
	col = 0;
	row += dy;
	//validate_vpos();
}

void Printer<>::scrollRect(int row, int col, int rows, int cols, int dy, int dx) noexcept
{
	// clang-format off
	if unlikely (row < 0) { rows += row; row = 0; }
	if unlikely (row + rows > this->rows) rows = this->rows - row;
	if unlikely (col < 0) { cols += col; col = 0; }
	if unlikely (col + cols > this->cols) cols = this->cols - col;
	// clang-format on

	int h = rows - abs(dy);
	int w = cols - abs(dx);

	if (w <= 0 || h <= 0) return clearRect(row, col, rows, cols);

	int qx = dx >= 0 ? 0 : -dx;
	int zx = dx >= 0 ? +dx : 0;
	int qy = dy >= 0 ? 0 : -dy;
	int zy = dy >= 0 ? +dy : 0;

	copyRect(row + zy, col + zx, row + qy, col + qx, h, w);

	if (dx > 0) clearRect(row, col, rows, +dx);
	if (dx < 0) clearRect(row, col + w, rows, -dx);
	if (dy > 0) clearRect(row, col, +dy, cols);
	if (dy < 0) clearRect(row + h, col, -dy, cols);
}

void Printer<>::scrollRectLeft(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, 0, -dist);
}

void Printer<>::scrollRectRight(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, 0, +dist);
}

void Printer<>::scrollRectUp(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, -dist, 0);
}

void Printer<>::scrollRectDown(int row, int col, int rows, int cols, int dist) noexcept
{
	if (dist > 0) scrollRect(row, col, rows, cols, +dist, 0);
}

void Printer<>::insertRows(int n) noexcept { scrollRectDown(row, 0, rows - row, cols, n); }

void Printer<>::deleteRows(int n) noexcept { scrollRectUp(row, 0, rows - row, cols, n); }

void Printer<>::insertColumns(int n) noexcept { scrollRectRight(0, col, rows, cols - col, n); }

void Printer<>::deleteColumns(int n) noexcept { scrollRectLeft(0, col, rows, cols - col, n); }

void Printer<>::insertChars(int n) noexcept { scrollRectRight(row, col, 1, cols - col, n); }

void Printer<>::deleteChars(int n) noexcept { scrollRectLeft(row, col, 1, cols - col, n); }

void Printer<>::clearToStartOfLine(bool incl_cpos) noexcept
{
	// allow col80, even if incl_cpos = true

	clearRect(row, 0, 1, col + incl_cpos);
}

void Printer<>::clearToStartOfScreen(bool incl_cpos) noexcept
{
	// allow col80, even if incl_cpos = true

	clearToStartOfLine(incl_cpos);
	clearRect(0, 0, row, cols);
}

void Printer<>::clearToEndOfLine() noexcept
{
	// allow col80
	// this allows to print an arbitrary string up to the last char and clear to eol.

	clearRect(row, col, 1, cols - col);
}

void Printer<>::clearToEndOfScreen() noexcept
{
	// allow col80
	// this allows to print an arbitrary string up to the last char and clear to end of screen
	// without scrolling and inserting a new empty line.

	clearToEndOfLine();
	clearRect(row + 1, 0, rows - (row + 1), cols);
}

void Printer<>::scrollScreen(int dy /*chars*/, int dx /*chars*/) noexcept
{
	int w = (cols - abs(dx));
	int h = (rows - abs(dy));

	if (w <= 0 || h <= 0) return clearRect(0, 0, rows, cols);

	int qx = dx >= 0 ? 0 : -dx;
	int zx = dx >= 0 ? +dx : 0;
	int qy = dy >= 0 ? 0 : -dy;
	int zy = dy >= 0 ? +dy : 0;

	copyRect(zy, zx, qy, qx, h, w);

	if (dx > 0) clearRect(0, 0, rows, +dx);
	if (dx < 0) clearRect(0, w, rows, -dx);
	if (dy > 0) clearRect(0, 0, +dy, cols);
	if (dy < 0) clearRect(h, 0, -dy, cols);
}

void Printer<>::scrollScreenUp(int rows) noexcept
{
	if (rows > 0) scrollScreen(-rows, 0);
}

void Printer<>::scrollScreenDown(int rows) noexcept
{
	if (rows > 0) scrollScreen(+rows, 0);
}

void Printer<>::scrollScreenLeft(int cols) noexcept
{
	if (cols > 0) scrollScreen(0, -cols);
}

void Printer<>::scrollScreenRight(int cols) noexcept
{
	if (cols > 0) scrollScreen(0, +cols);
}

void Printer<>::write(cptr text, int count) noexcept
{
	// any char, even chr(0)

	for (int i = 0; i < count; i++) { printChar(text[i]); }
}

void Printer<>::print(cstr s) noexcept
{
	// print printable text string.
	// control characters: only \t and \n.

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

		printChar(c);
	}
}

void Printer<>::printAt(int row, int col, cstr text) noexcept
{
	moveTo(row, col);
	print(text);
}

void Printer<>::printf(cstr fmt, va_list va) noexcept
{
	// note: caller must call va_start() before and va_end() afterwards.

	constexpr uint bsz = 200;
	char		   bu[bsz];

	va_list va2;
	va_copy(va2, va);
	uint size = uint(vsnprintf(bu, sizeof(bu), fmt, va2));
	va_end(va2);

	if (size < bsz) return print(bu);			   // success
	if (int(size) < 0) return print("{format?!}"); // format error

	// very long text:

	std::unique_ptr<char[]> bp {new (std::nothrow) char[size + 1]};
	if (bp)
	{
		vsnprintf(bp.get(), size + 1, fmt, va);
		print(bp.get());
	}
	else // out of memory! print what we have:
	{
		bu[bsz - 1] = 0;
		print(bu);
	}
}

void Printer<>::printf(cstr fmt, ...) noexcept
{
	va_list va;
	va_start(va, fmt);
	printf(fmt, va);
	va_end(va);
}

str Printer<>::inputLine(std::function<int()> getc, str oldtext, int epos)
{
	// enter a new line or edit an existing line of text by the user.
	// supports multiple types of cursor control.
	// note: getc() may combine multiple sources and run a state machine.
	// todo: positioning is still a little bit buggy.
	// returns a tempstr

	using namespace USB;
	trace(__func__);

	enum {
		BACKSPACE = 8,
		RUBOUT	  = 0x7f,
		RETURN	  = 13,
		ESC		  = 0x1b,
		CSI		  = 0x9b, // C1 version of ESC[
	};

	if (oldtext == nullptr) oldtext = emptystr;
	assert(epos <= int(strlen(oldtext)));

	int col0 = col;
	int row0 = row + scroll_count;

	print(oldtext);

	// TODO: this is not yet perfect, because while editing tempmem piles up:
	TempMemSave _;

	for (;;)
	{
		moveTo(row0 - scroll_count, col0 + epos, wrap);
		showCursor();
		int c = getc();

		if (c <= 0xff)
		{
			if (is_printable(char(c)))
			{
				oldtext = catstr(leftstr(oldtext, epos), charstr(char(c)), oldtext + epos);
				print(oldtext + epos++);
				continue;
			}

			// else it's a control code:
			switch (c)
			{
			case RETURN:
				print(oldtext + epos);
				newLine();
				return xdupstr(oldtext);
			case RUBOUT:
			case BACKSPACE: c = KEY_BACKSPACE; break;
			case ESC:
			case CSI:
				if (c == ESC) c = getc();
				if (c == '[') c = getc();
				switch (c)
				{
				case '3': // ESC[3~
					if (getc() != '~') continue;
					c = KEY_DELETE;
					break;
				case 'A': c = KEY_ARROW_UP; break;	  // ESC[A
				case 'B': c = KEY_ARROW_DOWN; break;  // ESC[B
				case 'C': c = KEY_ARROW_RIGHT; break; // ESC[C
				case 'D': c = KEY_ARROW_LEFT; break;  // ESC[D
				default: printf("{ESC,0x%02x}", uint(c)); continue;
				}
				break;
			default: printf("{0x%02x}", uint(c)); continue;
			}
		}

		// it's a USB key code:

		if (c == HID_KEY_OTHER + KEY_BACKSPACE + (LEFTSHIFT << 16)) //
		{
			c = KEY_DELETE;
		}

		if (c >> 16) // with modifiers
		{
			printf("{%s+%s}", tostr(HIDKey(c & 0xff)), tostr(Modifiers(c >> 16), true));
			continue;
		}

		switch (c & 0xff) // the USB keycode
		{
		case KEY_BACKSPACE:
			if (epos == 0) break;
			epos--;
			cursorLeft();
			[[fallthrough]];
		case KEY_DELETE:
			if (oldtext[epos] == 0) break;
			oldtext = catstr(leftstr(oldtext, epos), oldtext + epos + 1);
			print(oldtext + epos);
			printChar(' ');
			break;
		case KEY_ARROW_LEFT: epos = max(epos - 1, 0); break;
		case KEY_ARROW_RIGHT:
			if (oldtext[epos] != 0) printChar(oldtext[epos++]);
			break;
		case KEY_ARROW_UP: epos = max(epos - cols, 0); break;
		case KEY_ARROW_DOWN: epos = min(epos + cols, int(strlen(oldtext))); break;
		default: printf("{%s}", tostr(HIDKey(c & 0xff)));
		}
	}
}


} // namespace kilipili::Graphics


/*



































*/
