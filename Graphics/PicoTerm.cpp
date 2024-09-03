// Copyright (c) 2012 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "PicoTerm.h"
#include "USBHost/USBKeyboard.h"
#include "cstrings.h"
#include "glue.h"
#include "utilities/Trace.h"

namespace kio::Graphics
{

using namespace Devices;

// =============================================================================
//							F U N C T I O N S
// =============================================================================


PicoTerm::PicoTerm(CanvasPtr pixmap) : //
	SerialDevice(writable),
	TextVDU(pixmap)
{
	reset();
}

void PicoTerm::reset()
{
	// all settings = default, home cursor
	// does not clear screen

	auto_crlf = true;
	sm_state  = 0;

	TextVDU::reset();
}

uint32 PicoTerm::ioctl(IoCtl cmd, void*, void*)
{
	switch (cmd.cmd)
	{
	case IoCtl::FLUSH_OUT: return 0;
	case IoCtl::CTRL_RESET: reset(); return 0;
	default: throw INVALID_ARGUMENT;
	}
}

#define BEGIN       \
  switch (sm_state) \
  {                 \
  default:

#define GETC()           \
  while (idx == count)   \
  {                      \
	sm_state = __LINE__; \
	return count;        \
  case __LINE__:;        \
  }                      \
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
		if (is_printable(c)) { printChar(c, repeat_cnt); continue; }

		switch (c) 
		{
		case RESET:			   reset(); continue;
		case CLS:			   cls(); continue;
		case MOVE_TO_POSITION: goto move_to_position;
		case MOVE_TO_COL:      goto move_to_col;
		case SHOW_CURSOR:	   showCursor(); continue;
		case CURSOR_LEFT:	   cursorLeft(repeat_cnt); continue;  // scrolls
		case TAB:			   cursorTab(repeat_cnt); continue;	  // scrolls
		case CURSOR_DOWN:	   cursorDown(repeat_cnt); if (auto_crlf) cursorReturn(); continue;  // scrolls
		case CURSOR_UP:		   cursorUp(repeat_cnt); continue;	  // scrolls
		case CURSOR_RIGHT:	   cursorRight(repeat_cnt); continue; // scrolls
		case RETURN:		   cursorReturn(); continue;
		case CLEAR_TO_END_OF_LINE:   clearToEndOfLine(); continue;
		case CLEAR_TO_END_OF_SCREEN: clearToEndOfScreen(); continue;
		case SET_ATTRIBUTES:   goto set_attributes;
		case REPEAT_NEXT_CHAR: goto repeat_next_char;
		case SCROLL_SCREEN:    goto scroll_screen;
		case ESC:			   goto esc;
		}

		if (c == 127)
		{
			auto attr = attributes;
			attributes = Attributes(attributes & ~TRANSPARENT);
			cursorLeft(repeat_cnt);
			printChar(' ', repeat_cnt);
			cursorLeft(repeat_cnt);
			attributes = attr;
		}
		else printf("{$%02X}", c);
		continue;

		move_to_position: GETC(); moveToRow(int8(c-0x40)+0x40);
		move_to_col: GETC(); moveToCol(int8(c-0x40)+0x40); continue;
		set_attributes: GETC(); setCharAttributes(c); continue;
		repeat_next_char: GETC(); repeat_cnt = uchar(c); goto loop_repeat;
		scroll_screen: // scroll screen u/d/l/r
			GETC(); if (c == 'u') scrollScreenUp(repeat_cnt);
			else if (c == 'd') scrollScreenDown(repeat_cnt);
			else if (c == 'l') scrollScreenLeft(repeat_cnt);
			else if (c == 'r') scrollScreenRight(repeat_cnt);
			continue;
		esc:
			GETC(); if (c == '[')
			{
				GETC(); if (is_decimal_digit(c)) repeat_cnt = 0;
				while (is_decimal_digit(c)) { repeat_cnt = repeat_cnt * 10 + c - '0'; GETC(); }
				switch (c)
				{
				case 'A': cursorUp(min(row, repeat_cnt)); continue;				  // VT100
				case 'B': cursorDown(min(rows - 1 - row, repeat_cnt)); continue;  // VT100
				case 'C': cursorRight(min(cols - 1 - col, repeat_cnt)); continue; // VT100
				case 'D': cursorLeft(min(col, repeat_cnt)); continue;			  // VT100
				}
			}
			puts("{ESC}");
			continue;
	}
	FINISH
	// clang-format on
}

char* PicoTerm::identify()
{
	// PicoTerm gfx=400*300 txt=50*25 chr=8*12 cm=rgb
	// PicoTerm gfx=400*300 txt=50*25 chr=8*12 cm=i8 attr=8*12

	cstr amstr = attrmode == attrmode_none ? "" : usingstr(" attr=%u*%u", 1 << attrwidth, attrheight);

	return usingstr(
		"PicoTerm gfx=%u*%u txt=%u*%u chr=%u*%u cm=%s%s", pixmap->width, pixmap->height, cols, rows, CHAR_WIDTH,
		CHAR_HEIGHT, tostr(colordepth), amstr);
}

int PicoTerm::getc(void (*run_statemachines)(), int timeout_us)
{
	constexpr int32 crsr_phase = 600 * 1000; // 0.6 sec
	int32			now		   = int32(time_us_32());
	int32			crsr_start = now;
	int32			end		   = now + timeout_us;
	int				inchar	   = USB::getChar();

	while (inchar == -1 && (timeout_us == 0 || (now = int32(time_us_32())) - end < 0))
	{
		if (now - (crsr_start + crsr_phase) >= 0) crsr_start += crsr_phase;
		showCursor(now - (crsr_start + crsr_phase / 2) < 0);

		run_statemachines();
		inchar = USB::getChar();
	}

	return inchar;
}

str PicoTerm::input_line(void (*run_statemachines)(), str oldtext, int epos)
{
	trace(__func__);

	if (oldtext == nullptr) oldtext = emptystr;
	assert(epos <= int(strlen(oldtext)));

	int col0 = col;
	int row0 = row;

	print(oldtext);

	for (;;)
	{
		moveTo(row0, col0 + epos);
		int c = getc(run_statemachines);

		if (c < 32)
		{
			switch (c)
			{
			case RETURN:
				print(oldtext + epos);
				moveTo(row + 1, 0);
				return oldtext;
			case CURSOR_LEFT: c = USB::KEY_ARROW_LEFT; break;
			case CURSOR_RIGHT: c = USB::KEY_ARROW_RIGHT; break;
			case CURSOR_UP: c = USB::KEY_ARROW_UP; break;
			case CURSOR_DOWN: c = USB::KEY_ARROW_DOWN; break;
			case ESC:
				c = getc(run_statemachines);
				if (c != '[')
				{
					printf("{ESC,0x%02x}", c);
					continue;
				}
				c = getc(run_statemachines);
				switch (c)
				{
				case 'A': c = USB::KEY_ARROW_UP; break;	   // VT100
				case 'B': c = USB::KEY_ARROW_DOWN; break;  // VT100
				case 'C': c = USB::KEY_ARROW_RIGHT; break; // VT100
				case 'D': c = USB::KEY_ARROW_LEFT; break;  // VT100
				default: printf("{ESC[0x%02x}", c); continue;
				}
				break;
			default: printf("{0x%02x}", c); continue;
			}
		}

		else if (c == 127) //
		{
			c = USB::KEY_BACKSPACE;
		}

		else if (c <= 255)
		{
			oldtext = catstr(leftstr(oldtext, epos), charstr(char(c)), oldtext + epos);
			print(oldtext + epos++);
			continue;
		}

		else if (c == USB::HID_KEY_OTHER + USB::KEY_BACKSPACE + (USB::LEFTSHIFT << 16)) //
		{
			c = USB::KEY_DELETE;
		}

		if (c >> 16) // with modifiers
		{
			printf("{%s+%s}", tostr(USB::HIDKey(c & 0xff)), tostr(USB::Modifiers(c >> 16), true));
			continue;
		}

		static_assert(USB::KEY_ARROW_LEFT == 0x50);
		static_assert(USB::KEY_EXSEL == 0xA4);
		static_assert(USB::KEY_GUI_RIGHT == 0xE7);

		switch (c & 0xff)
		{
		case USB::KEY_BACKSPACE:
			if (epos == 0) break;
			epos--;
			cursorLeft();
			[[fallthrough]];
		case USB::KEY_DELETE:
			if (oldtext[epos] == 0) break;
			oldtext = catstr(leftstr(oldtext, epos), oldtext + epos + 1);
			print(oldtext + epos);
			printChar(' ');
			break;
		case USB::KEY_ARROW_LEFT: epos = max(epos - 1, 0); break;
		case USB::KEY_ARROW_RIGHT:
			if (oldtext[epos] != 0) printChar(oldtext[epos++]);
			break;
		case USB::KEY_ARROW_UP: epos = max(epos - cols, 0); break;
		case USB::KEY_ARROW_DOWN: epos = min(epos + cols, int(strlen(oldtext))); break;
		default: printf("{%s}", tostr(USB::HIDKey(c & 0xff)));
		}
	}
}

} // namespace kio::Graphics

/*





































*/
