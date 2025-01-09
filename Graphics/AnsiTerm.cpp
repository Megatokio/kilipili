// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "AnsiTerm.h"
#include "Graphics/Canvas.h"
#include "Graphics/Color.h"
#include "Graphics/ColorMap.h"
#include "USBHost/HidKeys.h"
#include "USBHost/USBKeyboard.h"
#include "USBHost/USBMouse.h"
#include "USBHost/hid_handler.h"
#include "common/cstrings.h"

namespace kio::Audio
{
extern void __weak_symbol beep(float frequency_hz = 880, float volume = .5f, uint32 duration_ms = 600);
}

// ##########################################################

namespace kio::Graphics
{

using AutoWrap = TextVDU::AutoWrap;

static constexpr int			   nochar = -1;
static constexpr AutoWrap		   nowrap = TextVDU::nowrap;
__unused static constexpr AutoWrap wrap	  = TextVDU::wrap;

static char default_import_char(uint c) noexcept { return c <= 0xff ? char(c) : '_'; }
static uint default_export_char(char c) noexcept { return uchar(c); }

static inline constexpr bool bit_at_index(uchar bits[], uint i) { return (bits[i / 8] >> (i % 8)) & 1; }
static inline void			 clear_bit_at_index(uchar bits[], uint i) { bits[i / 8] &= ~(1 << i % 8); }
__unused static inline void	 set_bit_at_index(uchar bits[], uint i) { bits[i / 8] |= (1 << i % 8); }

static inline constexpr bool is_fup(int c) noexcept { return schar(c) < schar(0xc0); }

static constexpr uint sizeof_utf8(char c) noexcept
{
	assert(uchar(c) >= 0xc0);
	if (uchar(c) < 0xe0) return 2;
	if (uchar(c) < 0xf0) return 3;
	if (uchar(c) < 0xf8) return 4;
	if (uchar(c) < 0xfc) return 5;
	return 6;
}

static uint16 decodeUtf8(const uchar* p) noexcept
{
	uint c = *p++;
	assert(c >= 0x80);
	assert(!is_fup(int(c)));
	uint c1 = c;

	c = *p++;
	assert(is_fup(int(c)));
	if (c1 < 0xE0) return uint16(((c1 & 0x1F) << 6) + (c & 0x3F));
	uint c2 = c;

	c = *p++;
	assert(is_fup(int(c)));
	if (c1 < 0xF0) return uint16(((c1 & 0x0F) << 12) + ((c2 & 0x3F) << 6) + (c & 0x3F));

	return '_'; // too large for UCS2
}

static uint encodeUtf8(uint c, char* z) noexcept
{
	// encode ucs2 char to utf-8
	// return: size of utf8 text

	if (c < 0x80)
	{
		*z++ = char(c);
		return 1;
	}
	else if (c < 0x800)
	{
		*z++ = char(0xC0) + char(c >> 6);
		*z++ = char(0x80 + (c & 0x3F));
		return 2;
	}
	else
	{
		*z++ = char(0xE0) + char(c >> 12);
		*z++ = char(0x80 + ((c >> 6) & 0x3F));
		*z++ = char(0x80 + (c & 0x3F));
		return 3;
	}
}

static uint buttons_for_buttons(uint usb_buttons) noexcept
{
	// convert to buttons mask for ansi reply
	// ansi	buttons:
	// 	  1   right button down.
	// 	  2   middle button down.
	// 	  4   left button down.
	// 	  8   M4 button down.

	uint b = usb_buttons;
	return (b & USB::LEFT_BUTTON ? 4 : 0) +		//
		   (b & USB::MIDDLE_BUTTON ? 2 : 0) +	//
		   (b & USB::RIGHT_BUTTON ? 1 : 0) +	//
		   (b & USB::BACKWARD_BUTTON ? 8 : 0) + //
		   (b & USB::FORWARD_BUTTON ? 8 : 0);
}


// ##########################################################

AnsiTerm::AnsiTerm(CanvasPtr pixmap, Color* colormap) :
	import_char(&default_import_char),
	export_char(&default_export_char),
	full_pixmap(pixmap),
	colormap(colormap)
{
	USB::setScreenSize(pixmap->width, pixmap->height);
	display = new TextVDU(pixmap);
	memset(htabs, 0x01, sizeof(htabs)); // lsb = leftmost position within 8 chars
}

void AnsiTerm::reset(bool hard) noexcept
{
	// reset to initial state
	// note: leaves the cursor off even if show_cursor=true

	// soft reset:
	// reset what is saved in pushCursor()/popCursor()

	if (display->pixmap.ptr() != full_pixmap.ptr()) display = new TextVDU(full_pixmap);
	else display->reset();
	insert_mode		   = false;
	cursor_visible	   = true;
	lr_margins_enabled = false;
	tb_margins_enabled = false;
	lr_set_by_csir	   = false;
	top_margin		   = 0;
	bottom_margin	   = 0;
	left_margin		   = 0;
	right_margin	   = 0;

	// additional resets:

	lr_ever_set_by_csis = false;
	memset(htabs, 0x01, NELEM(htabs));

	if (hard)
	{
		flush_in();
		flush_out();
		while (sp) stack[--sp].display = nullptr;

		utf8_mode		 = default_utf8_mode;
		c1_codes_8bit	 = default_c1_codes_8bit;
		application_mode = default_application_mode;
		local_echo		 = default_local_echo;
		newline_mode	 = default_newline_mode;
		auto_wrap		 = default_auto_wrap;
		sgr_cumulative	 = default_sgr_cumulative;

		display->cls();
	}
}

void AnsiTerm::push_cursor() noexcept
{
	// VT100: DECSC saves the following:
	//	 cursor position
	//	 graphic rendition
	//	 character set shift state
	//	 state of wrap flag
	//	 state of origin mode
	//	 state of selective erase

	if (sp >= NELEM(stack)) return;
	SavedState& s = stack[sp];
	s.display	  = new (std::nothrow) TextVDU(full_pixmap);
	if (!s.display) return; // silently ignore but don't throw!
	std::swap(display, s.display);
	sp++;

	s.insert_mode		 = insert_mode;
	s.cursor_visible	 = cursor_visible;
	s.lr_margins_enabled = lr_margins_enabled;
	s.tb_margins_enabled = tb_margins_enabled;
	s.lr_set_by_csir	 = lr_set_by_csir;
	s.top_margin		 = uint8(top_margin);
	s.bottom_margin		 = uint8(bottom_margin);
	s.left_margin		 = uint8(left_margin);
	s.right_margin		 = uint8(right_margin);

	insert_mode		   = false;
	cursor_visible	   = false;
	lr_margins_enabled = false;
	tb_margins_enabled = false;
	lr_set_by_csir	   = false;
}

void AnsiTerm::pop_cursor() noexcept
{
	// VT100: DECRC restores the states described for (DECSC) above.
	// If none of these characteristics were saved, the cursor moves to home position;
	// origin mode is reset; no character attributes are assigned;
	// and the default character set mapping is established.

	if (sp)
	{
		SavedState& s = stack[--sp];
		std::swap(display, s.display);
		s.display = nullptr;

		insert_mode		   = s.insert_mode;
		cursor_visible	   = s.cursor_visible;
		lr_margins_enabled = s.lr_margins_enabled;
		tb_margins_enabled = s.tb_margins_enabled;
		lr_set_by_csir	   = s.lr_set_by_csir;
		top_margin		   = s.top_margin;
		bottom_margin	   = s.bottom_margin;
		left_margin		   = s.left_margin;
		right_margin	   = s.right_margin;
	}
	else
	{
		reset(false); //
	}
}

void AnsiTerm::apply_margins() noexcept
{
	// set display for horizontal and/or vertical window or fullscreen
	// cursor position is reset to 0,0

	try
	{
		uint				bgcolor	   = display->bgcolor;
		uint				fgcolor	   = display->fgcolor;
		TextVDU::Attributes attributes = display->attributes;

		if (tb_margins_enabled || lr_margins_enabled) // partial window mode
		{
			int width  = full_pixmap->width / display->CHAR_WIDTH;
			int height = full_pixmap->height / display->CHAR_HEIGHT;

			int l = 1;
			int r = width;
			int t = 1;
			int b = height;

			if (tb_margins_enabled)
			{
				if (top_margin) t = top_margin;
				if (bottom_margin) b = bottom_margin;
				if (t > b || b > height) return; // error
			}

			if (lr_margins_enabled)
			{
				if (left_margin) l = left_margin;
				if (right_margin) r = right_margin;
				if (l > r || r > width) return; // error
			}

			l = --l * display->CHAR_WIDTH;
			r = r * display->CHAR_WIDTH;
			t = --t * display->CHAR_HEIGHT;
			b = b * display->CHAR_HEIGHT;

			display->hideCursor();
			CanvasPtr partial_pixmap = full_pixmap->cloneWindow(l, t, r - l, b - t);
			display					 = new TextVDU(partial_pixmap);
		}
		else // back to fullscreen
		{
			display->hideCursor();
			display = new TextVDU(full_pixmap);
		}

		display->bgcolor = bgcolor;
		display->fgcolor = fgcolor;
		display->setAttributes(attributes);
	}
	catch (...) // silently ignore
	{}
}


void AnsiTerm::log_rbu()
{
	char  bu[80];
	char* p = bu;

	*p++ = '{';

	uint i = 0;
	do {
		char c = char(wbu[i++]);
		if (is_printable(c)) *p++ = c;
		else if (c == 0x1b && i == 1) p = strcpy(p, "ESC") + 3;
		else if (c == '\n') p = strcpy(p, "\\n\n") + 3;
		else if (c == '\r') p = strcpy(p, "\\r") + 2;
		else if (wcnt <= 1) p += sprintf(p, "0x%02x", uchar(c));
		else p += sprintf(p, "\\x%02x", uchar(c));
	}
	while (i < wcnt);

	*p++ = '}';
	*p++ = 0;
	display->print(bu);
}

void AnsiTerm::put_csi_response(cstr s, ...)
{
	// message s is the response without CSI which may be encoded in various variants

	char	bu[40];
	va_list va;
	va_start(va, s);
	uint len = uint(vsnprintf(bu, sizeof(bu), s, va));
	assert(len < sizeof(bu));
	va_end(va);

	if (inputbuffer.free() < len + 1 + (!c1_codes_8bit || utf8_mode)) return; // silently drop it

	if (!c1_codes_8bit) inputbuffer.write("\x1b[", 2);
	else if (utf8_mode) inputbuffer.write("\xc2\x9b", 2);
	else inputbuffer.put('\x9b');

	inputbuffer.write(bu, len);
}

void AnsiTerm::handle_tab(uint n)
{
	// VT100: Moves cursor to next tab stop, or to right margin
	//		  if there are no more tab stops. Does not cause autowrap.
	// ECMA-48: CHT - Cursor Forward Tabulation n tab stops.  (display)

	int col = display->col;
	while (n && col + 1 < display->cols) { n -= bit_at_index(htabs, uint(++col)); }
	display->moveToCol(col, nowrap);
}

void AnsiTerm::handle_backTab(uint n)
{
	// ECMA-48: CBT - Cursor Backward Tabulation n tab stops. (display)

	int col = display->col;
	while (n && col > 0) { n -= bit_at_index(htabs, uint(--col)); }
	display->moveToCol(col, nowrap);
}


void AnsiTerm::handle_sendDA()
{
	// DA: send primary device attributes

	// Response:
	// ⇒  CSI ? 1 ; 2 c		("VT100 with Advanced Video Option")
	// ⇒  CSI ? 1 ; 0 c		("VT101 with No Options")
	// ⇒  CSI ? 4 ; 6 c		("VT132 with Advanced Video and Graphics")
	// ⇒  CSI ? 6 c			("VT102")
	// ⇒  CSI ? 7 c			("VT131")
	// ⇒  CSI ? 12 ; n c	("VT125")
	// ⇒  CSI ? 62 ; n c	("VT220")
	// ⇒  CSI ? 63 ; n c	("VT320")
	// ⇒  CSI ? 64 ; n c	("VT420")
	// ⇒  CSI ? 65 ; n c	("VT510" to ("VT525")
	//
	// The VT100-style response parameters do not mean anything by
	// themselves.  VT220 (and higher) parameters do, telling the
	// host what features the terminal supports:
	// 	n = 1  ⇒  132-columns.
	// 	n = 2  ⇒  Printer.
	// 	n = 3  ⇒  ReGIS graphics: a graphics description language
	// 	n = 4  ⇒  Sixel graphics: a paletted bitmap graphics system
	// 	n = 6  ⇒  Selective erase.
	// 	n = 8  ⇒  User-defined keys.
	// 	n = 9  ⇒  National Replacement Character sets.
	// 	n = 15 ⇒  Technical characters.
	// 	n = 16 ⇒  Locator port.
	// 	n = 17 ⇒  Terminal state interrogation.
	// 	n = 18 ⇒  User windows.
	// 	n = 21 ⇒  Horizontal scrolling.
	// 	n = 22 ⇒  ANSI color, e.g., VT525.
	// 	n = 28 ⇒  Rectangular editing.
	// 	n = 29 ⇒  ANSI text locator (i.e., DEC Locator mode).

	put_csi_response("62;16;21;22c");
}

void AnsiTerm::handle_C0(char c)
{
	// handle control code 0x00 … 0x1f
	// codes without parameter are handled immediately
	// ESC changes state to EscPending

	switch (c)
	{
	case 0x07: // BELL
	{
		if (kio::Audio::beep) kio::Audio::beep();
		return;
	}
	case 0x08: // BS - backspace (data)
	{
		// move cursor back 1 position
		// VT100: don't wrap if cursor is at start of line.
		// ECMA-48: may wrap and scroll
		display->cursorLeft(1, AutoWrap(auto_wrap));
		goto show_cursor;
	}
	case 0x09: // ECMA-48: TAB (display)
	{
		handle_tab(1);
		goto show_cursor;
	}
	case 0x0b: // VT - vertical tab: VT100: same as LF
			   // ECMA-48: goto next vertical tab position (display)
	case 0x0c: // FF - formfeed: VT100: same as LF
			   // ECMA-48: goto home position of next page (display)
	case 0x0a: // LF - linefeed
	{
		// ECMA-48: cursor down (data|display)
		// ANSI:    scrolls
		// VT100:   Linefeed or new line depending on newline_mode.
		if (newline_mode) display->newLine();
		else display->cursorDown();
		goto show_cursor;
	}
	case 0x0d: // CR - carriage return
	{
		// ECMA-48: move the cursor to start of line. (display|data)
		display->cursorReturn();
		goto show_cursor;
	}
	case 0x0e: // ECMA-35:  ^N  SO/LS1
	{
		// ECMA-48: SO or LS1: locking shift 1 - use G1 character set for GL.
		// NON STANDARD: fully swap in the graphics character set
		display->addAttributes(display->GRAPHICS);
		return;
	}
	case 0x0f: // ECMA-35:  ^O  SI/LS0
	{
		// ECMA-48: SI or LS0: locking shift 0 - use G0 character set for GL.
		// NON STANDARD: fully swap out the graphics character set
		display->removeAttributes(display->GRAPHICS);
		return;
	}
	case 0x1b: // Escape
	{
		wstate = EscPending;
		wbu[0] = 0x1b;
		wcnt   = 1;
		return;
	}
	default: break;
	}

	// unhandled C0 code:
	if (log_unhandled) display->printf("{0x%02x}", c);

show_cursor:
	if (cursor_visible) display->showCursor();
}

void AnsiTerm::handle_C1(char c)
{
	// handle C1 code
	// after ESC A-Z[\]^_  or  8-bit C1-code  or  utf-8 encoded C1-code

	wstate = NothingPending;

	switch (c & 0x1f)
	{
	case 'D' & 0x1f:
	{
		// VT100: IND - index: cursor down, scrolls (data|display)
		// VT100, not in ECMA-48
		display->cursorDown();
		goto show_cursor;
	}
	case 'E' & 0x1f:
	{
		// ECMA-48: NEL - next line (data|display)
		// same as CR+LF
		// VT510: scrolls
		display->newLine();
		goto show_cursor;
	}
	case 'H' & 0x1f:
	{
		// ECMA-48: HTS - set tabulator position
		// Sets one horizontal tab stop at the column where the cursor is.
		uint col = uint(display->col);
		if (col < NELEM(htabs) * 8) set_bit_at_index(htabs, col);
		return;
	}
	case 'M' & 0x1f:
	{
		// ECMA-48: RI - Reverse index: cursor up, scroll (data|display)
		display->cursorUp();
		goto show_cursor;
	}
	case 'P' & 0x1f: // ECMA-48: DCS Device Control String. terminated by ST
	case 'X' & 0x1f: // ECMA-48: SOS Start of String. terminated by ST
	case ']' & 0x1f: // ECMA-48: OSC - Operating System Command. terminated by ST.
	case '^' & 0x1f: // ECMA-48: PM  Privacy Message. Terminated by ST
	case '_' & 0x1f: // ECMA-48: APC Application Program Command. Terminated by ST
	{
		if (log_unhandled)
		{
			wbu[wcnt++] = uchar(c);
			log_rbu();
		}
		wstate = SkipUntilST;
		wcnt   = 0;
		return;
	}
	case 'Z' & 0x1f:
	{
		// VT100:   DECID: request to identify terminal type
		//		    Obsolete form of DA: CSI c.
		// ECMA-48: SCI: single character introducer. not implemented.
		handle_sendDA();
		return;
	}
	case '[' & 0x1f:
	{
		// ECMA-48: CSI

		wbu[wcnt++] = uchar(c);
		wstate		= CsiArgsPending;
		return;
	}
	case '\\' & 0x1f:
	{
		// ECMA-48: ST: string terminator
		// either unexpected or after DCS … APC, none of which is implemented.
		break;
	}
	default: break;
	}

	// unhandled C1 code:
	if (log_unhandled)
	{
		wbu[wcnt++] = uchar(c);
		log_rbu();
	}

show_cursor:
	if (cursor_visible) display->showCursor();
}

void AnsiTerm::handle_ESC(char c)
{
	// Escape Sequence:

	// Escape sequences vary in length. The general format for an ANSI-compliant escape sequence
	// is defined by ANSI X3.41 (equivalent to ECMA-35 or ISO/IEC 2022).
	// The escape sequences consist only of bytes in the range 0x20—0x7F and can be parsed without looking ahead.
	// The behavior when a control character, a byte with the high bit set, or a byte that is not
	// part of any valid sequence, is encountered before the end is undefined.

	// If the ESC is followed by a byte in the range 0x60—0x7E {`a-z{|}~}, the escape sequence is of type Fs.
	// This type is used for control functions individually registered with the ISO-IR registry.
	// A table of these is listed under ISO/IEC 2022.
	// If the ESC is followed by a byte in the range 0x30—0x3F {0-9:;<=>?}, the escape sequence is of type Fp,
	// which is set apart for up to sixteen private-use control functions.

	// C1 equivalent ESC codes are handled by handleC1():
	assert((c & 0xe0) != 0x40);

	wstate = NothingPending;

	switch (c)
	{
	case ' ': // ESC SPC F|G
	case '#': // ESC # 3-8
	case '%': // ESC % @|G
	case '(': // ESC ( c1 c2
	case ')': // ...
	case '*':
	case '+':
	case '-':
	case '.':
	case '/':
	{
		wbu[wcnt++] = uchar(c);
		wstate		= EscArgsPending;
		return;
	}
	case '6':
	{
		// VT510: DECBI - DEC back index
		// This control function moves the cursor backward one column. If the cursor is at the left margin,
		// then all screen data within the margin moves one column to the right.
		if (display->col > 0) display->cursorLeft();
		else display->scrollScreenRight();
		goto show_cursor;
	}
	case '7':
	{
		// private DECSC - DEC save cursor
		// Saves the following:
		//	cursor position
		//	attributes
		//	selected character set
		//	state of wrap flag
		//	state of origin mode
		//	state of selective erase
		push_cursor();
		return;
	}
	case '8':
	{
		// private DECRC  DEC restore cursor
		// Restores the states described for (DECSC) above.
		// If none of these characteristics were saved, the cursor moves to home position;
		// origin mode is reset; no character attributes are assigned;
		// and the default character set mapping is established.
		pop_cursor();
		goto show_cursor;
	}
	case '9':
	{
		// VT510: DECFI  DEC forward index
		// This control function moves the cursor forward one column. If the cursor is at the right margin,
		// then all screen data within the margins moves one column to the left.
		if (display->col + 1 < display->cols) display->cursorRight();
		else display->scrollScreenLeft();
		goto show_cursor;
	}
	case '=':
	{
		// DECKPAM - keypad keys in applications mode
		application_mode = true;
		return;
	}
	case '>':
	{
		// DECKPNM - keypad keys in normal mode
		application_mode = false;
		return;
	}
	case 'c':
	{
		// ECMA-48: RIS reset to initial state
		// VT100:   "hard" reset
		reset(true);
		goto show_cursor;
	}
	default: break;
	}

	// broken or unhandled ESC code:
	if (log_unhandled)
	{
		wbu[wcnt++] = uchar(c);
		log_rbu();
	}

show_cursor:
	if (cursor_visible) display->showCursor();
}

void AnsiTerm::handle_EscArgsPending(char c)
{
	wstate = NothingPending;

	switch (wbu[1])
	{
	case ' ': // ESC SPC …
	{
		if (c == 'F') // ESC SP F
		{
			// VT220: ACS6/S7C1T: Request 7-bit control codes.
			// The terminal will in responses use 2-byte ESC[ sequence instead of 8-bit C1 codes.
			// DEC VT200 and up always accept 8-bit control sequences on receive.
			// NON STANDARD: AnsiTerm saves this setting but also updates this setting
			// when it receives a request for a response acc. to what was used in that request.
			// so in effect what is set here is almost never used.
			c1_codes_8bit = false;
			return;
		}
		if (c == 'G') // ESC SP G
		{
			// VT220: ACS7/S8C1T: Request 8-bit C1 control codes.
			// The terminal will in responses use 8-bit C1 codes instead of 2-byte ESC[ sequence.
			// NON STANDARD: AnsiTerm saves this setting but also updates this setting
			// when it receives a request for a response acc. to what was used in that request.
			// so in effect what is set here is almost never used.
			c1_codes_8bit = true;
			return;
		}
		break;
	}
	case '#': // ESC # …
	{
		switch (c)
		{
		case '8': // ESC # 8
		{
			// VT100: DECALN: video alignment test: fill screen with E's
			display->fgcolor = display->default_fgcolor;
			display->bgcolor = display->default_bgcolor;
			display->cls();
			display->printChar('E', display->cols * display->rows);
			display->moveTo(0, 0, nowrap);
			display->showCursor(cursor_visible);
			return;
		}
		default: break;
		}
		break;
	}
	case '%': // ESC % @|G
	{
		if (c == '@') // ESC % @
		{
			// ISO 2022: Select 8bit latin-1 character set
			utf8_mode = false;
			return;
		}
		if (c == 'G') // ESC % G
		{
			// ISO 2022: Select utf-8 character set
			utf8_mode = true;
			return;
		}
		break;
	}
	case '(': // ESC ( c1 c2    ISO 2022: SCS Designate G0 character set with national character set
	case ')': // ESC ) c1 c2    ISO 2022: SCS Designate G1 character set with national character set
	case '*': // ESC * c1 c2    ISO 2022: SCS Designate G2 character set with national character set
	case '+': // ESC + c1 c2    ISO 2022: SCS Designate G3 character set with national character set
	case '-': // ESC , c1 c2    VT300: SCS Designate G1 character set with national character set, 96 char
	case '.': // ESC - c1 c2    VT300: SCS Designate G2 character set with national character set, 96 char
	case '/': // ESC . c1 c2    VT300: SCS Designate G3 character set with national character set, 96 char
	{
		// c1 = missing or [0x20…0x2f], c2 = [0x30…0x5f]

		if (wcnt < 4 && c >= 0x20 && c <= 0x2f)
		{
			wbu[wcnt++] = uchar(c);
			wstate		= EscArgsPending;
			return; // need one more
		}
		break; // bogus or IGNORED
	}
	default: break;
	}

	// unknown/broken:
	if (log_unhandled)
	{
		wbu[wcnt++] = uchar(c);
		log_rbu();
	}
}

void AnsiTerm::handle_CsiArgsPending(char c)
{
	// CSI Control Sequence Introducer ESC + '[' received:

	// CSI or ESC[
	// is followed by any number (including none) of "parameter bytes" 0x30…0x3F (0–9:;<=>?),
	// then by any number of "intermediate bytes" 0x20…0x2F (space and !"#$%&'()*+,-./),
	// then finally by a single "final byte" 0x40…0x7E (ASCII @A–Z[\]^_`a–z{|}~).
	//
	// Sequences containing the parameter bytes <=>? or the final bytes 0x70–0x7E (p–z{|}~) are private.
	//
	// actually seen single ! or ? at start and single SPC, ' or " before final char.
	//
	// All common sequences just use the parameters as a series of semicolon-separated numbers such as 1;2;3.
	// Missing numbers are treated as 0, e.g. 1;;3 acts like the middle number is 0, and no parameters at all
	// as in ESC[m acts like a 0 reset code).
	// Some sequences (such as CUU) treat 0 as 1 in order to make missing parameters useful.
	//
	// The behavior of the terminal is undefined in the case of illegal CSI sequence.

	// receive parameters and final code:
	// all received bytes are stored with bit15 set, except for ';' and decimal digits:
	// decimal numbers are stored decoded (expecting a max value of 0x7fff)
	// ';' requires a decimal number before and after, if it is omitted 'novalue' is stored.
	// the final command byte is in 'c'.

	wbu[wcnt++] = uchar(c);
	assert(wstate == CsiArgsPending);

	// intermediate byte or parameter byte => need more
	if (c >= 0x20 && c <= 0x3f && wcnt < NELEM(wbu)) return;

	wstate = NothingPending;

	if (c < 0x40 || c > 0x7e) // expect a final char
	{
		if (log_unhandled) log_rbu();
		else putc(c);
		return;
	}

	constexpr uint	 maxargs = 8;
	uint16			 args[maxargs];
	uint			 argc	 = 0;
	uint			 special = 0;
	constexpr uint16 novalue = 0xffff;

	uchar* p = &wbu[*wbu == 0x1b ? 2 : 1]; // esc[ or CSI
	for (uchar c;;)
	{
		if (is_decimal_digit(c = *p++))
		{
		a:
			uint value = c - '0';
			while (is_decimal_digit(c = *p++)) { value = value * 10 + c - '0'; }
			if (argc < maxargs) args[argc++] = uint16(value);
		}
		else if (c == ';')
		{
		b:
			if (argc < maxargs) args[argc++] = novalue;
		}

		if (c == ';')
		{
			if (is_decimal_digit(c = *p++)) goto a;
			else goto b;
		}

		if (c >= 0x40) break;  // cmd byte
		if (special) c = 0xff; // no known/supported CSI sequences contain more than one of these => break it
		special = c;
	}

	assert(p == wbu + wcnt);

	// handle esc sequence based on last char in 'c':

	if (!special)
	{
		switch (c)
		{
		case '@': // CSI n @
		{
			// ECMA-48: ICH insert characters (data|display)
			// Insert n blank characters at the cursor position, with the character attributes
			// set to normal. The cursor does not move and remains at the beginning of the
			// inserted blank characters. A parameter of 0 or 1 inserts one blank character.
			// Data on the line is shifted forward as in character insertion.
			if (argc > 1) break;
			display->insertChars(argc ? args[0] : 1);
			goto show_cursor;
		}
		case 'k': // CSI n k
		case 'A': // CSI n A
		{
			// ECMA-48: VPB: vertical position backward (data)
			// ECMA-48: CUU cursor up, no scroll (display)
			if (argc > 1) break;
			int n = argc ? args[0] : 1;
			display->cursorUp(n, nowrap);
			goto show_cursor;
		}
		case 'e': // CSI n e
		case 'B': // CSI n B
		{
			// ECMA-48: VPR: vertical position forward (data)
			// ECMA-48: CUD cursor down, no scroll (display)
			if (argc > 1) break;
			int n = argc ? args[0] : 1;
			display->cursorDown(n, nowrap);
			goto show_cursor;
		}
		case 'a': // CSI b a
		case 'C': // CSI n C
		{
			// ECMA-48: 'a'  HPR horizontal position forward (right) (data)
			// ECMA-48: 'C'  CUF cursor forward (right), no wrap (display)
			if (argc > 1) break;
			int n = argc ? args[0] : 1;
			display->cursorRight(n, nowrap);
			goto show_cursor;
		}
		case 'j': // CSI n j
		case 'D': // CSI n D
		{
			// ECMA-48: 'j'  HPB: character position backward (data)
			// ECMA-48: 'D'  CUB cursor back (left), no wrap (display)
			if (argc > 1) break;
			int n = argc ? args[0] : 1;
			display->cursorLeft(n, nowrap);
			goto show_cursor;
		}
		case 'E':
		{
			// ECMA-48: CNL cursor next line
			// Move the cursor to beginning of next line, n lines down. (display)
			// no scroll.
			if (argc > 1) break;
			int n = argc ? args[0] : 1;
			display->cursorDown(n, nowrap);
			display->cursorReturn();
			goto show_cursor;
		}
		case 'F':
		{
			// ECMA-48: CPL cursor previous line
			// Move the cursor to beginning of previous line, n lines up.  (display)
			// no scroll.
			if (argc > 1) break;
			int n = argc ? args[0] : 1;
			display->cursorUp(n, nowrap);
			display->cursorReturn();
			goto show_cursor;
		}
		case '`':
		case 'G':
		{
			// ECMA-48: '`'  HPA horizontal position absolute in current line (data)
			// ECMA-48: 'G'  CHA cursor horizontal absolute in current line (display)
			if (argc > 1) break;
			int col = argc && args[0] ? args[0] : 1;
			display->moveToCol(col - 1, nowrap);
			goto show_cursor;
		}
		case 'f': // CSI r ; c f
		case 'H': // CSI r ; c H
		{
			// ECMA-48: 'f'  HVP: horizontal vertical position: set cursor position (data)
			// ECMA-48: 'H'  CUP: cursor position (display)
			// limited inside screen
			if (argc > 2) break;
			int row = argc && args[0] && args[0] != novalue ? args[0] : 1;
			int col = argc == 2 && args[1] && args[1] != novalue ? args[1] : 1;
			display->moveTo(row - 1, col - 1, nowrap);
			goto show_cursor;
		}
		case 'I': // CSI n I
		{
			// ECMA-48: CHT	Cursor Forward Tabulation n tab stops (display)
			if (argc > 1) break;
			uint n = argc ? args[0] : 1;
			handle_tab(n);
			goto show_cursor;
		}
		case 'J': // CSI n J
		{
			// ECMA-48: ED: erase in display (data|display)
			// Clears part of the screen.
			if (argc > 1) break;
			switch (argc ? args[0] : 0)
			{
			case 0:
				// ECMA-48: Erase from the cursor to the end of the screen, including the cursor position.
				// the cursor does not move.
				display->clearToEndOfScreen();
				goto show_cursor;
			case 1:
				// ECMA-48: Erase from the beginning of the screen to the cursor, including the cursor position.
				// the cursor does not move, even if col==cols. (then the cursor position is actually not erased.)
				display->clearToStartOfScreen(true);
				goto show_cursor;
			case 3:
				// xterm: clear entire screen and delete all lines saved in the scrollback buffer
				// (this feature was added for xterm and is supported by other terminal applications).
				// we have no scrollback buffer
			case 2:
				// ECMA-48: Erase the whole screen.
				// VT100: The cursor does not move.
				// ANSI.SYS: the cursor is moved to the upper left corner.
				display->clearRect(0, 0, display->rows, display->cols);
				goto show_cursor;
			default: break;
			}
			break;
		}
		case 'K': // CSI n K
		{
			// ECMAL-48: EL: erase in line (data|display)
			// Erases part of the line. The cursor does not move.
			if (argc > 1) break;
			switch (argc ? args[0] : 0)
			{
			case 0:
				// ECMAL-48: Erases from the cursor to the end of the line, including the cursor position.
				display->clearToEndOfLine();
				goto show_cursor;
			case 1:
				// ECMAL-48: Erases from the beginning of the line to the cursor, including the cursor position.
				display->clearToStartOfLine(true);
				goto show_cursor;
			case 2:
				// ECMAL-48: Erases the complete line.
				display->clearRect(display->row, 0, 1, display->cols);
				goto show_cursor;
			default: break;
			}
			break;
		}
		case 'L': // CSI n L
		{
			// ECMA-48: IL: insert lines (data|display)
			// Inserts n lines at the current row, shifting rows downwards.
			// ECMA-48: The cursor is reset to the first column.
			// VT100: This sequence is ignored when the cursor is outside the scrolling region.
			if (argc > 1) break;
			display->insertRows(argc ? args[0] : 1);
			display->cursorReturn();
			goto show_cursor;
		}
		case 'M': // CSI n M
		{
			// ECMA-48: DL: delete lines (display|data)
			// Deletes n lines starting at the line with the cursor. Lines below the cursor move up
			// and blank lines are added at the bottom of the scrolling region.
			// ECMA-48: The cursor is reset to the first column.
			// VT100: This sequence is ignored when the cursor is outside the scrolling region.
			if (argc > 1) break;
			display->deleteRows(argc ? args[0] : 1);
			display->cursorReturn();
			goto show_cursor;
		}
		case 'P': // CSI n P
		{
			// ECMA-48: DCH: delete characters (display|data)
			// VT100: Deletes the character at the cursor position and n-1 characters to the right.
			// All characters to the right of the cursor move to the left padding with space.
			if (argc > 1) break;
			display->deleteChars(argc ? args[0] : 1);
			goto show_cursor;
		}
		case 'S': // CSI n S
		{
			// ECMA-48: SU: scroll up (display)
			// scroll whole page up by n rows (default=1). No cursor move.
			if (argc > 1) break;
			display->scrollScreenUp(argc ? args[0] : 1);
			goto show_cursor;
		}
		case '^': // CSI n ^  same as CSI n T  (ECMA-48 documentation error)
		case 'T': // CSI n T
		{
			// ECMA-48: SD: scroll down (display)
			// scroll whole page down n lines (default=1). No cursor move.
			if (argc > 1) break;
			display->scrollScreenDown(argc ? args[0] : 1);
			goto show_cursor;
		}
		case 'X': // CSI n X
		{
			// ECMA-48: ECH erase characters (data|display)
			// Erases characters at the cursor position and the next n-1 characters.
			// VT100: A parameter of 0 or 1 erases a single character.
			if (argc > 1) break;
			int n = argc ? args[0] : 1;
			display->clearRect(display->row, display->col, 1, n);
			goto show_cursor;
		}
		case 'Z': // CSI n Z
		{
			// ECMA-48: CBT: cursor backward tabulation  n tabs (display)
			if (argc > 1) break;
			uint n = argc ? args[0] : 1;
			handle_backTab(n);
			goto show_cursor;
		}
		case 'b':
		{
			// REP: repeat preceding graphics char n times
			break; // TODO
		}
		case 'c': // CSI 0 c
		{
			// ECMA-48: DA: send primary device attributes

			if (argc > 1) break;
			if (argc && args[0] != 0) break;
			handle_sendDA();
			return;
		}
		case 'd': // CSI n d
		{
			// ECMA-48: VPA: vertical position absolute (data)
			// VT510: stops at last line
			if (argc > 1) break;
			display->moveToRow(argc && args[0] ? args[0] - 1 : 0, nowrap /*auto_wrap*/);
			goto show_cursor;
		}
		case 'g': // CSI n g
		{
			// ECMA-48: TBC: n=0: clear tab stop in current position
			// ECMA-48: 	 n=3: clear all tab stops
			if (argc > 1) break;
			if (argc == 0 || args[0] == 0)
			{
				// Clear one horizontal tab stop at the column where the cursor is.
				uint col = uint(display->col);
				if (col < NELEM(htabs) * 8) clear_bit_at_index(htabs, col);
				return;
			}
			if (argc == 1 && args[0] == 3)
			{
				// Clear all horizontal tab stops.
				memset(htabs, 0x00, NELEM(htabs));
				return;
			}
			break;
		}
		case 'm': // CSI n… m
		{
			// VT100: SGR select graphic rendition
			handle_SGR(argc, args);
			return;
		}
		case 'n': // CSI n n
		{
			if (argc == 1)
			{
				if (args[0] == 5)
				{
					// ECMA-48: DSR: request status report
					put_csi_response("0n"); // no malfunction
					return;
				}
				if (args[0] == 6)
				{
					// ECMA-48: CPR: cursor position report
					// -> report cursor at line l, column c: ESC[l;cR
					put_csi_response("%u;%uR", display->row + 1, display->col + 1);
					return;
				}
			}
			break;
		}
		case 'r': // CSI top ; bot r
		{
			// DECSTBM  set scroll region top and bottom margin
			// VT510: Default: Margins are at the page limits.
			if (argc > 4) break;
			if (argc > 2)
			{
				// CSI t;b;l;r r  or  CSI ;;l;r r
				int	 t	= args[0];
				int	 b	= args[1];
				bool tb = t != novalue || b != novalue;

				if (tb) top_margin = t != novalue ? uint8(t) : 0;
				if (tb) bottom_margin = b != novalue ? uint8(b) : 0;
				left_margin	   = args[2] != novalue ? uint8(args[2]) : 0;
				right_margin   = argc >= 4 && args[3] != novalue ? uint8(args[3]) : 0;
				lr_set_by_csir = true;
				bool f		   = (tb && tb_margins_enabled) | lr_margins_enabled;
				if (f) apply_margins();
			}
			else
			{
				// CSI t;b r
				top_margin	  = argc != 0 && args[0] != novalue ? uint8(args[0]) : 0;
				bottom_margin = argc >= 2 && args[1] != novalue ? uint8(args[1]) : 0;
				if (tb_margins_enabled) apply_margins();
			}
			goto show_cursor;
		}
		case 's': // CSI s
		{
			// SCOSC:   save current cursor position and attributes.
			//		    deprecated: use DECSC.
			// DECSLRM: set left + right margin.
			//		    deprecated: use CSI r instead, though unofficial.
			if (argc || lr_ever_set_by_csis)
			{
				lr_ever_set_by_csis = true;
				left_margin			= argc != 0 && args[0] != novalue ? uint8(args[0]) : 0;
				right_margin		= argc >= 2 && args[1] != novalue ? uint8(args[1]) : 0;
				lr_set_by_csir		= false;
				if (lr_margins_enabled) apply_margins();
				goto show_cursor;
			}
			else // SCOSC:
			{
				push_cursor();
				return;
			}
		}
		case 'u': // CSI u
		{
			// SCORC: restore saved cursor position and attributes
			if (argc) break;
			pop_cursor();
			goto show_cursor;
		}
		case 'h': // CSI n… h: ECMA-48: SM: set a feature ON
		case 'l': // CSI n… h: ECMA-48: RM: set a feature OFF
		{
			for (uint i = 0; i < argc; i++)
			{
				// 1  GUARDED AREA TRANSFER MODE (GATM)		not supported
				// 2  KEYBOARD ACTION MODE (KAM)			ignored (always enabled)
				// 3  CONTROL REPRESENTATION MODE (CRM)		ignored (user setting)
				// 4  INSERTION REPLACEMENT MODE (IRM)		supported
				// 5  STATUS REPORT TRANSFER MODE (SRTM)	not supported
				// 6  ERASURE MODE (ERM)					not supported
				// 7  LINE EDITING MODE (VEM)				not supported
				// 8  BI-DIRECTIONAL SUPPORT MODE (BDSM)	not supported
				// 9  DEVICE COMPONENT SELECT MODE (DCSM)	not supported
				// 10 CHARACTER EDITING MODE (HEM)			not supported
				// 11 POSITIONING UNIT MODE (PUM)			not supported
				// 12 SEND/RECEIVE MODE (SRM)				supported (TODO)
				// 13 FORMAT EFFECTOR ACTION MODE (FEAM)	not supported
				// 14 FORMAT EFFECTOR TRANSFER MODE (FETM)	not supported
				// 15 MULTIPLE AREA TRANSFER MODE (MATM)	not supported
				// 16 TRANSFER TERMINATION MODE (TTM)		not supported
				// 17 SELECTED AREA TRANSFER MODE (SATM)	not supported
				// 18 TABULATION STOP MODE (TSM)			not supported
				// 20 newline mode
				// 21 GRAPHIC RENDITION COMBINATION (GRCM)	not supported (always combining)
				// 22 ZERO DEFAULT MODE (ZDM)				ignored

				bool f = c == 'h';
				switch (args[i])
				{
				case 4: // CSI 4 h
				{
					// ECMA-48: IRM: Insert/replace mode: h=insert, l=replace
					insert_mode = f;
					continue;
				}
				case 12: // CSI 12 h
				{
					// ECMA-48: SRM: send/receive mode: h=no local echo, l=local echo
					// When send/receive mode is reset (local echo on), every character sent from
					// the keyboard automatically appears on the screen. Therefore, the host does
					// not have to send (echo) the character back to the terminal display.
					local_echo = !f; // TODO
					continue;
				}
				case 20: // CSI 20 h
				{
					// ECMA-48v4: LNM: line feed / new line mode: l=line feed, h=new line
					// removed in ECMA-48v5.
					newline_mode = f;
					continue;
				}
				case 21: // CSI 21 h
				{
					// ECMA-48: GRCM: Graphic rendition combination mode
					sgr_cumulative = f;
					continue;
				}
				default: break;
				}
				if (log_unhandled) log_rbu();
				return;
			}
			return;
		}
		default: break;
		}
	}
	else // command has a special char
	{
		switch (c)
		{
		case '@': // CSI n SPC @
		{
			// ECMA-48: SL: scroll screen left n columns (display)
			if (special != ' ' || argc > 1) break;
			display->scrollScreenLeft(argc ? args[0] : 1);
			goto show_cursor;
		}
		case 'A': // CSI n SPC A
		{
			// ECMA-48: SR: scroll screen right n columns (display)
			if (special != ' ' || argc > 1) break;
			display->scrollScreenRight(argc ? args[0] : 1);
			goto show_cursor;
		}
		case 'W': // CSI ? 5 W
		{
			// VT510: DECST8C: reset tab stops to every 8 columns
			if (special == '?' && argc == 1 && args[0] == 5)
			{
				memset(htabs, 0x01, NELEM(htabs));
				return;
			}
			break;
		}
		case 'p': // CSI ! p
		{
			if (special == '!' && argc == 0)
			{
				// DECSTR: soft terminal reset
				// set terminal to power-up default state
				reset(false);
				goto show_cursor;
			}
			break;
		}
		case 'q': // CSI n SPC q
		{
			if (special == ' ' && argc == 1 && args[0] <= 6)
			{
				// DECSCUSR: select cursor shape		VT520
				// n = 0  ⇒  blinking block.
				// n = 1  ⇒  blinking block (default).
				// n = 2  ⇒  steady block.
				// n = 3  ⇒  blinking underline.
				// n = 4  ⇒  steady underline.
				// n = 5  ⇒  blinking bar, xterm.
				// n = 6  ⇒  steady bar, xterm.
				break; // IGNORE
			}
			break; // NOT SUPPORTED.
		}
		case 'w': // CSI t;l;b;r ' w
		{
			if (special == '\'' && argc <= 4)
			{
				// VT420, xterm: DECEFR: Enable Filter Rectangle.
				// Defines the coordinates of a filter rectangle and activates it.
				// Anytime the locator is detected outside of the filter rectangle,
				// an outside rectangle event is generated and the rectangle is disabled.
				// Filter rectangles are always treated as "one-shot" events.
				// Any parameters that are omitted default to the current locator position.
				// If all parameters are omitted, any locator motion will be reported.
				// DECELR always cancels any previous rectangle definition.

				Point m				= USB::getMousePosition();
				int	  x				= (mouse_report_pixels ? m.x : m.x / display->CHAR_WIDTH);
				int	  y				= (mouse_report_pixels ? m.y : m.y / display->CHAR_HEIGHT);
				mouse_rect.top()	= argc < 1 || args[0] == novalue ? y : args[0] - 1;
				mouse_rect.left()	= argc < 2 || args[1] == novalue ? x : args[1] - 1;
				mouse_rect.bottom() = (argc < 3 || args[2] == novalue ? y + 1 : args[2]);
				mouse_rect.right()	= (argc < 4 || args[3] == novalue ? x + 1 : args[3]);
				mouse_rect.normalize();
				mouse_enable_rect = true;
				return;
			}
			break;
		}
		case 'z': // xterm: CSI n ; m ' z
		{
			if (special == '\'' && argc <= 2)
			{
				// Enable Locator Reporting (DECELR).
				//	n = 0  ⇒  Mouse reports disabled (default).
				//	  = 1  ⇒  Mouse reports enabled.
				//    = 2  ⇒  Mouse reports enabled for one report, then disabled.
				//	m = 0  ⇒  character cells (default).
				//    = 1  ⇒  device physical pixels.
				//    = 2  ⇒  character cells.
				// DECELR cancels any previous rectangle definition.

				if (argc < 1 || args[0] == novalue) args[0] = 0;
				if (argc < 2 || args[1] == novalue) args[1] = 0;
				if (args[0] > 2 || args[1] > 2) break;

				while (!mouse_enabled && USB::mouseEventAvailable()) USB::getMouseEvent(); // flush old reports

				mouse_enabled		= args[0] != 0;
				mouse_enabled_once	= args[0] == 2;
				mouse_report_pixels = args[1] == 1;
				mouse_enable_rect	= false;
				return;
			}
			break;
		}
		case '{': // CSI n… ' {
		{
			if (special == '\'' && argc <= 3)
			{
				// Select Locator Events (DECSLE).
				// multiple arguments possible.
				// n = 0  ⇒  only respond to explicit host requests (DECRQLP).
				//			  This is default. It also cancels any filter rectangle.
				//     1  ⇒  report button down transitions.
				//     2  ⇒  do not report button down transitions.
				//     3  ⇒  report button up transitions.
				//     4  ⇒  do not report button up transitions.

				if (argc == 0) args[argc++] = 0;

				for (uint i = 0; i < argc; i++)
				{
					switch (auto v = args[i])
					{
					case novalue:
					case 0:
						mouse_enable_rect	  = false;
						mouse_report_btn_down = false;
						mouse_report_btn_up	  = false;
						break;
					case 1:
					case 2: mouse_report_btn_down = v == 1; break;
					case 3:
					case 4: mouse_report_btn_up = v == 3; break;
					default:
						if (log_unhandled) log_rbu();
						return;
					}
				}
				return;
			}
			break;
		}
		case '|': // CSI n ' |
		{
			if (special == '\'' && argc <= 1 && mouse_enabled)
			{
				// Request Locator Position (DECRQLP).
				// n = 0, 1 or omitted:  transmit a single DECLRP locator report.
				//
				// If Locator Reporting has been enabled by a DECELR, xterm will respond
				// with a DECLRP Locator Report. This report is also generated on button up
				// and down events if they have been enabled with a DECSLE, or when the locator
				// is detected outside of a filter rectangle, if filter rectangles have been
				// enabled with a DECEFR.
				//
				// --> Mouse Report:  CSI event ; buttons ; row ; col ; page & w
				// 	   event = 0: locator unavailable.
				// 	   event = 1: response to a DECRQLP request.

				if (argc && args[0] > 1) break;
				mouse_enabled = !mouse_enabled_once;					  // switch off if only once
				if (!USB::mousePresent()) return put_csi_response("0&w"); // no pointer device present
				USB::MouseEvent e = USB::getMouseEvent();				  // new or most recent event
				if (!mouse_report_pixels)
				{
					e.x /= display->CHAR_WIDTH;
					e.y /= display->CHAR_HEIGHT;
				}
				uint b = buttons_for_buttons(e.buttons);

				put_csi_response("1;%u;%u;%u&w", b, e.y + 1, e.x + 1);
				return;
			}
			break;
		}
		case '}': // CSI n ' }
		{
			if (special == '\'' && argc <= 1)
			{
				// VT420: DECIC: DEC Insert Columns
				display->insertColumns(argc ? args[0] : 1);
				goto show_cursor;
			}
			break;
		}
		case '~': // CSI n ' ~
		{
			if (special == '\'' && argc <= 1)
			{
				// VT420: DECDC: DEC Delete Columns
				display->deleteColumns(argc ? args[0] : 1);
				goto show_cursor;
			}
			break;
		}
		case 'h': // private CSI ? n… h
		case 'l': // private CSI ? n… l
		{
			if (special != '?') break;
			bool f = c == 'h';

			for (uint i = 0; i < argc; i++)
			{
				switch (args[i])
				{
				case 1: // CSI ? 1 h
				{
					// VT100: DECCKM: cursor keys send application control functions
					// VT100: DECCKM: cursor keys send ANSI cursor control sequences
					application_mode = f;
					continue;
				}
				case 5: // CSI ? 5 h
				{
					// VT100: DECSCNM: black characters on white screen mode
					// VT100: DECSCNM: white characters on black screen mode
					display->fgcolor = f ? black : white;
					display->bgcolor = f ? white : black;
					continue;
				}
				case 6: // CSI ? 6 h
				{
					// VT100: DECOM: turn on scroll region: origin mode  (scroll region set with CSI r)
					// VT100: DECOM: turn off scroll region: full screen mode.
					// 'h'	Selects the home position with line numbers starting at top margin of the
					//		user-defined scrolling region.
					//		The cursor cannot move out of the scrolling region.
					// 'l'	Selects the home position in the upper-left corner of screen.
					//		Line numbers are independent of the scrolling region.
					//
					// inofficial: if lr margins were also set with CSI r, then also enable lr margins.

					if (f != tb_margins_enabled || (lr_set_by_csir && f != lr_margins_enabled))
					{
						tb_margins_enabled = f;
						if (lr_set_by_csir) lr_margins_enabled = f;
						apply_margins();
					}
					continue;
				}
				case 7: // CSI ? 7 h
				{
					// VT100: DECAWM: auto wrap to new line
					// VT100: DECAWM: auto wrap off
					auto_wrap = f;
					if (!f) display->limitCursorPosition();
					continue;
				}
				case 25: // CSI ? 25 h
				{
					// VT220: DECTCEM: text cursor enable = show/hide cursor
					cursor_visible = f;
					if (!f) display->hideCursor();
					continue;
				}
				case 69: // CSI ? 69 h
				{
					// VT420: DECLRMM: enable l+r margin

					if (f != lr_margins_enabled)
					{
						lr_margins_enabled = f;
						apply_margins();
					}
					continue;
				}
				default: break;
				}
				if (log_unhandled) log_rbu();
				return;
			}
			goto show_cursor;
		}
		default: break;
		}
	}

	// unknown or broken!
	if (log_unhandled) log_rbu();

show_cursor:
	if (cursor_visible) display->showCursor();
}

void AnsiTerm::handle_SGR(uint argc, uint16 args[])
{
	// The control sequence CSI n m, named Select Graphic Rendition (SGR), sets display attributes.
	// Several attributes can be set in the same sequence, separated by semicolons.
	// Each display attribute remains in effect until a following occurrence of SGR resets it.
	// If no codes are given, CSI m is treated as CSI 0 m (reset / normal).

	if (argc == 0) args[argc++] = 0;
	bool					error	= false;
	static constexpr uint16 novalue = 0xffff;

	if (!sgr_cumulative)
	{
		display->setAttributes(0);
		// colors probably shouldn't be reset
	}

	for (uint i = 0; i < argc; i++)
	{
		switch (args[i])
		{
		case novalue:
		case 0:
			// VT100: Reset or normal
			// All attributes become turned off
			display->setAttributes(display->NORMAL);
			display->bgcolor = display->default_bgcolor;
			display->fgcolor = display->default_fgcolor;
			continue; // continue with next SGR attribute

		case 1:
			// Bold, increased intensity
			//As with faint, the color change is a PC (SCO / CGA) invention.
			display->addAttributes(display->BOLD);
			continue;

		case 2:
			// Faint, decreased intensity
			// May be implemented as a light font weight.
			// NOT SUPPORTED
			error = 1; // log it
			continue;

		case 3:
			// Italic
			display->addAttributes(display->ITALIC);
			continue;

		case 4:
			// Underline
			display->addAttributes(display->UNDERLINE);
			continue;

		case 5:
			// Slow blink
			// Sets blinking to less than 150 times per minute
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 6:
			// Rapid blink
			// 150+ per minute; not widely supported
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 7:
			// Reverse video
			// Swap foreground and background colors
			display->addAttributes(display->INVERTED);
			continue;

		case 8:
			// Concealed or invisible
			// Not widely supported.
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 9:
			// Crossed-out, or strike
			// Characters legible but marked as if for deletion
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 10:
			// select Primary (default) font
			continue;

		case 11:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
			// select Alternative font 1 … 9
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 20:
			// Gothic (Fraktur)
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 21:
			// Double underline
			// Double-underline per ECMA-48, but instead disables bold intensity on several terminals,
			// including in the Linux kernel's console before version 4.17.

			// double underline not supported.
			// using single underline instead.
			display->addAttributes(display->UNDERLINE);
			continue;

		case 22:
			// Normal intensity
			// Bold and Faint OFF
			display->removeAttributes(display->BOLD);
			continue;

		case 23:
			// italic and gothic OFF
			display->removeAttributes(display->ITALIC);
			continue;

		case 24:
			// single and double underline OFF
			display->removeAttributes(display->UNDERLINE);
			continue;

		case 25:
			// blinking OFF
			continue;

		case 26:
			// Proportional spacing
			// not known to be used on terminals
			// CCITT proposal
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 27:
			// reversed OFF
			display->removeAttributes(display->INVERTED);
			continue;

		case 28:
			// concealed OFF
			continue;

		case 29:
			// crossed out OFF
			continue;

		case 30:
		case 31:
		case 32:
		case 33:
		case 34:
		case 35:
		case 36:
		case 37:
			// Set foreground color
			if constexpr (!Color::is_monochrome) display->fgcolor = vga4_colors[args[i] - 30];
			continue;

		case 38:
			// Set foreground color: 38;5;n or 38;2;r;g;b
			// CCITT recommendation
			if (argc - i >= 3 && args[i + 1] == 5)
			{
				if constexpr (!Color::is_monochrome) display->fgcolor = vga8_colors[args[i + 2] & 0xff];
				i += 2; // skip arguments
				continue;
			}
			if (argc - i >= 5 && args[i + 1] == 2)
			{
				if constexpr (!Color::is_monochrome)
					display->fgcolor = Color::fromRGB8(uint8(args[i + 2]), uint8(args[i + 3]), uint8(args[i + 4]));
				i += 4; // skip arguments
				continue;
			}
			break; // abort

		case 39:
			// Default foreground color, Implementation defined (according to standard)
			display->fgcolor = display->default_fgcolor;									 // black
			if constexpr (Color::is_monochrome) display->bgcolor = display->default_bgcolor; // white
			continue;

		case 40:
		case 41:
		case 42:
		case 43:
		case 44:
		case 45:
		case 46:
		case 47:
			// Set background color
			if constexpr (!Color::is_monochrome) display->bgcolor = vga4_colors[args[i] - 40];
			continue;

		case 48:
			// Set background color: 48;5;n or 48;2;r;g;b
			// CCITT recommendation
			if (argc - i >= 3 && args[i + 1] == 5)
			{
				if constexpr (!Color::is_monochrome) display->bgcolor = vga8_colors[args[i + 2] & 0xff];
				i += 2; // skip arguments
				continue;
			}
			if (argc - i >= 5 && args[i + 1] == 2)
			{
				if constexpr (!Color::is_monochrome)
					display->bgcolor = Color::fromRGB8(uint8(args[i + 2]), uint8(args[i + 3]), uint8(args[i + 4]));
				i += 4; // skip arguments
				continue;
			}
			break; // abort

		case 49:
			// Default background color, Implementation defined (according to standard)
			display->bgcolor = display->default_bgcolor;									 // white
			if constexpr (Color::is_monochrome) display->fgcolor = display->default_fgcolor; // black
			continue;

		case 50:
			// proportional spacing OFF
			// CCITT proposal
			continue;

		case 51:
			// Framed
			// Implemented as "emoji variation selector" in mintty.
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 52:
			// Encircled
			// Implemented as "emoji variation selector" in mintty.
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 53:
			// Overlined
			// NOT SUPPORTED
			error = 1; // print SGR sequence
			continue;

		case 54:
			// framed and encircled OFF
			continue;

		case 55:
			// overlined OFF
			continue;

		case 58:
			// Set underline color: 58;5;n or 58;2;r;g;b.
			// No ECMA standard
			// NOT SUPPORTED
			error = 1;
			if (argc - i >= 3 && args[i + 1] == 5)
			{
				i += 2; // skip arguments
				continue;
			}
			if (argc - i >= 5 && args[i + 1] == 2)
			{
				i += 4; // skip arguments
				continue;
			}
			break; // abort

		case 59:
			// Default underline color
			// No ECMA standard
			continue;

		case 60: // Ideogram underline or right side line						rarely supported
		case 61: // Ideogram double underline, or double line on the right side	""
		case 62: // Ideogram overline or left side line							""
		case 63: // Ideogram double overline, or double line on the left side	""
		case 64: // Ideogram stress marking										""
			// NOT SUPPORTED
			error = 1;
			continue;

		case 65:
			// Ideogram OFF. Reset the effects of all of 60–64
			continue;

		case 66:
		{
			// Double-Height letters
			// INOFFICIAL EXTENSION, NON STANDARD
			display->setAttributes(display->DOUBLE_HEIGHT, display->DOUBLE_WIDTH);
			continue;
		}
		case 67:
		{
			// Double-width letters
			// INOFFICIAL EXTENSION, NON STANDARD
			display->setAttributes(display->DOUBLE_WIDTH, display->DOUBLE_HEIGHT);
			continue;
		}
		case 68:
		{
			// Double height and width letters
			// INOFFICIAL EXTENSION, NON STANDARD
			display->addAttributes(display->DOUBLE_HEIGHT | display->DOUBLE_WIDTH);
			continue;
		}
		case 69:
		{
			// Double-Height and double-width OFF
			// INOFFICIAL EXTENSION, NON STANDARD
			display->removeAttributes(display->DOUBLE_WIDTH | display->DOUBLE_HEIGHT);
			continue;
		}
		case 70:
		{
			// print with transparent background
			// INOFFICIAL EXTENSION, NON STANDARD
			display->addAttributes(display->TRANSPARENT);
			continue;
		}
		case 71:
		{
			// transparent background OFF
			// INOFFICIAL EXTENSION, NON STANDARD
			display->removeAttributes(display->TRANSPARENT);
			continue;
		}

		case 73:
			// Superscript.
			// No ECMA standard
			// Implemented only in mintty
			// NOT SUPPORTED
			error = 1;
			continue;

		case 74:
			// Subscript
			// No ECMA standard
			// NOT SUPPORTED
			error = 1;
			continue;

		case 75:
			// Superscript and subscript OFF
			// No ECMA standard
			continue;

		case 90:
		case 91:
		case 92:
		case 93:
		case 94:
		case 95:
		case 96:
		case 97:
			// Set bright foreground color
			// No ECMA standard
			if constexpr (!Color::is_monochrome) display->fgcolor = vga4_colors[args[i] - 90 + 8];
			continue;

		case 100:
		case 101:
		case 102:
		case 103:
		case 104:
		case 105:
		case 106:
		case 107:
			// Set bright background color
			// No ECMA standard
			if constexpr (!Color::is_monochrome) display->bgcolor = vga4_colors[args[i] - 100 + 8];
			continue;
		default: break;
		}

		// there was a fatal error in the color parameters or an unknown enumeration:
		error = 1;
		break;
	}

	if (error && log_unhandled) log_rbu();
}

inline void AnsiTerm::print_char(char c)
{
	if (insert_mode) display->insertChars(display->dx);
	display->printChar(c);
	if (cursor_visible) display->showCursor();
	else if (!auto_wrap) display->limitCursorPosition();
}

void AnsiTerm::handle_Utf8ArgsPending(char c)
{
	wbu[wcnt++] = uchar(c);
	wstate		= NothingPending;

	if unlikely (!is_fup(c)) // epcected fup
	{
		if (log_unhandled) log_rbu(); // log it
		else putc('_');				  // print a replacement for the broken utf-8 char
		putc(c);					  // handle the new char
		return;
	}

	if unlikely (wcnt < sizeof_utf8(char(wbu[0]))) // need more fups
	{
		wstate = Utf8ArgsPending;
		return;
	}

	uint wc = decodeUtf8(wbu);

	if unlikely (wc <= 0x7f) // illegal overlong encoding
	{
		if (log_unhandled) log_rbu(); // log it
		else putc('_');				  // print a replacement char
	}
	else if (wc <= 0x9f) // utf8-encoded C1 control char
	{
		wcnt = 0;
		handle_C1(char(wc));
	}
	else // printable unicode wide char
	{
		print_char(import_char(wc));
	}
}

void AnsiTerm::putc(char c)
{
	switch (wstate)
	{
	case NothingPending:
	{
		wcnt = 0;
		if (uchar(c) <= 31) // C0 control code
		{
			handle_C0(c);
			return;
		}
		else if (uchar(c) <= 0x7f) // printable ascii, including 0x7f
		{
			print_char(c);
			return;
		}
		else if (utf8_mode)
		{
			wbu[wcnt++] = uchar(c);
			if unlikely (is_fup(c)) break; // bogus: unexpected utf fup
			wstate = Utf8ArgsPending;
			return;
		}
		else if (uchar(c) <= 0x9f) // 8-bit C1 control code
		{
			handle_C1(c);
			return;
		}
		else // printable 8-bit latin-1 char
		{
			print_char(c);
			return;
		}
	}
	case Utf8ArgsPending:
	{
		handle_Utf8ArgsPending(c);
		return;
	}
	case EscPending:
	{
		if ((c & 0xe0) == 0x40) handle_C1(c); // @A…Z[\]^_
		else handle_ESC(c);
		return;
	}
	case EscArgsPending:
	{
		handle_EscArgsPending(c);
		return;
	}
	case CsiArgsPending:
	{
		handle_CsiArgsPending(c);
		return;
	}
	case SkipUntilST:
	{
		// expect only printable chars and 0x08…0x0F
		// finally expect ST

		bool printable = utf8_mode ? uchar(c) >= 0x20 : (c & 0x7f) >= 0x20;
		if (printable || (c >= 0x08 && c <= 0x0f))
		{
			// part of the message:
			if (log_unhandled) display->printChar(c);
		}
		else
		{
			// unexpected control code or ST:
			// since we handle no codes which use ST
			// we can simply finish here and handle char c as normal.
			// this will either handle the control code if ST was missing,
			// or if it is ST, it will be ignored or logged if debug = true.
			wstate = NothingPending;
			putc(c);
		}
		return;
	}
	}

	// bogus/unexpected byte:
	wstate = NothingPending;
	log_rbu();
}

uint AnsiTerm::write(const char* text, uint size)
{
	for (uint i = 0; i < size; i++) { putc(text[i]); }
	return size;
}

uint AnsiTerm::puts(cstr s)
{
	if unlikely (!s) return 0;
	cptr s0 = s;
	while (*s) { putc(*s++); }
	return uint(s - s0);
}

uint AnsiTerm::printf(cstr fmt, va_list va)
{
	char bu[200];

	va_list va2;
	va_copy(va2, va);
	int n = vsnprintf(bu, sizeof(bu), fmt, va2);
	va_end(va2);

	assert(n >= 0);
	uint size = uint(n);
	if (size <= sizeof(bu)) return write(bu, size);

	std::unique_ptr<char[]> bp {new (std::nothrow) char[size + 1]};
	if (!bp) return write(bu, sizeof(bu)); // desperately short of memory

	vsnprintf(bp.get(), size + 1, fmt, va);
	return write(bp.get(), size);
}

uint AnsiTerm::printf(cstr fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	uint size = printf(fmt, va);
	va_end(va);
	return size;
}

int AnsiTerm::getc()
{
	// get a key from the USB keyboard or a mouse event from the mouse
	// and return it in "ANSI style".
	// also returns CSI responses.
	// also tests for CTL-ALT-DEL and sets flag.

	// note:
	// ECMA-48 defines CSI n… SPC W "FNK - Function key" which seems to be intended to report
	// function key presses but no enumeration found (yet) and not supported by others anyway.

	if (inputbuffer.avail()) return uchar(inputbuffer.get());

	using namespace USB;

	if (!keyEventAvailable()) pollUSB();
	while (keyEventAvailable())
	{
		KeyEvent e = getKeyEvent();
		if (!e.down) continue;

		if unlikely (local_echo)
		{
			// in case of local echo the editing functions can only be minimalistic.

			char c = e.getchar();
			if ((c & 0x7f) < 0x20)
			{
				if (c == 13) display->newLine();
				else continue;
			}
			else if (c != 0x7f)
			{
				display->printChar(c); //
			}
			else
			{
				display->cursorLeft();
				display->printChar(' ');
				display->cursorLeft();
			}
			return uchar(c); // printable char, backspace or return
		}

		if (e.hidkey <= 0x38 || e.hidkey >= (application_mode ? 0x68 : 0x54))
		{
			// normal key or
			// keypad key in normal mode:

			char c = e.getchar();
			if (uchar(c) < 0x20)
			{
				if unlikely (c == 0) continue;
				if unlikely (c == 0x1b) inputbuffer.put(0x1b);						   // esc => send esc esc
				if unlikely (c == 13 && newline_mode) { inputbuffer.put(13), c = 10; } // VT100
				return uchar(c);
			}
			else if (uchar(c) < 0x80 || !utf8_mode)
			{
				return uchar(c); //
			}
			else // 8-bit char in utf-8 mode:
			{
				char bu[6];
				uint wc = export_char(c);
				uint n	= encodeUtf8(wc, bu);
				inputbuffer.write(bu + 1, n - 1);
				return uchar(bu[0]);
			}
		}

		// special key or keypad key in application mode:

		// Table telling what ANSI, DEC or xterm might send:
		// 0:    dead key
		// 1-24: send with CSI or ESC[ in all modes as decimal number
		// A-D:  send with CSI or ESC[ or SS3 or ESCO depending on mode
		// E-z:	 send with CSI or ESC[ in all modes. (note: keypad in normal mode is already handled)
		static constexpr char cmds[0x68 - 0x39] = {
			0,	 // 0x39  KEY_CAPS_LOCK
			'P', // 0x3A  KEY_F1
			'Q', // 0x3B  KEY_F2
			'R', // 0x3C  KEY_F3
			'S', // 0x3D  KEY_F4
			15,	 // 0x3E  KEY_F5
			17,	 // 0x3F  KEY_F6
			18,	 // 0x40  KEY_F7
			19,	 // 0x41  KEY_F8
			20,	 // 0x42  KEY_F9
			21,	 // 0x43  KEY_F10
			23,	 // 0x44  KEY_F11
			24,	 // 0x45  KEY_F12
			0,	 // 0x46  KEY_PRINT_SCREEN
			0,	 // 0x47  KEY_SCROLL_LOCK
			0,	 // 0x48  KEY_PAUSE
			2,	 // 0x49  KEY_INSERT
			1,	 // 0x4A  KEY_HOME
			5,	 // 0x4B  KEY_PAGE_UP
			3,	 // 0x4C  KEY_DELETE
			4,	 // 0x4D  KEY_END
			6,	 // 0x4E  KEY_PAGE_DOWN
			'C', // 0x4F  KEY_ARROW_RIGHT
			'D', // 0x50  KEY_ARROW_LEFT
			'B', // 0x51  KEY_ARROW_DOWN
			'A', // 0x52  KEY_ARROW_UP
			0,	 // 0x53  KEY_NUM_LOCK
			'o', // 0x54  KEY_KEYPAD_DIVIDE
			'j', // 0x55  KEY_KEYPAD_MULTIPLY
			'm', // 0x56  KEY_KEYPAD_SUBTRACT
			'k', // 0x57  KEY_KEYPAD_ADD
			'M', // 0x58  KEY_KEYPAD_ENTER
			'q', // 0x59  KEY_KEYPAD_1
			'r', // 0x5A  KEY_KEYPAD_2
			's', // 0x5B  KEY_KEYPAD_3
			't', // 0x5C  KEY_KEYPAD_4
			'u', // 0x5D  KEY_KEYPAD_5
			'v', // 0x5E  KEY_KEYPAD_6
			'w', // 0x5F  KEY_KEYPAD_7
			'x', // 0x60  KEY_KEYPAD_8
			'y', // 0x61  KEY_KEYPAD_9
			'p', // 0x62  KEY_KEYPAD_0
			'n', // 0x63  KEY_KEYPAD_DECIMAL
			0,	 // 0x64  KEY_EUROPE_2			TODO
			0,	 // 0x65  KEY_APPLICATION
			0,	 // 0x66  KEY_POWER
			'X', // 0x67  KEY_KEYPAD_EQUAL
		};

		char cmd = cmds[e.hidkey - 0x39];
		if (cmd == 0) continue; // dead special key

		char c = '\x1b';
		if (cmd <= (application_mode ? '@' : 'D')) // numbers and A-D in normal mode
		{
			if (!c1_codes_8bit) inputbuffer.put('['); // 7-bit CSI
			else if (!utf8_mode) c = 0x9b;			  // 8-bit CSI
			else { c = 0xc2, inputbuffer.put(0x9b); } // 8-bit CSI utf-8 encoded
		}
		else
		{
			if (!c1_codes_8bit) inputbuffer.put('O'); // 7-bit SS3
			else if (!utf8_mode) c = 0x8f;			  // 8-bit SS3
			else { c = 0xc2, inputbuffer.put(0x8f); } // 8-bit SS3 utf-8 encoded
		}

		if (cmd <= '@') // number
		{
			if (cmd >= 10) inputbuffer.put('0' + cmd / 10);
			inputbuffer.put('0' + cmd % 10);
		}

		if (e.modifiers)
		{
			uint m = 1;
			if (e.modifiers & (LEFTSHIFT | RIGHTSHIFT)) m += 1;
			if (e.modifiers & (LEFTALT)) m += 2;
			if (e.modifiers & (LEFTCTRL | RIGHTCTRL)) m += 4;
			if (e.modifiers & (RIGHTALT)) m += 8;

			if (cmd <= '@') inputbuffer.put(';'); // number
			if (m >= 10) inputbuffer.put('1');
			inputbuffer.put('0' + m % 10u);
		}

		inputbuffer.put(cmd <= '@' ? '~' : cmd);
		return uchar(c);
	}

	while (mouse_enabled && mouseEventAvailable())
	{
		// Mouse Report:
		// 	CSI event ; buttons ; row ; col ; page & w

		// 	event:
		// 	  0   locator unavailable - no other parameters sent.
		// 	  1   request - the terminal responds to a DECRQLP request.
		// 	  2   left button down.
		// 	  3   left button up.
		// 	  4   middle button down.
		// 	  5   middle button up.
		// 	  6   right button down.
		// 	  7   right button up.
		// 	  8   M4 button down.
		// 	  9   M4 button up.
		// 	  10  mouse left filter rectangle.
		// 	buttons:
		// 	  0   no buttons down.
		// 	  1   right button down.
		// 	  2   middle button down.
		// 	  4   left button down.
		// 	  8   M4 button down.

		// mouse position and filter rect coordinates use the full screen size, not a possibly set scroll region.

		MouseEvent e = getMouseEvent();
		int		   x = (mouse_report_pixels ? e.x : e.x / display->CHAR_WIDTH);
		int		   y = (mouse_report_pixels ? e.y : e.y / display->CHAR_HEIGHT);

		uint new_buttons = buttons_for_buttons(e.buttons);
		uint toggled	 = buttons_for_buttons(e.toggled);
		uint old_buttons = new_buttons ^ toggled;
		uint down		 = toggled & new_buttons;
		uint up			 = toggled & old_buttons;

		if (up && mouse_report_btn_up)
		{
			if (up & 1) put_csi_response("7;%u;%i;%i&w", old_buttons &= ~1u, y + 1, x + 1);
			if (up & 2) put_csi_response("5;%u;%i;%i&w", old_buttons &= ~2u, y + 1, x + 1);
			if (up & 4) put_csi_response("3;%u;%i;%i&w", old_buttons &= ~4u, y + 1, x + 1);
			if (up & 8) put_csi_response("9;%u;%i;%i&w", old_buttons &= ~8u, y + 1, x + 1);
			mouse_enabled = !mouse_enabled_once;
		}
		else old_buttons &= ~up;

		if (mouse_enabled && mouse_enable_rect && !mouse_rect.contains(Point(x, y)))
		{
			put_csi_response("10;%u;%u;%u&w", old_buttons, y + 1, x + 1);
			mouse_enable_rect = false;
			mouse_enabled	  = !mouse_enabled_once;
		}

		if (mouse_enabled && down && mouse_report_btn_down)
		{
			if (down & 1) put_csi_response("6;%u;%i;%i&w", old_buttons |= 1, y + 1, x + 1);
			if (down & 2) put_csi_response("4;%u;%i;%i&w", old_buttons |= 2, y + 1, x + 1);
			if (down & 4) put_csi_response("2;%u;%i;%i&w", old_buttons |= 4, y + 1, x + 1);
			if (down & 8) put_csi_response("8;%u;%i;%i&w", old_buttons |= 8, y + 1, x + 1);
			mouse_enabled = !mouse_enabled_once;
		}

		if (inputbuffer.avail()) return uchar(inputbuffer.get());
	}

	return nochar;
}

uint AnsiTerm::read(char* text, uint size)
{
	for (uint i = 0; i < size; i++)
	{
		int c = getc();
		if (c == -1) return i;
		text[i] = char(c);
	}
	return size;
}

} // namespace kio::Graphics


/*



































*/
