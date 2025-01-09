// Copyright (c) 2012 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "PicoTerm.h"
#include "USBHost/USBKeyboard.h"
#include "common/cstrings.h"

namespace kio::Devices
{

using namespace Graphics;

// =============================================================================
//							F U N C T I O N S
// =============================================================================


PicoTerm::PicoTerm(CanvasPtr pixmap) : //
	PicoTerm(new TextVDU(pixmap))
{}

PicoTerm::PicoTerm(RCPtr<TextVDU> text) noexcept : //
	SerialDevice(writable),
	text(std::move(text))
{
	reset();
}

void PicoTerm::reset() noexcept
{
	// all settings = default, home cursor
	// does not clear screen

	auto_crlf = true;
	sm_state  = 0;

	text->reset();
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
		if (is_printable(c)) { text->printChar(c, repeat_cnt); continue; }

		switch (c) 
		{
		case RESET:			   reset(); continue;
		case CLS:			   text->cls(); continue;
		case MOVE_TO_POSITION: goto move_to_position;
		case MOVE_TO_COL:      goto move_to_col;
		case SHOW_CURSOR:	   text->showCursor(); continue;
		case CURSOR_LEFT:	   text->cursorLeft(repeat_cnt); continue;  // scrolls
		case TAB:			   text->cursorTab(repeat_cnt); continue;	  // scrolls
		case CURSOR_DOWN:	   text->cursorDown(repeat_cnt); if (auto_crlf) text->cursorReturn(); continue;  // scrolls
		case CURSOR_UP:		   text->cursorUp(repeat_cnt); continue;	  // scrolls
		case CURSOR_RIGHT:	   text->cursorRight(repeat_cnt); continue; // scrolls
		case RETURN:		   text->cursorReturn(); continue;
		case CLEAR_TO_END_OF_LINE:   text->clearToEndOfLine(); continue;
		case CLEAR_TO_END_OF_SCREEN: text->clearToEndOfScreen(); continue;
		case SET_ATTRIBUTES:   goto set_attributes;
		case REPEAT_NEXT_CHAR: goto repeat_next_char;
		case SCROLL_SCREEN:    goto scroll_screen;
		case ESC:			   goto esc;
		}

		if (c == 127)
		{
			auto attr = text->attributes;
			text->removeAttributes(text->TRANSPARENT);
			text->cursorLeft(repeat_cnt);
			text->printChar(' ', repeat_cnt);
			text->cursorLeft(repeat_cnt);
			text->setAttributes(attr);
		}
		else printf("{$%02X}", c);
		continue;

		move_to_position: GETC(); text->moveToRow(int8(c-0x40)+0x40);
		move_to_col: GETC(); text->moveToCol(int8(c-0x40)+0x40); continue;
		set_attributes: GETC(); text->setAttributes(c); continue;
		repeat_next_char: GETC(); repeat_cnt = uchar(c); goto loop_repeat;
		scroll_screen: // scroll screen u/d/l/r
			GETC(); if (c == 'u') text->scrollScreenUp(repeat_cnt);
			else if (c == 'd') text->scrollScreenDown(repeat_cnt);
			else if (c == 'l') text->scrollScreenLeft(repeat_cnt);
			else if (c == 'r') text->scrollScreenRight(repeat_cnt);
			continue;
		esc:
			GETC(); if (c == '[')
			{
				GETC(); if (is_decimal_digit(c)) repeat_cnt = 0;
				while (is_decimal_digit(c)) { repeat_cnt = repeat_cnt * 10 + c - '0'; GETC(); }
				switch (c)
				{
				case 'A': text->cursorUp(min(text->row, repeat_cnt)); continue;				  // VT100
				case 'B': text->cursorDown(min(text->rows - 1 - text->row, repeat_cnt)); continue;  // VT100
				case 'C': text->cursorRight(min(text->cols - 1 - text->col, repeat_cnt)); continue; // VT100
				case 'D': text->cursorLeft(min(text->col, repeat_cnt)); continue;			  // VT100
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

	cstr amstr = text->attrmode == attrmode_none ? "" : usingstr(" attr=%u*%u", 1 << text->attrwidth, text->attrheight);

	return usingstr(
		"PicoTerm gfx=%u*%u txt=%u*%u chr=%u*%u cm=%s%s", text->pixmap->width, text->pixmap->height, text->cols,
		text->rows, text->CHAR_WIDTH, text->CHAR_HEIGHT, tostr(text->colordepth), amstr);
}

} // namespace kio::Devices

/*





































*/
