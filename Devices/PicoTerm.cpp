// Copyright (c) 2012 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "PicoTerm.h"
#include "common/basic_math.h"
#include "common/cstrings.h"

namespace kilipili
{

// =============================================================================
//							F U N C T I O N S
// =============================================================================


PicoTerm::PicoTerm(RCPtr<Printer<>> text) noexcept : //
	SerialDevice(writable),
	printer(text)
{
	//auto_crlf = true;
	//sm_state  = 0;
	//reset();
}

void PicoTerm::reset() noexcept
{
	// all settings = default, home cursor
	// does not clear screen

	auto_crlf = true;
	sm_state  = 0;

	printer->reset();
}

uint32 PicoTerm::ioctl(IoCtl cmd, void* arg1, void* arg2)
{
	switch (cmd.cmd)
	{
	case IoCtl::FLUSH_OUT: return 0;
	case IoCtl::CTRL_RESET: reset(); return 0;
	default: return super::ioctl(cmd, arg1, arg2);
	}
}

#define BEGIN         \
	switch (sm_state) \
	{                 \
	default:

#define GETC()               \
	while (idx == count)     \
	{                        \
		sm_state = __LINE__; \
		return count;        \
	case __LINE__:;          \
	}                        \
	c = data[idx++]

#define FINISH }

SIZE PicoTerm::write(const void* _data, SIZE count, bool /*partial*/)
{
	cptr data = cptr(_data);
	uint idx  = 0;
	char c;

	// clang-format off
	BEGIN
	for (;;)
	{
		repeat_cnt = 1;

	loop_repeat:
		GETC();
		if (uchar(c) >= 0x20 && c != 127) { printer->printChar(c, repeat_cnt); continue; }

		switch (c) 
		{
		case RESET:			   reset(); continue;
		case CLS:			   printer->cls(); continue;
		case MOVE_TO_POSITION: goto move_to_position;
		case MOVE_TO_COL:      goto move_to_col;
		case SHOW_CURSOR:	   printer->showCursor(); continue;
		case CURSOR_LEFT:	   printer->cursorLeft(repeat_cnt); continue;  // scrolls
		case TAB:			   printer->cursorTab(repeat_cnt); continue;	  // scrolls
		case CURSOR_DOWN:	   printer->cursorDown(repeat_cnt); if (auto_crlf) printer->cursorReturn(); continue;  // scrolls
		case CURSOR_UP:		   printer->cursorUp(repeat_cnt); continue;	  // scrolls
		case CURSOR_RIGHT:	   printer->cursorRight(repeat_cnt); continue; // scrolls
		case RETURN:		   printer->cursorReturn(); continue;
		case CLEAR_TO_END_OF_LINE:   printer->clearToEndOfLine(); continue;
		case CLEAR_TO_END_OF_SCREEN: printer->clearToEndOfScreen(); continue;
		case SET_ATTRIBUTES:   goto set_attributes;
		case REPEAT_NEXT_CHAR: goto repeat_next_char;
		case SCROLL_SCREEN:    goto scroll_screen;
		case ESC:			   goto esc;
		default:			   break;
		}

		if (c == 127)
		{
			auto attr = printer->getAttributes();
			printer->removeAttributes(printer->transparent);
			printer->cursorLeft(repeat_cnt);
			printer->printChar(' ', repeat_cnt);
			printer->cursorLeft(repeat_cnt);
			printer->setAttributes(attr);
		}
		else printf("{$%02X}", c);
		continue;

		move_to_position: GETC(); printer->moveToRow(int8(c-0x40)+0x40);
		move_to_col: GETC(); printer->moveToCol(int8(c-0x40)+0x40); continue;
		set_attributes: GETC(); printer->setAttributes(c); continue;
		repeat_next_char: GETC(); repeat_cnt = uchar(c); goto loop_repeat;
		scroll_screen: // scroll screen u/d/l/r
			GETC(); if (c == 'u') printer->scrollScreenUp(repeat_cnt);
			else if (c == 'd') printer->scrollScreenDown(repeat_cnt);
			else if (c == 'l') printer->scrollScreenLeft(repeat_cnt);
			else if (c == 'r') printer->scrollScreenRight(repeat_cnt);
			continue;
		esc:
			GETC(); if (c == '[')
			{
				GETC(); if (is_decimal_digit(c)) repeat_cnt = 0;
				while (is_decimal_digit(c)) { repeat_cnt = repeat_cnt * 10 + c - '0'; GETC(); }
				switch (c)
				{
				case 'A': printer->cursorUp(min(printer->row, repeat_cnt)); continue;				  // VT100
				case 'B': printer->cursorDown(min(printer->rows - 1 - printer->row, repeat_cnt)); continue;  // VT100
				case 'C': printer->cursorRight(min(printer->cols - 1 - printer->col, repeat_cnt)); continue; // VT100
				case 'D': printer->cursorLeft(min(printer->col, repeat_cnt)); continue;			  // VT100
				default:  break;
				}
			}
			puts("{ESC}");
			continue;
	}
	FINISH
	// clang-format on
}

cstr PicoTerm::identify()
{
	// PicoTerm gfx=400*300 txt=50*25 chr=8*12 cm=rgb
	// PicoTerm gfx=400*300 txt=50*25 chr=8*12 cm=i8 attr=8*12

	return usingstr("PicoTerm %s", printer->identify());
}

} // namespace kilipili

/*





































*/
