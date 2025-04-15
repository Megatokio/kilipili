// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "AnsiTerm.h"
#include "Graphics/Color.h"
#include "Graphics/ColorMap.h"
#include "MockPixmap.h"
#include "Pixmap_wAttr.h"
#include "USBHost/HidKeyTables.h"
#include "USBHost/USBKeyboard.h"
#include "USBHost/USBMouse.h"
#include "doctest.h"
#include "mock_hid_handler.h"

namespace kio::Test
{

using namespace kio::Graphics;
using namespace USB;
using namespace kio;
using TextVDU = kio::Graphics::Mock::TextVDU;
using Pixmap  = kio::Graphics::Mock::Pixmap;

static_assert(TextVDU::CHAR_HEIGHT == 12);
static_assert(TextVDU::CHAR_WIDTH == 8);
static_assert(Color::total_colorbits >= 15); // we get the defaults from vgaboard

// defined in main_unit_test.cpp:

namespace kio
{
extern doctest::String toString(Array<cstr>);
}
namespace kio::Graphics
{
extern doctest::String toString(ColorMode);
}

// construct a USB KeyboardReport:
static HidKeyboardReport keys(Modifiers m = NO_MODIFIERS, HIDKey a = NO_KEY, HIDKey b = NO_KEY, HIDKey c = NO_KEY)
{
	return HidKeyboardReport {m, 0, {a, b, c, NO_KEY, NO_KEY, NO_KEY}};
}

// construct a USB mouse report:
static HidMouseReport mouse(uint8 b = 0, int8 dx = 0, int8 dy = 0, int8 w = 0, int8 p = 0)
{
	return HidMouseReport {b, dx, dy, w, p};
}


TEST_CASE("AnsiTerm: constructor")
{
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";
	REQUIRE_EQ(at.display->log, ref);

	CHECK_NE(at.import_char, nullptr);
	CHECK_NE(at.export_char, nullptr);
	CHECK_EQ(at.full_pixmap, pm);
	CHECK_NE(at.display, nullptr);
	CHECK_EQ(at.import_char('A'), 'A');
	CHECK_EQ(at.import_char(220u), char(220));
	CHECK_EQ(at.export_char('A'), 'A');
	CHECK_EQ(at.export_char(char(220)), 220u);

	CHECK_EQ(at.default_auto_wrap, ANSITERM_DEFAULT_AUTO_WRAP);
	CHECK_EQ(at.default_application_mode, ANSITERM_DEFAULT_APPLICATION_MODE);
	CHECK_EQ(at.default_utf8_mode, ANSITERM_DEFAULT_UTF8_MODE);
	CHECK_EQ(at.default_c1_codes_8bit, ANSITERM_DEFAULT_C1_CODES_8BIT);
	CHECK_EQ(at.default_newline_mode, ANSITERM_DEFAULT_NEWLINE_MODE);
	CHECK_EQ(at.default_local_echo, ANSITERM_DEFAULT_LOCAL_ECHO);
	CHECK_EQ(at.sgr_cumulative, ANSITERM_DEFAULT_SGR_CUMULATIVE);
	CHECK_EQ(at.log_unhandled, ANSITERM_DEFAULT_LOG_UNHANDLED);

	CHECK_EQ(at.auto_wrap, at.default_auto_wrap);
	CHECK_EQ(at.application_mode, at.default_application_mode);
	CHECK_EQ(at.utf8_mode, at.default_utf8_mode);
	CHECK_EQ(at.c1_codes_8bit, at.default_c1_codes_8bit);
	CHECK_EQ(at.newline_mode, at.default_newline_mode);
	CHECK_EQ(at.local_echo, at.default_local_echo);
	CHECK_EQ(at.sgr_cumulative, at.default_sgr_cumulative);
	CHECK_EQ(at.lr_ever_set_by_csis, false);
	CHECK_EQ(at.mouse_enabled, false);
	CHECK_EQ(at.mouse_enabled_once, false);
	CHECK_EQ(at.mouse_report_pixels, false);
	CHECK_EQ(at.mouse_report_btn_down, false);
	CHECK_EQ(at.mouse_report_btn_up, false);
	CHECK_EQ(at.mouse_enable_rect, false);
	//CHECK_EQ(at.mouse_rect, Rect(0, 0, 0, 0));

	uchar htabs[sizeof(at.htabs)];
	memset(htabs, 0x01, sizeof(htabs));
	CHECK_EQ(memcmp(at.htabs, htabs, sizeof(htabs)), 0);

	CHECK_EQ(at.auto_wrap, at.default_auto_wrap);
	CHECK_EQ(at.insert_mode, false);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(at.top_margin, 0);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.left_margin, 0);
	CHECK_EQ(at.right_margin, 0);
}

TEST_CASE("AnsiTerm: reset(soft)")
{
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";
	REQUIRE_EQ(at.display->log, ref);

	at.insert_mode				   = !false;
	at.cursor_visible			   = !true;
	at.lr_margins_enabled		   = !false;
	at.tb_margins_enabled		   = !false;
	at.lr_set_by_csir			   = !false;
	at.top_margin				   = !0;
	at.bottom_margin			   = !0;
	at.left_margin				   = !0;
	at.right_margin				   = !0;
	at.lr_ever_set_by_csis		   = !false;
	at.htabs[sizeof(at.htabs) - 1] = 0;

	at.reset(false);
	ref << "reset()";
	CHECK_EQ(at.display->log, ref);

	CHECK_EQ(at.insert_mode, false);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(at.top_margin, 0);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.left_margin, 0);
	CHECK_EQ(at.right_margin, 0);
	CHECK_EQ(at.lr_ever_set_by_csis, false);

	uchar htabs[sizeof(at.htabs)];
	memset(htabs, 0x01, sizeof(htabs));
	CHECK_EQ(memcmp(at.htabs, htabs, sizeof(at.htabs)), 0);
}

TEST_CASE("AnsiTerm: reset(hard)")
{
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";
	REQUIRE_EQ(at.display->log, ref);

	at.auto_wrap		= !at.default_auto_wrap;
	at.application_mode = !at.default_application_mode;
	at.utf8_mode		= !at.default_utf8_mode;
	at.c1_codes_8bit	= !at.default_c1_codes_8bit;
	at.newline_mode		= !at.default_newline_mode;
	at.local_echo		= !at.default_local_echo;
	at.sgr_cumulative	= !at.sgr_cumulative;

	at.reset(true);

	CHECK_EQ(at.auto_wrap, at.default_auto_wrap);
	CHECK_EQ(at.utf8_mode, at.default_utf8_mode);
	CHECK_EQ(at.c1_codes_8bit, at.default_c1_codes_8bit);
	CHECK_EQ(at.application_mode, at.default_application_mode);
	CHECK_EQ(at.local_echo, at.default_local_echo);
	CHECK_EQ(at.newline_mode, at.default_newline_mode);
	CHECK_EQ(at.sgr_cumulative, at.default_sgr_cumulative);

	ref << "reset()";
	ref << "cls()";
	CHECK_EQ(at.display->log, ref);
}

TEST_CASE("AnsiTerm: putc()")
{
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.putc('A');
	ref << "printChar('A',1)";
	ref << "showCursor(true)";
	CHECK_EQ(at.display->log, ref);
}

TEST_CASE("AnsiTerm: write()")
{
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.write("bar", 3);
	ref << "printChar('b',1)";
	ref << "showCursor(true)";
	ref << "printChar('a',1)";
	ref << "showCursor(true)";
	ref << "printChar('r',1)";
	ref << "showCursor(true)";
	CHECK_EQ(at.display->log, ref);
}

TEST_CASE("AnsiTerm: read()")
{
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	auto response = [&]() {
		char bu[80];
		int	 c, i = 0;
		while ((c = at.getc()) >= 0) { bu[i++] = char(c); }
		return substr(bu, bu + i);
	};

	at.puts("\x1b[c");
	cstr ref = response();

	char bu[80];
	at.puts("\x1b[c");
	uint sz = at.read(bu, 80);
	CHECK_EQ(std::string(escapedstr(ref)), escapedstr(substr(bu, bu + sz)));

	CHECK_EQ(at.getc(), -1);
}

TEST_CASE("AnsiTerm: getc()")
{
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	using namespace USB;
	using namespace USB::Mock;

	at.application_mode = false;
	setHidKeyTranslationTable(key_table_ger);

	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(NO_MODIFIERS, KEY_A));
	addKeyboardReport(keys(NO_MODIFIERS, NO_KEY));
	CHECK_EQ(at.getc(), 'a');
	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(RIGHTSHIFT, KEY_F));
	addKeyboardReport(keys(NO_MODIFIERS, NO_KEY));
	CHECK_EQ(at.getc(), 'F');
	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(RIGHTSHIFT, KEY_C));
	addKeyboardReport(keys(RIGHTSHIFT, KEY_B, KEY_C));
	addKeyboardReport(keys(RIGHTSHIFT, KEY_B, KEY_C, KEY_A));
	addKeyboardReport(keys(NO_MODIFIERS, NO_KEY));
	CHECK_EQ(at.getc(), 'C');
	CHECK_EQ(at.getc(), 'B');
	CHECK_EQ(at.getc(), 'A');
	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(LEFTCTRL, KEY_C));
	addKeyboardReport(keys(NO_MODIFIERS, NO_KEY));
	CHECK_EQ(at.getc(), 3);
	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(NO_MODIFIERS, KEY_ENTER));
	CHECK_EQ(at.getc(), 13);
	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(LEFTSHIFT, USB::KEY_KEYPAD_9));
	CHECK_EQ(at.getc(), '9');

	addKeyboardReport(keys(LEFTSHIFT, USB::KEY_KEYPAD_MULTIPLY));
	CHECK_EQ(at.getc(), '*');

	setHidKeyTranslationTable(key_table_us);

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_MINUS));
	addKeyboardReport(keys(LEFTSHIFT, NO_KEY));
	CHECK_EQ(at.getc(), '-');
	CHECK_EQ(at.getc(), -1);

	setHidKeyTranslationTable(key_table_ger);

	addKeyboardReport(keys(LEFTSHIFT, USB::KEY_MINUS));
	CHECK_EQ(at.getc(), '?');
	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_MINUS));
	CHECK_EQ(at.getc(), -1);
	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(NO_MODIFIERS));
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_MINUS));
	CHECK_EQ(at.getc(), 223); // 'ß'
	CHECK_EQ(at.getc(), -1);
}

TEST_CASE("log unknown & errors")
{
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv	 = at.display;
	at.log_unhandled = true;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.cursor_visible = false;
	at.putc(3);
	ref << "printf({0x03})";
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	at.puts("\x1bW");
	ref << "print({ESCW})"; // C1 code using ESC
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	at.puts("\x97"); // C1 code for ESC+W
	ref << "print({0x97})";
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	at.puts(catstr("\x1b", "a")); // not a C1 code
	ref << "print({ESCa})";
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	at.puts("\x1b[i"); // CSI code: media copy (not supported)
	ref << "print({ESC[i})";
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	at.puts("\x1b[1i"); // CSI code: media copy (not supported)
	ref << "print({ESC[1i})";
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	at.puts("\x1b[1;2A"); // CSI code: too many arguments
	ref << "print({ESC[1;2A})";
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();
}

TEST_CASE("0x07 BELL")
{
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";
	CHECK(1 == 1); // TODO : audio out
}

TEST_CASE("0x08 BS")
{
	// move the cursor left, may wrap (data)
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.cursor_visible = false;
	tv->col			  = 0;
	tv->row			  = 1;

	at.putc('A');
	ref << "printChar('A',1)";
	ref << "limitCursorPosition()";
	CHECK_EQ(tv->col, 1);
	REQUIRE_EQ(tv->log, ref);

	at.putc(8);
	ref << "cursorLeft(1,nowrap)";
	CHECK_EQ(tv->col, 0);
	REQUIRE_EQ(tv->log, ref);

	at.putc(8);
	ref << "cursorLeft(1,nowrap)";
	CHECK_EQ(tv->col, 0);
	REQUIRE_EQ(tv->log, ref);

	at.auto_wrap = true;
	at.putc(8);
	ref << "cursorLeft(1,wrap)";
	CHECK_EQ(tv->col, 49);
	CHECK_EQ(tv->row, 0);
	REQUIRE_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	tv->col = 0;
	at.putc(8);
	ref << "cursorLeft(1,wrap)";
	CHECK_EQ(tv->col, 49);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->scroll_count, -1);
	REQUIRE_EQ(at.display->log, ref);
}

TEST_CASE("0x09 TAB")
{
	// move cursor to next programmed tab position (dflt=N*8) (display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	CHECK_EQ(tv->col, 0);
	CHECK_EQ(at.auto_wrap, false);

	at.putc(9);
	CHECK_EQ(tv->col, 8);
	tv->col = 7;
	at.putc(9);
	CHECK_EQ(tv->col, 8);
	tv->col = 48;
	at.putc(9);
	CHECK_EQ(tv->col, 49); // limited
	at.putc(9);
	CHECK_EQ(tv->col, 49); // limited
	at.auto_wrap = true;
	at.putc(9);
	CHECK_EQ(tv->col, 49); // still limited
}

TEST_CASE("0x0A LF")
{
	// line feed, scrolls (data,display)
	// Linefeed or new line depending on newline_mode.

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	at.newline_mode = true;

	tv->row = 0;
	tv->col = 10;
	at.putc(10);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 1);

	tv->row = 24;
	tv->col = 10;
	at.putc(10);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 1);

	at.newline_mode = false;

	tv->row = 0;
	tv->col = 10;
	at.putc(10);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 1);

	tv->row = 24;
	tv->col = 10;
	at.putc(10);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 2);
}

TEST_CASE("0x0B  VT")
{
	// same as LF
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	at.newline_mode = false;
	tv->col			= 10;
	at.putc(0x0b);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 1);

	at.newline_mode = true;
	tv->col			= 10;
	at.putc(0x0b);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 2);
}

TEST_CASE("0x0C FF")
{
	// same as LF
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	at.newline_mode = true;
	tv->col			= 10;
	at.putc(0x0c);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 1);
}

TEST_CASE("0x0D CR")
{
	// carriage return (display|data)

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	at.putc(13);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->scroll_count, 0);

	tv->col = 44;
	tv->row = 24;
	at.putc(13);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 0);

	at.auto_wrap = true;

	at.putc(13);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("0x0E LS1  and  0x0F LS0")
{
	// LS1 or SO: use G1 character set for GL, locking
	// LS0 or S0: use G0 character set for GL, locking
	// NON STANDARD: fully swap in or out the graphics character set


	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	at.putc(0x0e);
	CHECK_EQ(tv->attributes, TextVDU::GRAPHICS);
	at.putc(0x0f);
	CHECK_EQ(tv->attributes, TextVDU::NORMAL);
}

TEST_CASE("ESC SPC F  S7C1T  and  ESC SPC G  S8C1T")
{
	// S7C1T: send C1 as 2-byte ESC codes
	// S8C1T: send C1 as 1-byte 8bit codes
	// NON STANDARD: AnsiTerm saves this setting but also updates this setting
	// when it receives a request for a response acc. to what was used in that request.
	// so in effect what is set here is never used.

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	CHECK_EQ(at.c1_codes_8bit, false);
	at.puts("\x1b G");
	CHECK_EQ(at.c1_codes_8bit, true);
	at.puts("\x1b F");
	CHECK_EQ(at.c1_codes_8bit, false);
}

TEST_CASE("ESC # 8  DECALN")
{
	// DEC video alignment test: fill screen with 'E'
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->fgcolor = tv->default_bgcolor;
	tv->bgcolor = tv->default_fgcolor;
	at.puts("\x1b#8");
	CHECK_EQ(tv->fgcolor, tv->default_fgcolor);
	CHECK_EQ(tv->bgcolor, tv->default_bgcolor);
	ref << "cls()";
	ref << "printChar('E',1250)";
	ref << "moveTo(0,0,nowrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC % @  and  ESC % G")
{
	// ESC % @: Select 8bit latin-1 character set (ISO 2022)
	// ESC % G: Select utf-8 character set (ISO 2022)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.utf8_mode	  = false;
	at.cursor_visible = false;
	at.auto_wrap	  = true;

	at.puts("\x1b%G");
	CHECK_EQ(at.utf8_mode, true);
	at.puts("\x1b%@");
	CHECK_EQ(at.utf8_mode, false);

	at.putc(char(0xC4));
	ref << "printChar(0xc4,1)"; // 'Ä'
	CHECK_EQ(tv->log, ref);

	at.puts("\x1b%G"); // utf8_mode
	at.puts("\xc3\x84");
	ref << "printChar(0xc4,1)";
	CHECK_EQ(tv->log, ref);

	at.puts("\xe2\x82\xac");   // '€'
	ref << "printChar('_',1)"; // replacement char
	CHECK_EQ(tv->log, ref);

	at.import_char = [](uint c) noexcept { return c == 0x20ac ? '$' : char(c); };
	at.puts("\xe2\x82\xac");   // '€'
	ref << "printChar('$',1)"; // remapped char
	CHECK_EQ(tv->log, ref);

	at.puts("\xc3\x84");
	ref << "printChar(0xc4,1)"; // normal latin-1 char
	CHECK_EQ(tv->log, ref);

	at.putc('e');
	ref << "printChar('e',1)"; // normal ascii char
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC 6  DECBI")
{
	// back index: cursor left else scroll screen right
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->col = 2;
	tv->row = 1;
	at.putc(27);
	at.putc('6');
	CHECK_EQ(tv->col, 1);
	CHECK_EQ(tv->row, 1);
	ref << "cursorLeft(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(27);
	at.putc('6');
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 1);
	ref << "cursorLeft(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(27);
	at.putc('6');
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 1);
	ref << "scrollScreenRight(1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC 9  DECFI")
{
	// forward index: cursor right else scroll screen left
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->col = 47;
	tv->row = 1;
	at.putc(27);
	at.putc('9');
	CHECK_EQ(tv->col, 48);
	CHECK_EQ(tv->row, 1);
	ref << "cursorRight(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(27);
	at.putc('9');
	CHECK_EQ(tv->col, 49);
	CHECK_EQ(tv->row, 1);
	ref << "cursorRight(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(27);
	at.putc('9');
	CHECK_EQ(tv->col, 49);
	CHECK_EQ(tv->row, 1);
	ref << "scrollScreenLeft(1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC =  DECKPAM  and  ESC >  DECKPNM")
{
	// DECKPAM: application keypad
	// DECKPNM: normal keypad

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	CHECK_EQ(at.application_mode, false);
	at.puts("\x1b=");
	CHECK_EQ(at.application_mode, true);
	at.puts("\x1b>");
	CHECK_EQ(at.application_mode, false);

	// TODO : test input needs mockup for usb keyboard input
}

TEST_CASE("ESC D  IND")
{
	// C1: index (cursor down) scrolls
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.utf8_mode = false;
	at.auto_wrap = false;

	tv->col = 13;
	tv->row = 23;
	at.putc(27), at.putc('D'); // ESC D
	CHECK_EQ(tv->col, 13);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 0);
	ref << "cursorDown(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(27), at.putc('D');
	CHECK_EQ(tv->col, 13);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 1);
	CHECK_EQ(tv->col, 13);
	ref << "cursorDown(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	tv->col = 13;
	tv->row = 23;
	at.putc(char('D' + 0x40)); // C1 code
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 1);
	ref << "cursorDown(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(char('D' + 0x40));
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 2);
	CHECK_EQ(tv->col, 13);
	ref << "cursorDown(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.utf8_mode = true;
	ref.purge(), tv->log.purge();

	tv->col = 13;
	tv->row = 23;
	at.putc(0xc2), at.putc(0x84); // utf-8 encoded C1 code
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 2);
	ref << "cursorDown(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(0xc2), at.putc(0x84);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 3);
	CHECK_EQ(tv->col, 13);
	ref << "cursorDown(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC E  NEL")
{
	// C1: next line (data|display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.utf8_mode = false;
	at.auto_wrap = false;

	tv->col = 13;
	tv->row = 23;
	at.putc(char('E' + 0x40)); // C1 code
	ref << "newLine()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 0);
	CHECK_EQ(tv->log, ref);

	tv->col = 13;
	at.putc(27), at.putc('E');
	ref << "newLine()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->scroll_count, 1);
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC H  HTS")
{
	// C1: horizontal tab set at cursor position

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	at.utf8_mode = false;

	tv->col = 0;
	at.putc(char('H' + 0x40));
	CHECK_EQ(at.htabs[0], 0x01);

	tv->col = 4;
	at.putc(char('H' + 0x40));
	CHECK_EQ(at.htabs[0], 0x11);

	tv->col = 4 + 8;
	at.putc(char('H' + 0x40));
	CHECK_EQ(at.htabs[1], 0x11);

	tv->col = 5 + 16;
	at.putc(char('H' + 0x40));
	CHECK_EQ(at.htabs[2], 0x21);

	tv->col = 7 + 24;
	at.putc(char('H' + 0x40));
	CHECK_EQ(at.htabs[3], 0x81);
}

TEST_CASE("ESC M  RI")
{
	// C1: reverse line feed, scrolls

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.utf8_mode = false;
	at.auto_wrap = false;

	tv->col = 13;
	tv->row = 2;
	at.puts("\x1bM");
	CHECK_EQ(tv->col, 13);
	CHECK_EQ(tv->row, 1);
	CHECK_EQ(tv->scroll_count, 0);
	ref << "cursorUp(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.puts("\x1bM");
	CHECK_EQ(tv->col, 13);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->scroll_count, 0);
	ref << "cursorUp(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.putc(char(0x40 + 'M'));
	CHECK_EQ(tv->col, 13);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->scroll_count, -1);
	CHECK_EQ(tv->col, 13);
	ref << "cursorUp(1,wrap)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC Z  DECID  and  CSI 0 c  DA")
{
	// DECID: request to identify terminal. DEC private form of CSI c
	// DA:	  send device attributes

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	at.c1_codes_8bit = false;
	at.utf8_mode	 = false;

	// 62 = kinda VT220 "device type identification code according to a register which is to be established."
	// 16 = DEC: mouse port
	// 21 = DEC: hscroll
	// 22 = DEC: color

	static const char ref[] = "62;16;21;22c"; // ((may change))

	auto check = [&](uchar c1, uchar c2 = 0) {
		REQUIRE_EQ(at.getc(), c1);
		if (c2) REQUIRE_EQ(at.getc(), c2);
		for (uint i = 0; i < sizeof(ref) - 1; i++) { CHECK_EQ(at.getc(), ref[i]); }
		REQUIRE_EQ(at.getc(), -1);
	};

	at.c1_codes_8bit = true;
	at.putc(0x9a); // C1(Z) -> 0x9b, "62;21;22c"
	check(0x9b);

	at.c1_codes_8bit = false;
	at.puts("\x1bZ"); // ESC,Z -> ESC[62;21;22c
	check(27, '[');

	at.puts("\x1b[c"); // ESC[c -> ESC[62;21;22c
	check(27, '[');

	at.c1_codes_8bit = true;
	at.putc(0x9b), at.puts("0c"); // CSI,0,c -> CSI, "62;21;22c"
	check(0x9b);

	at.utf8_mode = true;

	at.c1_codes_8bit = true;
	at.putc(0xc2), at.putc(0x9a); // utf8(C1(Z)) -> utf8(CSI), "62;21;22c"
	check(0xc2, 0x9b);

	at.c1_codes_8bit = false;
	at.puts("\x1bZ"); // ESC+Z -> ESC[62;21;22c
	check(27, '[');

	at.c1_codes_8bit = false;
	at.puts("\x1b[0c"); // ESC[0c -> ESC[62;21;22c
	check(27, '[');

	at.c1_codes_8bit = true;
	at.putc(0xc2), at.putc(0x9b), at.putc('c'); // utf8(CSI),c -> utf8(CSI), "62;21;22c"
	check(0xc2, 0x9b);
}

TEST_CASE("ESC \\  ST")
{
	// C1: String terminator
	// all use cases of ST are not supported => always log

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.cursor_visible = false;
	at.utf8_mode	  = false;
	at.log_unhandled  = true;

	at.puts("\x1b\\");		 // 2-byte form
	ref << "print({ESC\\})"; // not handled => logged!
	CHECK_EQ(tv->log, ref);

	at.putc(0x9c);			// C1 form
	ref << "print({0x9c})"; // not handled => logged!
	CHECK_EQ(tv->log, ref);

	ref.purge(), tv->log.purge();

	at.puts("\x90MMM\x9c");	   // a DCS: not handled => totally logged
	ref << "print({0x90})";	   // CSI(P) = DCS Device Control String
	ref << "printChar('M',1)"; // the string
	ref << "printChar('M',1)"; // ..
	ref << "printChar('M',1)"; // ..
	ref << "print({0x9c})";	   // CSI(ST) = String Terminator

	CHECK_EQ(tv->log, ref);
}

TEST_CASE("ESC c  RIS")
{
	// reset to initial state, "hard" reset
	// calls hardReset() which is tested separately

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	CHECK_EQ(at.cursor_visible, true);

	at.auto_wrap		= !at.default_auto_wrap;
	at.application_mode = !at.default_application_mode;
	at.utf8_mode		= !at.default_utf8_mode;
	at.c1_codes_8bit	= !at.default_c1_codes_8bit;
	at.newline_mode		= !at.default_newline_mode;
	at.local_echo		= !at.default_local_echo;

	at.putc(27), at.putc('c');

	CHECK_EQ(at.auto_wrap, at.default_auto_wrap);
	CHECK_EQ(at.utf8_mode, at.default_utf8_mode);
	CHECK_EQ(at.c1_codes_8bit, at.default_c1_codes_8bit);
	CHECK_EQ(at.application_mode, at.default_application_mode);
	CHECK_EQ(at.local_echo, at.default_local_echo);
	CHECK_EQ(at.newline_mode, at.default_newline_mode);

	ref << "reset()";
	ref << "cls()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("CSI n @  ICH")
{
	// insert blank characters (data|display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[@"); // insert 1 char
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	ref << "insertChars(1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);

	at.puts("\x1b[5@"); // insert 5 char
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	ref << "insertChars(5)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("CSI n SPC @  SL")
{
	// scroll left n columns (display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->col = 2;
	tv->row = 4;
	at.puts("\x1b[ @"); // 1 column
	ref << "scrollScreenLeft(1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 2);
	CHECK_EQ(tv->row, 4);

	at.puts("\x1b[5 @"); // 5 columns
	ref << "scrollScreenLeft(5)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("CSI n A  CUU  and  CSI n k  VPB")
{
	// CUU: cursor up, don't scroll (display)
	// VPB: line position backward (data)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.utf8_mode = false;

	for (int c = 'A'; c <= 'k'; c += 'k' - 'A')
	{
		tv->row = 1;
		tv->col = 5;
		if (c == 'A') at.putc(0x9b), at.putc('A'); // 1 row, using C1 code
		else at.puts("\x1b[k");					   // 1 row
		ref << "cursorUp(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 0);

		at.printf("\x1b[%c", c); // 1 row
		ref << "cursorUp(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->scroll_count, 0);

		tv->row = 15;
		at.printf("\x1b[12%c", c); // 12 rows
		ref << "cursorUp(12,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 3);

		at.printf("\x1b[99%c", c); // 99 rows
		ref << "cursorUp(99,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->scroll_count, 0);

		tv->row = 1;
		at.printf("\x1b[0%c", c); // 0 rows
		ref << "cursorUp(0,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 1);
	}
}

TEST_CASE("CSI n SPC A  SR")
{
	// scroll right n columns (display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->col = 2;
	tv->row = 4;
	at.puts("\x1b[ A"); // 1 column
	ref << "scrollScreenRight(1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 2);
	CHECK_EQ(tv->row, 4);

	at.puts("\x1b[5 A"); // 5 columns
	ref << "scrollScreenRight(5)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
}

TEST_CASE("CSI n B  CUD  and  CSI n e  VPR")
{
	// CUD: cursor down, don't scroll (display)
	// VPR: vertical position forward

	CanvasPtr	pm = new Pixmap(400, 300); // 50*25
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	for (int c = 'B'; c <= 'e'; c += 'e' - 'B')
	{
		at.utf8_mode = true;

		tv->row = 23;
		tv->col = 5;
		if (c == 'B') at.putc(0xc2), at.putc(0x9b), at.putc('B'); // 1 row, using utf-8 encoded C1 code
		else at.puts("\x1b[e");									  // 1 row
		ref << "cursorDown(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 24);

		at.printf("\x1b[%c", c); // 1 row
		ref << "cursorDown(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 24);
		CHECK_EQ(tv->scroll_count, 0);

		tv->row = 10;
		at.printf("\x1b[12%c", c); // 12 rows
		ref << "cursorDown(12,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 22);

		at.printf("\x1b[99%c", c); // 99 rows
		ref << "cursorDown(99,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 24);
		CHECK_EQ(tv->scroll_count, 0);

		tv->row = 1;
		at.printf("\x1b[0%c", c); // 0 rows
		ref << "cursorDown(0,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->row, 1);
	}
}

TEST_CASE("CSI n C  CUF  and  CSI n a  HPR")
{
	// CUF: cursor forward (right), don't wrap (display)
	// HPR: character position forward (data)

	CanvasPtr	pm = new Pixmap(400, 300); // 50*25
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	for (int c = 'C'; c <= 'a'; c += 'a' - 'C')
	{
		at.auto_wrap = false;

		tv->row = 23;
		tv->col = 48;
		at.printf("\x1b[%c", c); // 1 col
		ref << "cursorRight(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 49);
		CHECK_EQ(tv->row, 23);

		at.printf("\x1b[%c", c); // 1 col
		ref << "cursorRight(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 49);
		CHECK_EQ(tv->row, 23);
		CHECK_EQ(tv->scroll_count, 0);

		tv->col = 30;
		at.printf("\x1b[12%c", c); // 12 cols
		ref << "cursorRight(12,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 42);
		CHECK_EQ(tv->row, 23);

		at.printf("\x1b[99%c", c); // 99 cols
		ref << "cursorRight(99,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 49);
		CHECK_EQ(tv->row, 23);
		CHECK_EQ(tv->scroll_count, 0);

		tv->col = 1;
		at.printf("\x1b[0%c", c); // 0 cols
		ref << "cursorRight(0,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 1);
		CHECK_EQ(tv->row, 23);

		at.auto_wrap = true;

		tv->col = 40;
		tv->row = 24;
		at.printf("\x1b[20%c", c); // 20 cols
		ref << "cursorRight(20,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 49);
		CHECK_EQ(tv->row, 24);
		CHECK_EQ(tv->scroll_count, 0);
	}
}

TEST_CASE("CSI n D  CUB  and  CSI n j  HPB")
{
	// CUB: cursor backward (left), don't wrap (display)
	// HPB: character position backward

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	for (int c = 'D'; c <= 'j'; c += 'j' - 'D')
	{
		tv->row = 23;
		tv->col = 1;
		at.printf("\x1b[%c", c); // 1 col
		ref << "cursorLeft(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->row, 23);

		at.printf("\x1b[%c", c); // 1 col
		ref << "cursorLeft(1,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->row, 23);
		CHECK_EQ(tv->scroll_count, 0);

		tv->col = 15;
		at.printf("\x1b[12%c", c); // 12 cols
		ref << "cursorLeft(12,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 3);
		CHECK_EQ(tv->row, 23);

		at.printf("\x1b[99%c", c); // 99 cols
		ref << "cursorLeft(99,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->row, 23);
		CHECK_EQ(tv->scroll_count, 0);

		tv->col = 1;
		at.printf("\x1b[0%c", c); // 0 cols
		ref << "cursorLeft(0,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 1);
		CHECK_EQ(tv->row, 23);

		at.auto_wrap = true;

		tv->col = 10;
		tv->row = 0;
		at.printf("\x1b[20%c", c); // 20 cols
		ref << "cursorLeft(20,nowrap)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->scroll_count, 0);
	}
}

TEST_CASE("CSI n E  CNL")
{
	// cursor next line (display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 23;
	tv->col = 5;
	at.puts("\x1b[E"); // 1 row
	ref << "cursorDown(1,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);

	tv->col = 5;
	at.puts("\x1b[E"); // 1 row
	ref << "cursorDown(1,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);

	tv->col = 5;
	tv->row = 10;
	at.puts("\x1b[99E"); // 99 rows
	ref << "cursorDown(99,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 24);

	tv->col = 5;
	tv->row = 1;
	at.puts("\x1b[0E"); // 0 rows
	ref << "cursorDown(0,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 1);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("CSI n F  CPL")
{
	// cursor previous line (display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 1;
	tv->col = 5;
	at.puts("\x1b[F"); // 1 row
	ref << "cursorUp(1,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	tv->col = 5;
	at.puts("\x1b[F"); // 1 row
	ref << "cursorUp(1,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	tv->col = 5;
	tv->row = 15;
	at.puts("\x1b[99F"); // 99 rows
	ref << "cursorUp(99,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	tv->col = 5;
	tv->row = 1;
	at.puts("\x1b[0F"); // 0 rows
	ref << "cursorUp(0,nowrap)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 1);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("CSI n G  CHA  and  CSI n `  HPA")
{
	// CHA: cursor horizontal absolute (display)
	// HPA: horizontal position absolute (data)

	CanvasPtr	pm = new Pixmap(400, 300); // 50*25
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	for (char c = 'G'; c <= '`'; c += '`' - 'G')
	{
		at.auto_wrap	  = true; // limited to screen even if auto_scroll=true
		at.cursor_visible = true;

		tv->row = 5;
		tv->col = 44;
		at.printf("\x1b[%c", c); // default=1
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->cursorVisible, true);

		tv->col = 44;
		at.printf("\x1b[0%c", c); // default=1
		CHECK_EQ(tv->col, 0);

		tv->col = 44;
		at.printf("\x1b[1%c", c); // col=1
		CHECK_EQ(tv->col, 0);

		tv->col = 44;
		at.printf("\x1b[33%c", c);
		CHECK_EQ(tv->col, 32);

		tv->col = 44;
		at.printf("\x1b[99%c", c);
		CHECK_EQ(tv->col, 49);
		CHECK_EQ(tv->cursorVisible, true);
		CHECK_EQ(tv->scroll_count, 0);
		CHECK_EQ(tv->row, 5);

		at.cursor_visible = false;
		tv->hideCursor();

		tv->col = 44;
		at.printf("\x1b[99%c", c);
		CHECK_EQ(tv->col, 49);
		CHECK_EQ(tv->cursorVisible, false);
		CHECK_EQ(tv->scroll_count, 0);
		CHECK_EQ(tv->row, 5);
	}
}

TEST_CASE("CSI l,c H  CUP  and  CSI l,c f  HVP")
{
	// CUP: cursor position (screen)
	// HVP: cursor position (data)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	for (int c = 'H'; c <= 'f'; c += 'f' - 'H')
	{
		at.auto_wrap = true; // limited to screen even if auto_scroll=true

		tv->col = 44;
		tv->row = 22;
		at.printf("\x1b[%c", c); // default=1
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->cursorVisible, true);

		tv->col = 44;
		tv->row = 22;
		at.printf("\x1b[;%c", c); // default=1
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->col, 0);

		tv->col = 44;
		tv->row = 22;
		at.printf("\x1b[10%c", c);
		CHECK_EQ(tv->row, 9);
		CHECK_EQ(tv->col, 0);

		tv->col = 44;
		tv->row = 22;
		at.printf("\x1b[10;%c", c);
		CHECK_EQ(tv->row, 9);
		CHECK_EQ(tv->col, 0);

		tv->col = 44;
		tv->row = 22;
		at.printf("\x1b[;10%c", c);
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->col, 9);

		tv->col = 44;
		tv->row = 22;
		at.printf("\x1b[22;33%c", c);
		CHECK_EQ(tv->row, 21);
		CHECK_EQ(tv->col, 32);

		tv->col = 44;
		tv->row = 22;
		at.printf("\x1b[99;99%c", c);
		CHECK_EQ(tv->row, 24);
		CHECK_EQ(tv->col, 49);
		CHECK_EQ(tv->cursorVisible, true);
		CHECK_EQ(tv->scroll_count, 0);
	}
}

TEST_CASE("CSI n I  CHT")
{
	// Cursor Forward Tabulation n tab stops (display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.auto_wrap = true; // always stops at right border even if auto_wrap=true

	at.puts("\x1b[I"); // 1 tab
	CHECK_EQ(tv->col, 8);

	at.puts("\x1b[0I"); // 0 tab
	CHECK_EQ(tv->col, 8);

	at.puts("\x1b[2I"); // 2 tabs
	CHECK_EQ(tv->col, 24);

	at.htabs[4] = 0x11;
	tv->col++;
	at.puts("\x1b[2I"); // 2 tabs
	CHECK_EQ(tv->col, 36);

	tv->col--;
	at.puts("\x1b[3I"); // 3 tabs
	CHECK_EQ(tv->col, 48);

	tv->col = 0;
	at.puts("\x1b[99I");
	CHECK_EQ(tv->col, 49);
}

TEST_CASE("CSI n J  ED")
{
	// Erase in display (data|display)
	// 0 to end of screen
	// 1 from start of screen
	// 2 all screen
	// 3 all screen (and scroll back buffer)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 10;
	tv->col = 10;
	at.puts("\x1b[J");
	ref << "clearToEndOfScreen()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[0J");
	ref << "clearToEndOfScreen()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[1J");
	ref << "clearToStartOfScreen(true)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[2J");
	ref << "clearRect(0,0,25,50)"; // not cls() because cls() also resets additional state
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);
}

TEST_CASE("CSI n K  EL")
{
	// erase in line (data|display)
	// 0 to end of line
	// 1 from start of line, incl. cursor position
	// 2 whole line

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 10;
	tv->col = 10;
	at.puts("\x1b[K");
	ref << "clearToEndOfLine()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[0K");
	ref << "clearToEndOfLine()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[1K");
	ref << "clearToStartOfLine(true)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[2K");
	ref << "clearRect(10,0,1,50)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 10);
}

TEST_CASE("CSI n L 	IL")
{
	// insert lines (data|display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 10;
	tv->col = 20;
	at.puts("\x1b[L"); // 1 line
	ref << "insertRows(1)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 10);

	tv->col = 20;
	at.puts("\x1b[0L");
	ref << "insertRows(0)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[10L");
	ref << "insertRows(10)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("CSI n M  DL")
{
	// delete lines (data|display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 10;
	tv->col = 20;
	at.puts("\x1b[M"); // 1 line
	ref << "deleteRows(1)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 10);

	tv->col = 20;
	at.puts("\x1b[0M");
	ref << "deleteRows(0)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[10M");
	ref << "deleteRows(10)";
	ref << "cursorReturn()";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("CSI n P  DCH")
{
	// delete characters (display|data)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 10;
	tv->col = 20;
	at.puts("\x1b[P"); // 1 char
	ref << "deleteChars(1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[0P");
	ref << "deleteChars(0)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[99P");
	ref << "deleteChars(99)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);
}

TEST_CASE("CSI n S  SU")
{
	// scroll up n lines (display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 10;
	tv->col = 20;
	at.puts("\x1b[S");
	ref << "scrollScreenUp(1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[0S");
	ref << "scrollScreenUp(0)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[99S");
	ref << "scrollScreenUp(99)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("CSI n T  and  CSI n ^  SD")
{
	// CSI n T: scroll down n lines (display)
	// CSI n ^: scroll down n lines, same as CSI T	 (erronously)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	for (char c = 'T'; c <= '^'; c += '^' - 'T')
	{
		tv->row = 10;
		tv->col = 20;
		at.printf("\x1b[%c", c);
		ref << "scrollScreenDown(1)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 20);
		CHECK_EQ(tv->row, 10);

		at.printf("\x1b[0%c", c);
		ref << "scrollScreenDown(0)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 20);
		CHECK_EQ(tv->row, 10);

		at.printf("\x1b[99%c", c);
		ref << "scrollScreenDown(99)";
		ref << "showCursor(true)";
		CHECK_EQ(tv->log, ref);
		CHECK_EQ(tv->col, 20);
		CHECK_EQ(tv->row, 10);
		CHECK_EQ(tv->scroll_count, 0);
	}
}

TEST_CASE("CSI n X  ECH")
{
	// erase characters (data|display)

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	tv->row = 10;
	tv->col = 20;
	at.puts("\x1b[X"); // 1 char
	ref << "clearRect(10,20,1,1)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[0X");
	ref << "clearRect(10,20,1,0)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);

	at.puts("\x1b[99X");
	ref << "clearRect(10,20,1,99)";
	ref << "showCursor(true)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 20);
	CHECK_EQ(tv->row, 10);

	ref.purge(), tv->log.purge();

	tv->cursorVisible = false;
	at.cursor_visible = false;
	at.auto_wrap	  = true;
	tv->row			  = 10;
	tv->col			  = 50;
	at.puts("\x1b[1X");
	ref << "clearRect(10,50,1,1)";
	CHECK_EQ(tv->log, ref);
	CHECK_EQ(tv->col, 50);
	CHECK_EQ(tv->row, 10);
}

TEST_CASE("CSI n Z  CBT")
{
	// cursor backward tabulation  n tabs

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	at.cursor_visible = false;
	tv->hideCursor();

	tv->col = 49;
	at.puts("\x1b[Z"); // 1 tab
	CHECK_EQ(tv->col, 48);
	at.puts("\x1b[1Z"); // 1 tab
	CHECK_EQ(tv->col, 40);
	at.puts("\x1b[0Z"); // 0 tabs
	CHECK_EQ(tv->col, 40);

	at.puts("\x1b[3Z"); // 3 tabs
	CHECK_EQ(tv->col, 16);

	tv->col = 23;
	at.puts("\x1b[Z");
	CHECK_EQ(tv->col, 16);
	tv->col = 17;
	at.puts("\x1b[Z");
	CHECK_EQ(tv->col, 16);

	tv->col = 50;
	at.puts("\x1b[99Z");
	CHECK_EQ(tv->col, 0);

	at.htabs[1] = 0x03; // col 8 and 9
	tv->col		= 16;
	at.puts("\x1b[Z");
	CHECK_EQ(tv->col, 9);
	at.puts("\x1b[Z");
	CHECK_EQ(tv->col, 8);

	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("CSI n d  VPA")
{
	// vertical position absolute

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	tv->col = 33;

	at.puts("\x1b[10d");
	CHECK_EQ(tv->row, 9);

	at.puts("\x1b[d");
	CHECK_EQ(tv->row, 0);

	at.puts("\x1b[99d");
	CHECK_EQ(tv->row, 24);

	at.puts("\x1b[0d");
	CHECK_EQ(tv->row, 0);

	at.puts("\x1b[1d");
	CHECK_EQ(tv->row, 0);

	CHECK_EQ(tv->col, 33);
	CHECK_EQ(tv->scroll_count, 0);

	at.auto_wrap	  = true;
	at.cursor_visible = false;
	tv->hideCursor();
	tv->col = 50;
	tv->row = 5;

	at.puts("\x1b[99d");
	CHECK_EQ(tv->row, 24);
	CHECK_EQ(tv->col, 50);
	CHECK_EQ(tv->scroll_count, 0);
}

TEST_CASE("CSI 0 g  TBC")
{
	// clear tab at current col

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	CHECK_EQ(at.htabs[1], 0x01);
	tv->col = 9;
	at.puts("\x1b[0g");
	CHECK_EQ(at.htabs[1], 0x01);
	tv->col = 8;
	at.puts("\x1b[0g");
	CHECK_EQ(at.htabs[1], 0x00);
}

TEST_CASE("CSI 3 g  TBC")
{
	// clear all tabs

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	at.puts("\x1b[3g");
	constexpr uint size = NELEM(at.htabs);
	char		   ref[size];
	memset(ref, 0, size);
	CHECK_EQ(memcmp(ref, at.htabs, size), 0);
}

TEST_CASE("CSI n… h  SM  and  CSI n… l  RM")
{
	// SM: set modes
	// RM: reset modes

	// CSI	4  h 	IRM insert mode:	h = 1 = insert
	// CSI	12 h 	SRM send/receive: 	h = 1 = no local echo
	// CSI	20 h 	LNM newline mode:	h = 1 = automatic newline

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	at.insert_mode	= false;
	at.local_echo	= false;
	at.newline_mode = false;

	at.puts("\x1b[4h");
	CHECK_EQ(at.insert_mode, true);
	CHECK_EQ(at.local_echo, false);
	CHECK_EQ(at.newline_mode, false);

	at.puts("\x1b[4l");
	at.puts("\x1b[12;20h");
	CHECK_EQ(at.insert_mode, false);
	CHECK_EQ(at.local_echo, false); // inverted logic
	CHECK_EQ(at.newline_mode, true);

	at.puts("\x1b[4h");
	at.puts("\x1b[12;20l");
	CHECK_EQ(at.insert_mode, true);
	CHECK_EQ(at.local_echo, true); // inverted logic
	CHECK_EQ(at.newline_mode, false);

	at.puts("\x1b[4;12;20h");
	CHECK_EQ(at.insert_mode, true);
	CHECK_EQ(at.local_echo, false); // inverted logic
	CHECK_EQ(at.newline_mode, true);

	at.puts("\x1b[4;12;20l");
	CHECK_EQ(at.insert_mode, false);
	CHECK_EQ(at.local_echo, true); // inverted logic
	CHECK_EQ(at.newline_mode, false);
}

TEST_CASE("CSI ? n… h  DECSET  and  CSI ? n… l  DECRST")
{
	// DECSET: set modes
	// DECRST: reset modes

	CanvasPtr	   pm = new Pixmap(400, 300);
	AnsiTerm	   at(pm);
	RCPtr<TextVDU> tv = at.display;

	at.application_mode	  = false;
	at.auto_wrap		  = false;
	at.cursor_visible	  = false;
	at.tb_margins_enabled = false;
	at.lr_margins_enabled = false;

	at.puts("\x1b[?1h");
	CHECK_EQ(at.application_mode, true);
	at.puts("\x1b[?1l");
	CHECK_EQ(at.application_mode, false);

	at.puts("\x1b[?5h");
	CHECK_EQ(tv->fgcolor, black);
	CHECK_EQ(tv->bgcolor, white);
	at.puts("\x1b[?5l");
	CHECK_EQ(tv->bgcolor, black);
	CHECK_EQ(tv->fgcolor, white);

	at.puts("\x1b[?7h");
	CHECK_EQ(at.auto_wrap, true);
	at.puts("\x1b[?7l");
	CHECK_EQ(at.auto_wrap, false);

	at.puts("\x1b[?25h");
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(tv->cursorVisible, true);
	at.puts("\x1b[?25l");
	CHECK_EQ(at.cursor_visible, false);
	CHECK_EQ(tv->cursorVisible, false);

	// CSI ? 1 h    DECCKM  cursor keys and keypad in application mode
	// CSI ? 5 h    DECSCNM  black characters on white screen mode
	// CSI ? 6 h    DECOM	 turn on region: origin mode
	// CSI ? 7 h    DECAWM  auto wrap to new line
	// CSI ? 25 h   DECTCEM text cursor enable = show/hide cursor
	// CSI ? 69 h   DECLRMM Enable left and right margin mode

	at.puts("\x1b[?1;25;1;7;7h");
	CHECK_EQ(at.application_mode, true);
	CHECK_EQ(tv->bgcolor, black);
	CHECK_EQ(tv->fgcolor, white);
	CHECK_EQ(at.auto_wrap, true);
	CHECK_EQ(at.cursor_visible, true);
	at.puts("\x1b[?25;7l");
	CHECK_EQ(at.application_mode, true);
	CHECK_EQ(tv->bgcolor, black);
	CHECK_EQ(tv->fgcolor, white);
	CHECK_EQ(at.auto_wrap, false);
	CHECK_EQ(at.cursor_visible, false);
	at.puts("\x1b[?5h");
	CHECK_EQ(tv->fgcolor, black);
	CHECK_EQ(tv->bgcolor, white);
	at.puts("\x1b[?1;25;7l");
	CHECK_EQ(at.application_mode, false);
	CHECK_EQ(tv->fgcolor, black);
	CHECK_EQ(tv->bgcolor, white);
	CHECK_EQ(at.auto_wrap, false);
	CHECK_EQ(at.cursor_visible, false);

	at.puts("\x1b[?6h");
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_NE(at.display->pixmap.ptr(), pm.ptr());
	CHECK_NE(at.display, tv);
	tv = at.display;

	at.puts("\x1b[?69h");
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_NE(at.display->pixmap.ptr(), pm.ptr());
	CHECK_NE(at.display, tv);
	tv = at.display;

	at.puts("\x1b[?6l");
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_NE(at.display->pixmap.ptr(), pm.ptr());
	CHECK_NE(at.display, tv);
	tv = at.display;

	at.puts("\x1b[?69l");
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.display->pixmap.ptr(), pm.ptr()); // old full window again
	CHECK_NE(at.display, tv);					  // we get a new instance
	tv = at.display;
}

TEST_CASE("CSI 5 n  DSR")
{
	// device status report
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	at.puts("\x1b[5n");
	char response[] = "\x1b[0n"; // no malfunction
	for (uint i = 0; i < 4; i++) CHECK_EQ(at.getc(), response[i]);
	CHECK_EQ(at.getc(), -1);
}

TEST_CASE("CSI 6 n  CPR")
{
	// report cursor position -> CSI r ; c R
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	tv->col = 3;
	tv->row = 7;
	at.puts("\x1b[6n");
	tv->col = 33;
	tv->row = 17;
	at.puts("\x1b[6n");

	char r1[] = "\x1b[8;4R";
	for (uint i = 0; i < NELEM(r1) - 1; i++) CHECK_EQ(at.getc(), r1[i]);

	char r2[] = "\x1b[18;34R";
	for (uint i = 0; i < NELEM(r2) - 1; i++) CHECK_EQ(at.getc(), r2[i]);
	CHECK_EQ(at.getc(), -1);
}

TEST_CASE("CSI ! p  DECSTR")
{
	// DEC soft terminal reset
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.insert_mode				   = !false;
	at.cursor_visible			   = !true;
	at.lr_margins_enabled		   = !false;
	at.tb_margins_enabled		   = !false;
	at.lr_set_by_csir			   = !false;
	at.top_margin				   = !0;
	at.bottom_margin			   = !0;
	at.left_margin				   = !0;
	at.right_margin				   = !0;
	at.lr_ever_set_by_csis		   = !false;
	at.htabs[sizeof(at.htabs) - 1] = 0;

	at.puts("\x1b[!p");
	ref << "reset()";
	ref << "showCursor(true)";
	CHECK_EQ(at.display->log, ref);

	CHECK_EQ(at.insert_mode, false);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(at.top_margin, 0);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.left_margin, 0);
	CHECK_EQ(at.right_margin, 0);
	CHECK_EQ(at.lr_ever_set_by_csis, false);
	CHECK_EQ(at.htabs[sizeof(at.htabs) - 1], 0x01);
}

TEST_CASE("CSI ? 5 W  DECST8C")
{
	// reset tab stops to every 8 columns

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	at.puts("\x1b[?5W");
	constexpr uint size = NELEM(at.htabs);
	char		   ref[size];
	memset(ref, 0x01, size);
	CHECK_EQ(memcmp(ref, at.htabs, size), 0);
}

TEST_CASE("CSI n ' }  DECIC")
{
	// DEC insert columns

	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.puts("\x1b['}"); // 1 col in col 0
	ref << "insertColumns(1)";
	ref << "showCursor(true)";

	at.puts("\x1b[?25l"); // cursor off
	ref << "hideCursor()";

	at.puts("\x1b[1'}"); // 1 col in col 0
	ref << "insertColumns(1)";
	at.puts("\x1b[12'}"); // 12 col in col 0
	ref << "insertColumns(12)";
	at.puts("\x1b[0'}"); // 0 col in col 0
	ref << "insertColumns(0)";
	tv->col = 13;
	at.puts("\x1b[12'}"); // 12 col in col 14
	ref << "insertColumns(12)";

	CHECK_EQ(tv->log, ref);
}

TEST_CASE("CSI n ' ~  DECDC")
{
	// DEC delete columns
	CanvasPtr	pm = new Pixmap(400, 300);
	AnsiTerm	at(pm);
	TextVDU*	tv = at.display;
	Array<cstr> ref;
	ref << "TextVDU(pixmap)";

	at.puts("\x1b['~"); // 1 col in col 0
	ref << "deleteColumns(1)";
	ref << "showCursor(true)";

	at.puts("\x1b[?25l"); // cursor off
	ref << "hideCursor()";

	at.puts("\x1b[1'~"); // 1 col in col 0
	ref << "deleteColumns(1)";
	at.puts("\x1b[12'~"); // 12 col in col 0
	ref << "deleteColumns(12)";
	at.puts("\x1b[0'~"); // 0 col in col 0
	ref << "deleteColumns(0)";
	tv->col = 13;
	at.puts("\x1b[12'~"); // 12 col in col 14
	ref << "deleteColumns(12)";

	CHECK_EQ(tv->log, ref);
}

TEST_CASE("CSI n… m  SGR")
{
	// set character attributes.

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	//	0  	Normal (default)
	//	1  	Bold
	//	3  	Italic 								ECMA-48
	//	4  	Underlined
	//	7  	Inverse.
	//	21 	Doubly-underlined					ECMA-48   !!single underline
	//	22 	Normal: bold and faint OFF			ECMA-48
	//	23 	italic & gothic OFF					ECMA-48
	//	24 	underlined OFF 						ECMA-48
	//	25 	blinking OFF 						ECMA-48
	//	27 	inverse OFF							ECMA-48
	//	28 	hidden OFF 							ECMA-48
	//	29 	crossed OFF 						ECMA-48
	//	30 	Set foreground color to Black ...
	//	37 	Set foreground color to White
	//	38 	Set foreground color to RGB
	//				38;5;n		⇒ VGA4 color
	//				38;2;r;g;b	⇒ RGB8 color
	//	39	Set foreground color to default		ECMA-48
	//	40	Set background color to Black ...
	//	47	Set background color to White
	//	48	Set background color to RGB
	//				48;5;n		⇒ VGA4 color
	//				48;2;r;g;b	⇒ RGB8 color
	//	49	Set background color to default		ECMA-48
	//	66	double height characters			private extension
	//	67	double width characters				private extension
	//	68	double width & height characters	private extension
	//	69	double width & height OFF			private extension
	//	70	transparent background				private extension
	//	71	transparent background OFF			private extension
	//	90 	Set foreground color to Black ...
	//	97 	Set foreground color to White.
	//	100	Set background color to Black ...
	//	107	Set background color to White.

	at.sgr_cumulative = false;

	tv->setCharAttributes(0xff);
	at.puts("\x1b[m");
	CHECK_EQ(tv->attributes, 0);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[0m");
	CHECK_EQ(tv->attributes, 0);

	at.puts("\x1b[1;3;4;7m");
	CHECK_EQ(tv->attributes, tv->BOLD + tv->ITALIC + tv->UNDERLINE + tv->INVERTED);

	at.puts("\x1b[1;4;66m");
	CHECK_EQ(uint8(tv->attributes), tv->BOLD + tv->UNDERLINE + tv->DOUBLE_HEIGHT);

	at.puts("\x1b[1;4;0;3;7m");
	CHECK_EQ(tv->attributes, tv->ITALIC + tv->INVERTED);

	at.puts("\x1b[1;31;46m");
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, Color(vga::red));
	CHECK_EQ(tv->bgcolor, Color(vga::cyan));

	at.puts("\x1b[m");
	CHECK_EQ(tv->attributes, 0);
	CHECK_EQ(tv->fgcolor, tv->default_fgcolor);
	CHECK_EQ(tv->bgcolor, tv->default_bgcolor);

	at.sgr_cumulative = true;

	at.puts("\x1b[1;3;4;7m");
	at.puts("\x1b[m");
	CHECK_EQ(tv->attributes, 0);

	at.puts("\x1b[1m");
	CHECK_EQ(tv->attributes, tv->BOLD);

	at.puts("\x1b[4m");
	CHECK_EQ(tv->attributes, tv->BOLD + tv->UNDERLINE);

	at.puts("\x1b[66m");
	CHECK_EQ(tv->attributes, tv->BOLD + tv->UNDERLINE + tv->DOUBLE_HEIGHT);

	at.puts("\x1b[0;1;31;46m");
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, Color(vga::red));
	CHECK_EQ(tv->bgcolor, Color(vga::cyan));

	at.puts("\x1b[m");
	CHECK_EQ(tv->attributes, 0);
	CHECK_EQ(tv->fgcolor, tv->default_fgcolor);
	CHECK_EQ(tv->bgcolor, tv->default_bgcolor);

	at.puts("\x1b[;1m");
	CHECK_EQ(tv->attributes, tv->BOLD);

	at.puts("\x1b[;3m");
	CHECK_EQ(tv->attributes, tv->ITALIC);

	at.puts("\x1b[;4m");
	CHECK_EQ(tv->attributes, tv->UNDERLINE);

	at.puts("\x1b[;7m");
	CHECK_EQ(tv->attributes, tv->INVERTED);

	at.puts("\x1b[;21m");
	CHECK_EQ(tv->attributes, tv->UNDERLINE);
	at.puts("\x1b[4m");
	CHECK_EQ(tv->attributes, tv->UNDERLINE);

	at.puts("\x1b[30;47m");
	CHECK_EQ(tv->fgcolor, vga::black);
	CHECK_EQ(tv->bgcolor, vga::white);

	at.puts("\x1b[32;45m");
	CHECK_EQ(tv->fgcolor, vga::green);
	CHECK_EQ(tv->bgcolor, vga::magenta);

	at.puts("\x1b[39m");
	CHECK_EQ(tv->fgcolor, tv->default_fgcolor);
	CHECK_EQ(tv->bgcolor, vga::magenta);

	at.puts("\x1b[49m");
	CHECK_EQ(tv->fgcolor, tv->default_fgcolor);
	CHECK_EQ(tv->bgcolor, tv->default_bgcolor);

	at.puts("\x1b[38;5;10m");
	at.puts("\x1b[48;5;12m");
	CHECK_EQ(tv->fgcolor, vga::bright_green);
	CHECK_EQ(tv->bgcolor, vga::bright_blue);

	at.puts("\x1b[38;5;100m");
	at.puts("\x1b[48;5;200m");
	CHECK_EQ(tv->fgcolor, vga8_colors[100].raw);
	CHECK_EQ(tv->bgcolor, vga8_colors[200].raw);

	at.puts("\x1b[38;2;80;160;240m");
	at.puts("\x1b[48;2;4;16;64m");
	CHECK_EQ(tv->fgcolor, Color::fromRGB8(80, 160, 240).raw);
	CHECK_EQ(tv->bgcolor, Color::fromRGB8(4, 16, 64).raw);

	at.puts("\x1b[0;66m");
	CHECK_EQ(tv->attributes, tv->DOUBLE_HEIGHT);
	at.puts("\x1b[67m");
	CHECK_EQ(tv->attributes, tv->DOUBLE_WIDTH);
	at.puts("\x1b[0;68m");
	CHECK_EQ(tv->attributes, tv->DOUBLE_WIDTH + tv->DOUBLE_HEIGHT);
	at.puts("\x1b[1;69m");
	CHECK_EQ(tv->attributes, tv->BOLD);

	at.puts("\x1b[0;70m");
	CHECK_EQ(tv->attributes, tv->TRANSPARENT);

	at.puts("\x1b[90;107m");
	CHECK_EQ(tv->fgcolor, vga::bright_black);
	CHECK_EQ(tv->bgcolor, vga::bright_white);

	at.puts("\x1b[92;105m");
	CHECK_EQ(tv->fgcolor, vga::bright_green);
	CHECK_EQ(tv->bgcolor, vga::bright_magenta);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[22m");
	CHECK_EQ(tv->attributes, 0xff - tv->BOLD);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[23m");
	CHECK_EQ(tv->attributes, 0xff - tv->ITALIC);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[24m");
	CHECK_EQ(tv->attributes, 0xff - tv->UNDERLINE);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[25m");
	CHECK_EQ(tv->attributes, 0xff);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[27m");
	CHECK_EQ(tv->attributes, 0xff - tv->INVERTED);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[69m");
	CHECK_EQ(tv->attributes, 0xff - tv->DOUBLE_HEIGHT - tv->DOUBLE_WIDTH);

	tv->setCharAttributes(0xff);
	at.puts("\x1b[71m");
	CHECK_EQ(tv->attributes, 0xff - tv->TRANSPARENT);
}


// ################### PUSH CURSOR #############################


TEST_CASE("ESC 7  DECSC  and  ESC 8  DECRC")
{
	// DECSC: DEC save cursor
	// DECRC: DEC restore cursor

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);
	TextVDU*  tv = at.display;

	CHECK_EQ(tv->pixmap, pm);

	// +++ set a scroll region:
	at.top_margin	  = 5;
	at.bottom_margin  = 20;
	at.left_margin	  = 10;
	at.right_margin	  = 40;
	at.lr_set_by_csir = true;
	at.puts("\x1b[?6h"); // turn on region

	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_NE(at.display, tv);

	tv = at.display;

	CHECK_NE(tv->pixmap, pm);
	CHECK_EQ(tv->cols, 31);
	CHECK_EQ(tv->rows, 16);

	// +++ set cursor and attributes:
	tv->fgcolor = vga::red;
	tv->bgcolor = vga::green;
	tv->setCharAttributes(tv->ITALIC);
	tv->col = 33;
	tv->row = 22;

	// +++ other saved stuff:
	at.insert_mode	  = true;
	at.cursor_visible = true;

	at.putc(27), at.putc('7'); // push cursor

	tv = at.display;

	CHECK_EQ(at.insert_mode, false);
	CHECK_EQ(at.cursor_visible, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, 0);
	CHECK_EQ(at.display->cols, 50);
	CHECK_EQ(at.display->rows, 25);

	at.top_margin	 = 0;
	at.bottom_margin = 0;
	at.left_margin	 = 0;
	at.right_margin	 = 0;

	at.putc(27), at.putc('8'); // pop cursor

	CHECK_NE(at.display, tv);
	tv = at.display;

	CHECK_EQ(at.insert_mode, true);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_EQ(tv->col, 33);
	CHECK_EQ(tv->row, 22);
	CHECK_EQ(tv->attributes, tv->ITALIC);
	CHECK_EQ(at.display->cols, 31);
	CHECK_EQ(at.display->rows, 16);
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 20);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 40);

	at.putc(27), at.putc('8'); // pop cursor => soft reset

	CHECK_NE(at.display, tv);
	tv = at.display;

	CHECK_EQ(tv->pixmap, pm);

	CHECK_EQ(at.insert_mode, false);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, tv->NORMAL);
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(at.top_margin, 0);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.left_margin, 0);
	CHECK_EQ(at.right_margin, 0);
}

TEST_CASE("CSI s  SCOSC  and  CSI u  SCORC")
{
	// SCOSC: SCO save cursor
	// SCORC: SCO restore cursor

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	// the functionality of push / pop cursor was already tested for DECSC and DECRC

	at.display->col = 33;
	at.display->row = 22;
	at.display->setCharAttributes(8);

	at.puts("\x1b[s");

	CHECK_EQ(at.display->col, 0);
	CHECK_EQ(at.display->row, 0);
	CHECK_EQ(at.display->attributes, 0);

	at.puts("\x1b[u");

	CHECK_EQ(at.display->col, 33);
	CHECK_EQ(at.display->row, 22);
	CHECK_EQ(at.display->attributes, 8);
}


// ################### WINDOW #############################


TEST_CASE("CSI n ; m r  DECSTBM")
{
	// DEC set top and bottom margin of scroll region

	CanvasPtr	   pm = new Pixmap(400, 300);
	AnsiTerm	   at(pm);
	RCPtr<TextVDU> tv = at.display;

	tv->col = 10;
	tv->row = 5;
	tv->setCharAttributes(tv->BOLD);
	tv->fgcolor = vga::yellow;
	tv->bgcolor = vga::green;

	// set vertical scroll region boundaries
	// scroll region not yet enabled

	at.puts("\x1b[5;15r");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(tv.ptr(), at.display.ptr());
	CHECK_EQ(pm.ptr(), tv->pixmap.ptr());
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// enable scroll region set with CSI r

	at.puts("\x1b[?6h");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 11);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// disable scroll region

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[?6l");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	CHECK_EQ(tv->pixmap.ptr(), at.full_pixmap.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// enable invalid scroll region
	// => marked as enabled but scroll region not activated

	tv->col			 = 10;
	tv->row			 = 5;
	at.top_margin	 = 99;
	at.bottom_margin = 0;
	at.puts("\x1b[?6h");
	CHECK_EQ(at.top_margin, 99);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(at.display.ptr(), tv.ptr());
	CHECK_EQ(tv->pixmap.ptr(), pm.ptr());
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// set scroll region while enabled:

	at.puts("\x1b[5;15r");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 11);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[6;10r");
	CHECK_EQ(at.top_margin, 6);
	CHECK_EQ(at.bottom_margin, 10);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 5);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// check scroll region works as advertised:

	{
		using Pixmap	 = Graphics::Pixmap<colormode_a1w8_rgb>;
		RCPtr<Pixmap> pm = new Pixmap(80, 72, attrheight_12px); // 10*6
		AnsiTerm	  at(pm);
		tv = at.display;

		// print s.th. and use the scroll region:

		at.cursor_visible = false;
		tv->printChar('E', 60);
		CHECK_EQ(tv->row, 5);
		CHECK_EQ(tv->col, 10);
		CHECK_EQ(tv->scroll_count, 0);

		at.puts("\x1b[2;5r");
		at.puts("\x1b[?6h");
		tv = at.display;
		CHECK_EQ(tv->cols, 10);
		CHECK_EQ(tv->rows, 4);
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(at.cursor_visible, false);

		at.auto_wrap = false;
		at.puts("\x1b[5;20H");
		CHECK_EQ(tv->row, 3);
		CHECK_EQ(tv->col, 9);
		CHECK_EQ(tv->scroll_count, 0);
		tv->col = 0;
		tv->row = 0;

		at.puts("1234567890\n\r");
		CHECK_EQ(tv->row, 1);
		CHECK_EQ(tv->col, 0);
		at.puts("abcdefghij\n\r");
		at.puts("klmnopqrst\n\r");
		at.puts("ABCDEFGHIJ\n\r"); // scrolls
		CHECK_EQ(tv->row, 3);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->scroll_count, 1);
		at.puts("KLMNOPQRST\n\r"); // scrolls
		CHECK_EQ(tv->row, 3);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->scroll_count, 2);
		at.puts("\x1b[?25h"); // cursor on
		CHECK_EQ(at.cursor_visible, true);

		// construct reference:

		RCPtr<Pixmap> ref = new Pixmap(80, 72, attrheight_12px); // 10*6
		tv				  = new TextVDU(ref);
		tv->printChar('E', 10);
		tv->print("klmnopqrst");
		tv->print("ABCDEFGHIJ");
		tv->print("KLMNOPQRST");
		tv->printChar(' ', 10);
		tv->printChar('E', 10);
		tv->col = 0;
		tv->row = 4;
		tv->showCursor();

		CHECK_EQ(*pm, *ref);
	}
}

TEST_CASE("CSI n;n;n;n r  DECSTBM")
{
	// DEC set top, bottom, left and right margins	NON-STANDARD EXTENSION

	CanvasPtr	   pm = new Pixmap(400, 300);
	AnsiTerm	   at(pm);
	RCPtr<TextVDU> tv = at.display;

	tv->col = 10;
	tv->row = 5;
	tv->setCharAttributes(tv->BOLD);
	tv->fgcolor = vga::yellow;
	tv->bgcolor = vga::green;

	// set scroll region boundaries
	// scroll region not yet enabled

	at.puts("\x1b[5;15;10;40r");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 40);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_EQ(tv.ptr(), at.display.ptr());
	CHECK_EQ(pm.ptr(), tv->pixmap.ptr());
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// enable scroll region set with CSI r

	at.puts("\x1b[?6h");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 40);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 31);
	CHECK_EQ(tv->rows, 11);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// disable scroll region

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[?6l");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 40);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	CHECK_EQ(tv->pixmap.ptr(), at.full_pixmap.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// enable invalid scroll region
	// => marked as enabled but scroll region not activated

	tv->col			 = 10;
	tv->row			 = 5;
	at.top_margin	 = 99;
	at.bottom_margin = 0;
	at.puts("\x1b[?6h");
	CHECK_EQ(at.top_margin, 99);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 40);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_EQ(at.display.ptr(), tv.ptr());
	CHECK_EQ(tv->pixmap.ptr(), pm.ptr());
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// set scroll region while enabled:

	at.puts("\x1b[5;15;10;30r");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 30);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 21);
	CHECK_EQ(tv->rows, 11);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[6;10;10;40r");
	CHECK_EQ(at.top_margin, 6);
	CHECK_EQ(at.bottom_margin, 10);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 40);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 31);
	CHECK_EQ(tv->rows, 5);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:
	// set only top,bottom:

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[6;10r");
	CHECK_EQ(at.top_margin, 6);
	CHECK_EQ(at.bottom_margin, 10);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 40);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true); // remains true, could be set by DECSLRM
	CHECK_EQ(at.lr_set_by_csir, true);	   //
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 31);
	CHECK_EQ(tv->rows, 5);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:
	// set only left,right:

	tv->col = 8;
	tv->row = 3;
	at.puts("\x1b[;;20;30r");
	CHECK_EQ(at.top_margin, 6);
	CHECK_EQ(at.bottom_margin, 10);
	CHECK_EQ(at.left_margin, 20);
	CHECK_EQ(at.right_margin, 30);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 11);
	CHECK_EQ(tv->rows, 5);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:
	// use default values:

	tv->col = 8;
	tv->row = 3;
	at.puts("\x1b[;15;;40r");
	CHECK_EQ(at.top_margin, 0);
	CHECK_EQ(at.bottom_margin, 15);
	CHECK_EQ(at.left_margin, 0);
	CHECK_EQ(at.right_margin, 40);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 40);
	CHECK_EQ(tv->rows, 15);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:
	// use default values:

	tv->col = 8;
	tv->row = 3;
	at.puts("\x1b[5;;10;r");
	CHECK_EQ(at.top_margin, 5);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.left_margin, 10);
	CHECK_EQ(at.right_margin, 0);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 41);
	CHECK_EQ(tv->rows, 21);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:
	// set to full window size:

	tv->col = 8;
	tv->row = 3;
	at.puts("\x1b[r");	 // reset top,bottom
	at.puts("\x1b[;;r"); // reset left,right
	CHECK_EQ(at.top_margin, 0);
	CHECK_EQ(at.bottom_margin, 0);
	CHECK_EQ(at.left_margin, 0);
	CHECK_EQ(at.right_margin, 0);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, true);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, true);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// check scroll region works as advertised:

	{
		using Pixmap	 = Graphics::Pixmap<colormode_a1w8_rgb>;
		RCPtr<Pixmap> pm = new Pixmap(80, 72, attrheight_12px); // 10*6
		AnsiTerm	  at(pm);
		tv = at.display;

		// print s.th. and use the scroll region:

		at.cursor_visible = false;
		tv->printChar('E', 60);
		CHECK_EQ(tv->row, 5);
		CHECK_EQ(tv->col, 10);
		CHECK_EQ(tv->scroll_count, 0);

		at.puts("\x1b[2;5;3;8r");
		at.puts("\x1b[?6h");
		tv = at.display;
		CHECK_EQ(tv->cols, 6);
		CHECK_EQ(tv->rows, 4);
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(at.cursor_visible, false);

		at.auto_wrap = false;
		at.puts("\x1b[5;20H");
		CHECK_EQ(tv->row, 3);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->scroll_count, 0);
		tv->col = 0;
		tv->row = 0;

		at.puts("123456\n\r");
		CHECK_EQ(tv->row, 1);
		CHECK_EQ(tv->col, 0);
		at.puts("abcdef\n\r");
		at.puts("klmnop\n\r");
		at.puts("ABCDEF\n\r"); // scrolls
		CHECK_EQ(tv->row, 3);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->scroll_count, 1);
		at.puts("KLMNOP\n\r"); // scrolls
		CHECK_EQ(tv->row, 3);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->scroll_count, 2);
		at.puts("\x1b[?25h"); // cursor on
		CHECK_EQ(at.cursor_visible, true);

		// construct reference:

		RCPtr<Pixmap> ref = new Pixmap(80, 72, attrheight_12px); // 10*6
		tv				  = new TextVDU(ref);
		tv->printChar('E', 10);
		tv->print("EEklmnopEE");
		tv->print("EEABCDEFEE");
		tv->print("EEKLMNOPEE");
		tv->print("EE      EE");
		tv->printChar('E', 10);
		tv->col = 2;
		tv->row = 4;
		tv->showCursor();

		CHECK_EQ(*pm, *ref);
	}
}

TEST_CASE("CSI n ; m s  DECSLRM")
{
	// DEC set left and right margins

	CanvasPtr	   pm = new Pixmap(400, 300);
	AnsiTerm	   at(pm);
	RCPtr<TextVDU> tv = at.display;

	tv->col = 10;
	tv->row = 5;
	tv->setCharAttributes(tv->BOLD);
	tv->fgcolor = vga::yellow;
	tv->bgcolor = vga::green;

	// set horizontal scroll region boundaries
	// scroll region not yet enabled

	at.puts("\x1b[5;15s");
	CHECK_EQ(at.left_margin, 5);
	CHECK_EQ(at.right_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(tv.ptr(), at.display.ptr());
	CHECK_EQ(pm.ptr(), tv->pixmap.ptr());
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// enable scroll region set with CSI s

	at.puts("\x1b[?69h");
	CHECK_EQ(at.left_margin, 5);
	CHECK_EQ(at.right_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 11);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// disable scroll region

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[?69l");
	CHECK_EQ(at.left_margin, 5);
	CHECK_EQ(at.right_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, false);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	CHECK_EQ(tv->pixmap.ptr(), at.full_pixmap.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 50);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// enable invalid scroll region
	// => marked as enabled but scroll region not activated

	tv->col			= 10;
	tv->row			= 5;
	at.left_margin	= 99;
	at.right_margin = 0;
	at.puts("\x1b[?69h");
	CHECK_EQ(at.left_margin, 99);
	CHECK_EQ(at.right_margin, 0);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_EQ(at.display.ptr(), tv.ptr());
	CHECK_EQ(tv->pixmap.ptr(), pm.ptr());
	CHECK_EQ(tv->col, 10);
	CHECK_EQ(tv->row, 5);
	CHECK_EQ(tv->attributes, tv->BOLD);
	CHECK_EQ(tv->fgcolor, vga::yellow);
	CHECK_EQ(tv->bgcolor, vga::green);

	// set scroll region while enabled:

	at.puts("\x1b[5;15s");
	CHECK_EQ(at.left_margin, 5);
	CHECK_EQ(at.right_margin, 15);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 11);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// change scroll region while enabled:

	tv->col = 10;
	tv->row = 5;
	at.puts("\x1b[6;10s");
	CHECK_EQ(at.left_margin, 6);
	CHECK_EQ(at.right_margin, 10);
	CHECK_EQ(at.cursor_visible, true);
	CHECK_EQ(at.display->cursorVisible, true);
	CHECK_EQ(at.tb_margins_enabled, false);
	CHECK_EQ(at.lr_margins_enabled, true);
	CHECK_EQ(at.lr_set_by_csir, false);
	CHECK_NE(at.display.ptr(), tv.ptr());
	tv = at.display;
	CHECK_NE(tv->pixmap.ptr(), pm.ptr());
	pm = tv->pixmap;
	CHECK_EQ(tv->cols, 5);
	CHECK_EQ(tv->rows, 25);
	CHECK_EQ(tv->col, 0);
	CHECK_EQ(tv->row, 0);

	// check scroll region works as advertised:

	{
		using Pixmap	 = Graphics::Pixmap<colormode_a1w8_rgb>;
		RCPtr<Pixmap> pm = new Pixmap(80, 72, attrheight_12px); // 10*6
		AnsiTerm	  at(pm);
		tv = at.display;

		// print s.th. and use the scroll region:

		at.cursor_visible = false;
		tv->printChar('E', 60);
		CHECK_EQ(tv->row, 5);
		CHECK_EQ(tv->col, 10);
		CHECK_EQ(tv->scroll_count, 0);

		at.puts("\x1b[3;8s");
		at.puts("\x1b[?69h");
		tv = at.display;
		CHECK_EQ(tv->cols, 6);
		CHECK_EQ(tv->rows, 6);
		CHECK_EQ(tv->row, 0);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(at.cursor_visible, false);

		at.auto_wrap = false;
		at.puts("\x1b[20;20H");
		CHECK_EQ(tv->row, 5);
		CHECK_EQ(tv->col, 5);
		CHECK_EQ(tv->scroll_count, 0);
		tv->col = 0;
		tv->row = 0;

		at.puts("123456\n\r");
		CHECK_EQ(tv->row, 1);
		CHECK_EQ(tv->col, 0);
		at.puts("abcdef\n\r");
		at.puts("klmnop\n\r");
		at.puts("ABCDEF\n\r");
		CHECK_EQ(tv->row, 4);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->scroll_count, 0);
		at.puts("KLMNOP\n\r");
		CHECK_EQ(tv->row, 5);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->scroll_count, 0);
		at.puts("UVWXYZ\n\r"); // scrolls
		CHECK_EQ(tv->row, 5);
		CHECK_EQ(tv->col, 0);
		CHECK_EQ(tv->scroll_count, 1);
		at.puts("\x1b[?25h"); // cursor on
		CHECK_EQ(at.cursor_visible, true);

		// construct reference:

		RCPtr<Pixmap> ref = new Pixmap(80, 72, attrheight_12px); // 10*6
		tv				  = new TextVDU(ref);
		tv->print("EEabcdefEE");
		tv->print("EEklmnopEE");
		tv->print("EEABCDEFEE");
		tv->print("EEKLMNOPEE");
		tv->print("EEUVWXYZEE");
		tv->print("EE      EE");
		tv->col = 2;
		tv->row = 5;
		tv->showCursor();

		CHECK_EQ(*pm, *ref);
	}
}


// ################### MOUSE #############################


TEST_CASE("CSI	n… ' w  DECEFR")
{
	// DEC enable filter rectangle (mouse fence)

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	CHECK_EQ(at.mouse_enable_rect, false);
	at.puts("\x1b[10;5;20;40'w");
	CHECK_EQ(at.mouse_rect.top(), 10 - 1);
	CHECK_EQ(at.mouse_rect.left(), 5 - 1);
	CHECK_EQ(at.mouse_rect.bottom(), 20 - 1 + 1);
	CHECK_EQ(at.mouse_rect.right(), 40 - 1 + 1);
	CHECK_EQ(at.mouse_enable_rect, true);

	using namespace USB;
	using namespace USB::Mock;

	setMousePresent();
	setMouseLimits(400, 300); // also set mouse to 400/2,300/3 = 200,100 ~ 25,8

	at.puts("\x1b['w");
	CHECK_EQ(at.mouse_rect.top(), 8);
	CHECK_EQ(at.mouse_rect.left(), 25);
	CHECK_EQ(at.mouse_rect.bottom(), 9);
	CHECK_EQ(at.mouse_rect.right(), 26);
	CHECK(at.mouse_rect.contains(Point(25, 8)));
	CHECK_FALSE(at.mouse_rect.contains(Point(24, 8)));
	CHECK_FALSE(at.mouse_rect.contains(Point(26, 8)));
	CHECK_FALSE(at.mouse_rect.contains(Point(25, 7)));
	CHECK_FALSE(at.mouse_rect.contains(Point(25, 9)));

	at.mouse_rect = {0, 0, 0, 0};
	at.puts("\x1b[;;20;40'w");
	CHECK_EQ(at.mouse_rect.top(), 8);
	CHECK_EQ(at.mouse_rect.left(), 25);
	CHECK_EQ(at.mouse_rect.bottom(), 20);
	CHECK_EQ(at.mouse_rect.right(), 40);

	at.mouse_rect = {0, 0, 0, 0};
	at.puts("\x1b[2;4;;'w");
	CHECK_EQ(at.mouse_rect.top(), 2 - 1);
	CHECK_EQ(at.mouse_rect.left(), 4 - 1);
	CHECK_EQ(at.mouse_rect.bottom(), 8 + 1);
	CHECK_EQ(at.mouse_rect.right(), 25 + 1);
}

TEST_CASE("CSI	n;m ' z  DECELR")
{
	// DEC enable locator (mouse) reports
	//	n = 0  ⇒  Mouse reports disabled (default).
	//	  = 1  ⇒  Mouse reports enabled.
	//    = 2  ⇒  Mouse reports enabled for one report, then disabled.
	//	m = 0  ⇒  character cells (default).
	//    = 1  ⇒  device physical pixels.
	//    = 2  ⇒  character cells.

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	using namespace USB;
	using namespace USB::Mock;

	setMousePresent();
	setMouseLimits(400, 300); // also set mouse to 400/2,300/3 = 200,100 ~ 25,8

	CHECK_EQ(at.mouse_enabled, false);
	CHECK_EQ(at.mouse_enable_rect, false);
	CHECK_EQ(at.mouse_enabled_once, false);
	CHECK_EQ(at.mouse_report_btn_down, false);
	CHECK_EQ(at.mouse_report_btn_up, false);
	CHECK_EQ(at.mouse_report_pixels, false);

	at.puts("\x1b[2;1'z");

	CHECK_EQ(at.mouse_enabled, true);
	CHECK_EQ(at.mouse_enable_rect, false);
	CHECK_EQ(at.mouse_enabled_once, true);
	CHECK_EQ(at.mouse_report_btn_down, false);
	CHECK_EQ(at.mouse_report_btn_up, false);
	CHECK_EQ(at.mouse_report_pixels, true);

	at.mouse_enable_rect = true;
	at.puts("\x1b[1'z");

	CHECK_EQ(at.mouse_enabled, true);
	CHECK_EQ(at.mouse_enable_rect, false);
	CHECK_EQ(at.mouse_enabled_once, false);
	CHECK_EQ(at.mouse_report_btn_down, false);
	CHECK_EQ(at.mouse_report_btn_up, false);
	CHECK_EQ(at.mouse_report_pixels, false);
}

TEST_CASE("CSI n… ' {  DECSLE")
{
	// Select Locator Events
	// multiple arguments possible.
	// n = 0  ⇒  only respond to explicit host requests (DECRQLP).
	//			  This is default. It also cancels any filter rectangle.
	//     1  ⇒  report button down transitions.
	//     2  ⇒  do not report button down transitions.
	//     3  ⇒  report button up transitions.
	//     4  ⇒  do not report button up transitions.

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	using namespace USB;
	using namespace USB::Mock;

	setMousePresent();
	setMouseLimits(400, 300); // also set mouse to 400/2,300/3 = 200,100 ~ 25,8

	at.puts("\x1b[1'{");
	CHECK_EQ(at.mouse_enabled, false);
	CHECK_EQ(at.mouse_enable_rect, false);
	CHECK_EQ(at.mouse_enabled_once, false);
	CHECK_EQ(at.mouse_report_btn_down, true);
	CHECK_EQ(at.mouse_report_btn_up, false);
	CHECK_EQ(at.mouse_report_pixels, false);

	at.puts("\x1b[3'{");
	CHECK_EQ(at.mouse_report_btn_down, true);
	CHECK_EQ(at.mouse_report_btn_up, true);

	at.mouse_enable_rect = true;
	at.puts("\x1b['{");
	CHECK_EQ(at.mouse_report_btn_down, false);
	CHECK_EQ(at.mouse_report_btn_up, false);

	at.puts("\x1b[1'{");
	CHECK_EQ(at.mouse_report_btn_down, true);
	CHECK_EQ(at.mouse_report_btn_up, false);
	at.puts("\x1b[2;3'{");
	CHECK_EQ(at.mouse_report_btn_down, false);
	CHECK_EQ(at.mouse_report_btn_up, true);
	at.puts("\x1b[4;1'{");
	CHECK_EQ(at.mouse_report_btn_down, true);
	CHECK_EQ(at.mouse_report_btn_up, false);
}

TEST_CASE("CSI n ' |  DECRQLP")
{
	// DEC request locator (mouse) position & state
	// --> Mouse Report:  CSI event ; buttons ; row ; col ; page & w
	// 	   event = 0: locator unavailable.
	// 	   event = 1: response to a DECRQLP request.

	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	using namespace USB;
	using namespace USB::Mock;

	auto response = [&]() {
		static char bu[80];
		int			c, i = 0;
		while ((c = at.getc()) >= 0) { bu[i++] = char(c); }
		bu[i] = 0;
		return std::string(escapedstr(bu));
	};

	setMouseLimits(400, 300); // also set mouse to 400/2,300/3 = 200,100 ~ 25,8

	at.mouse_enabled = false;
	at.puts("\x1b['|");
	CHECK_EQ(response(), ""); // no response if mouse not enabled

	at.mouse_enabled = true;
	setMousePresent(false);
	at.puts("\x1b['|");
	CHECK_EQ(response(), "\\033[0&w"); // no mouse present
	CHECK_EQ(at.mouse_enabled, true);

	at.mouse_enabled	  = true;
	at.mouse_enabled_once = true;
	setMousePresent();
	at.puts("\x1b['|");
	CHECK_EQ(response(), "\\033[1;0;9;26&w");
	CHECK_EQ(at.mouse_enabled, false);
}

TEST_CASE("AnsiTerm: getc() special keys")
{
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	auto response = [&]() {
		char bu[80];
		int	 c, i = 0;
		while ((c = at.getc()) >= 0) { bu[i++] = char(c); }
		bu[i] = 0;
		return std::string(escapedstr(bu));
	};

	using namespace USB;
	using namespace USB::Mock;

	at.application_mode = false;
	at.c1_codes_8bit	= false;
	at.utf8_mode		= false;
	setHidKeyTranslationTable(key_table_ger);

	CHECK_EQ(at.getc(), -1);

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\033[A");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_DOWN));
	CHECK_EQ(response(), "\\033[B");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_RIGHT));
	CHECK_EQ(response(), "\\033[C");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_LEFT));
	CHECK_EQ(response(), "\\033[D");

	at.c1_codes_8bit = true;
	at.utf8_mode	 = false;
	addKeyboardReport(keys(NO_MODIFIERS));
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\233A");
	CHECK_EQ(at.getc(), -1);

	at.c1_codes_8bit = false;
	at.utf8_mode	 = true;
	addKeyboardReport(keys(NO_MODIFIERS));
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\033[A");
	CHECK_EQ(at.getc(), -1);

	at.c1_codes_8bit = true;
	at.utf8_mode	 = true;
	addKeyboardReport(keys(NO_MODIFIERS));
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\302\\233A"); // c2 9b A
	CHECK_EQ(at.getc(), -1);

	at.c1_codes_8bit = false;
	at.utf8_mode	 = false;

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_HOME));
	CHECK_EQ(response(), "\\033[1~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_INSERT));
	CHECK_EQ(response(), "\\033[2~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_DELETE));
	CHECK_EQ(response(), "\\033[3~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_END));
	CHECK_EQ(response(), "\\033[4~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_PAGE_UP));
	CHECK_EQ(response(), "\\033[5~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_PAGE_DOWN));
	CHECK_EQ(response(), "\\033[6~");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F1));
	CHECK_EQ(response(), "\\033OP");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F2));
	CHECK_EQ(response(), "\\033OQ");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F3));
	CHECK_EQ(response(), "\\033OR");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F4));
	CHECK_EQ(response(), "\\033OS");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F5));
	CHECK_EQ(response(), "\\033[15~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F6));
	CHECK_EQ(response(), "\\033[17~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F7));
	CHECK_EQ(response(), "\\033[18~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F8));
	CHECK_EQ(response(), "\\033[19~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F9));
	CHECK_EQ(response(), "\\033[20~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F10));
	CHECK_EQ(response(), "\\033[21~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F11));
	CHECK_EQ(response(), "\\033[23~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F12));
	CHECK_EQ(response(), "\\033[24~");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_MULTIPLY));
	CHECK_EQ(response(), "*");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_ADD));
	CHECK_EQ(response(), "+");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_COMMA)); // keycode 0x85/133 => not in table
	CHECK_EQ(at.getc(), -1);
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_SUBTRACT));
	CHECK_EQ(response(), "-");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_DECIMAL));
	CHECK_EQ(response(), ".");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_DIVIDE));
	CHECK_EQ(response(), "/");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_0));
	CHECK_EQ(response(), "0");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_1));
	CHECK_EQ(response(), "1");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_2));
	CHECK_EQ(response(), "2");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_3));
	CHECK_EQ(response(), "3");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_4));
	CHECK_EQ(response(), "4");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_5));
	CHECK_EQ(response(), "5");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_6));
	CHECK_EQ(response(), "6");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_7));
	CHECK_EQ(response(), "7");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_8));
	CHECK_EQ(response(), "8");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_9));
	CHECK_EQ(response(), "9");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_ENTER));
	CHECK_EQ(response(), "\\r");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_EQUAL));
	CHECK_EQ(response(), "=");

	at.application_mode = true;

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\033OA");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_DOWN));
	CHECK_EQ(response(), "\\033OB");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_RIGHT));
	CHECK_EQ(response(), "\\033OC");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_LEFT));
	CHECK_EQ(response(), "\\033OD");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_HOME));
	CHECK_EQ(response(), "\\033[1~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_INSERT));
	CHECK_EQ(response(), "\\033[2~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_DELETE));
	CHECK_EQ(response(), "\\033[3~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_END));
	CHECK_EQ(response(), "\\033[4~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_PAGE_UP));
	CHECK_EQ(response(), "\\033[5~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_PAGE_DOWN));
	CHECK_EQ(response(), "\\033[6~");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F1));
	CHECK_EQ(response(), "\\033OP");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F2));
	CHECK_EQ(response(), "\\033OQ");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F3));
	CHECK_EQ(response(), "\\033OR");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F4));
	CHECK_EQ(response(), "\\033OS");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F5));
	CHECK_EQ(response(), "\\033[15~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F6));
	CHECK_EQ(response(), "\\033[17~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F7));
	CHECK_EQ(response(), "\\033[18~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F8));
	CHECK_EQ(response(), "\\033[19~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F9));
	CHECK_EQ(response(), "\\033[20~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F10));
	CHECK_EQ(response(), "\\033[21~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F11));
	CHECK_EQ(response(), "\\033[23~");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_F12));
	CHECK_EQ(response(), "\\033[24~");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_MULTIPLY));
	CHECK_EQ(response(), "\\033Oj");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_ADD));
	CHECK_EQ(response(), "\\033Ok");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_COMMA)); // keycode 0x85/133 => not in table
	CHECK_EQ(at.getc(), -1);
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_SUBTRACT));
	CHECK_EQ(response(), "\\033Om");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_DECIMAL));
	CHECK_EQ(response(), "\\033On");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_DIVIDE));
	CHECK_EQ(response(), "\\033Oo");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_0));
	CHECK_EQ(response(), "\\033Op");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_1));
	CHECK_EQ(response(), "\\033Oq");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_2));
	CHECK_EQ(response(), "\\033Or");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_3));
	CHECK_EQ(response(), "\\033Os");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_4));
	CHECK_EQ(response(), "\\033Ot");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_5));
	CHECK_EQ(response(), "\\033Ou");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_6));
	CHECK_EQ(response(), "\\033Ov");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_7));
	CHECK_EQ(response(), "\\033Ow");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_8));
	CHECK_EQ(response(), "\\033Ox");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_9));
	CHECK_EQ(response(), "\\033Oy");

	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_ENTER));
	CHECK_EQ(response(), "\\033OM");
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_KEYPAD_EQUAL));
	CHECK_EQ(response(), "\\033OX");

	at.c1_codes_8bit = true;
	at.utf8_mode	 = false;
	addKeyboardReport(keys(NO_MODIFIERS));
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\217A");
	CHECK_EQ(at.getc(), -1);

	at.c1_codes_8bit = false;
	at.utf8_mode	 = true;
	addKeyboardReport(keys(NO_MODIFIERS));
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\033OA");
	CHECK_EQ(at.getc(), -1);

	at.c1_codes_8bit = true;
	at.utf8_mode	 = true;
	addKeyboardReport(keys(NO_MODIFIERS));
	addKeyboardReport(keys(NO_MODIFIERS, USB::KEY_ARROW_UP));
	CHECK_EQ(response(), "\\302\\217A"); // c2 8f A
	CHECK_EQ(at.getc(), -1);

	at.c1_codes_8bit = false;
	at.utf8_mode	 = false;
	addKeyboardReport(keys(NO_MODIFIERS));
	CHECK_EQ(at.getc(), -1);
}

TEST_CASE("AnsiTerm: getc() mouse reports")
{
	CanvasPtr pm = new Pixmap(400, 300);
	AnsiTerm  at(pm);

	auto response = [&]() {
		char bu[80];
		int	 c, i = 0;
		while ((c = at.getc()) >= 0) { bu[i++] = char(c); }
		bu[i] = 0;
		return std::string(escapedstr(bu));
	};

	using namespace USB;
	using namespace USB::Mock;

	setMouseLimits(400, 300); // also set mouse to 400/2,300/3 = 200,100 ~ 25,8
	at.application_mode = false;
	at.c1_codes_8bit	= false;
	at.utf8_mode		= false;
	setHidKeyTranslationTable(key_table_ger);
	CHECK_EQ(USB::getMousePosition(), Point(200, 100));

	at.puts("\x1b[1'z"); // enable mouse reports
	CHECK_EQ(at.getc(), -1);
	CHECK_EQ(at.mouse_enabled, true);
	CHECK_EQ(at.mouse_report_pixels, false);
	CHECK_EQ(at.mouse_enabled_once, false);
	CHECK_EQ(at.mouse_enable_rect, false);
	CHECK_EQ(at.mouse_report_btn_down, false);
	CHECK_EQ(at.mouse_report_btn_up, false);

	addMouseReport(mouse());
	CHECK_EQ(at.getc(), -1);

	addMouseReport(mouse(0x1f, 10, 10, 1, 1));
	pollUSB(); // process it
	CHECK_EQ(USB::getMousePosition(), Point(210, 110));
	CHECK_EQ(at.getc(), -1); // but we throw away the msg!

	addMouseReport(mouse(0, 11, 12));
	pollUSB(); // process it
	CHECK_EQ(USB::getMousePosition(), Point(221, 122));
	at.puts("\x1b['|"); // request report
	CHECK_EQ(response(), "\\033[1;0;11;28&w");
	at.puts("\x1b['|"); // request report
	CHECK_EQ(response(), "\\033[1;0;11;28&w");

	at.puts("\x1b[1;1'z"); // enable mouse reports, pixels
	at.puts("\x1b['|");	   // request report
	CHECK_EQ(response(), "\\033[1;0;123;222&w");

	at.puts("\x1b[1;3'{"); // report button up and down
	CHECK_EQ(at.getc(), -1);

	addMouseReport(mouse(0, -10, -10));
	CHECK_EQ(at.getc(), -1); // process it
	CHECK_EQ(USB::getMousePosition(), Point(211, 112));
	CHECK_EQ(at.getc(), -1); // pure movement => no msg

	addMouseReport(mouse(USB::LEFT_BUTTON, 1, 0));
	pollUSB(); // process it
	CHECK_EQ(USB::getMousePosition(), Point(212, 112));
	CHECK_EQ(response(), "\\033[2;4;113;213&w");

	addMouseReport(mouse(RIGHT_BUTTON, 1, 0));
	pollUSB(); // process it
	CHECK_EQ(getMousePosition(), Point(213, 112));
	CHECK_EQ(
		response(),
		"\\033[3;0;113;214&w"	// left button up
		"\\033[6;1;113;214&w"); // right button down

	at.puts("\x1b['w"); // filter rect around mouse position  // CSI t;l;b;r ' w
	CHECK_EQ(at.getc(), -1);

	addMouseReport(mouse(RIGHT_BUTTON + MIDDLE_BUTTON, 1, 0));
	pollUSB(); // process it
	CHECK_EQ(getMousePosition(), Point(214, 112));
	CHECK_EQ(
		response(),
		"\\033[10;1;113;215&w"	// mouse moved out of fence
		"\\033[4;3;113;215&w"); // middle button down

	addMouseReport(mouse(MIDDLE_BUTTON + FORWARD_BUTTON, 1, 0)); //right up m4 down
	pollUSB();													 // process it
	CHECK_EQ(getMousePosition(), Point(215, 112));
	CHECK_EQ(
		response(),				 // no report on movement because 'fence' is a one-shot
		"\\033[7;2;113;216&w"	 // right button up
		"\\033[8;10;113;216&w"); // M4 button down

	at.puts("\x1b[4'{"); // do not report button up
	CHECK_EQ(at.getc(), -1);

	at.puts("\x1b[100;200;120;220'w"); // filter rect around mouse position  // CSI t;l;b;r ' w
	CHECK_EQ(at.getc(), -1);

	addMouseReport(mouse(LEFT_BUTTON + FORWARD_BUTTON, 1, 2)); //left down
	pollUSB();												   // process it
	CHECK_EQ(getMousePosition(), Point(216, 114));
	// still in fence => no report on movement, no report on button up:
	CHECK_EQ(response(), "\\033[2;12;115;217&w"); // left button down

	addMouseReport(mouse(LEFT_BUTTON, 3, 2)); // fwd btn up
	pollUSB();								  // process it
	CHECK_EQ(getMousePosition(), Point(219, 116));
	CHECK_EQ(at.getc(), -1);

	addMouseReport(mouse(0, 1, 0)); // left btn up
	pollUSB();						// process it
	CHECK_EQ(getMousePosition(), Point(220, 116));
	CHECK_EQ(response(), "\\033[10;0;117;221&w"); // left fence, all buttons up
}


TEST_CASE("CSI n b  REP")
{
	// repeat preceding char n times		NIMP
}

TEST_CASE("CSI n Y  CVT")
{
	// cursor vertical tabulation			NIMP
}

} // namespace kio::Test


/*




































*/
