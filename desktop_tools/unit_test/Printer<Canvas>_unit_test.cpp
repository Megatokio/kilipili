// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Graphics/Color.h"
#include "Graphics/Pixmap_wAttr.h"
#include "Graphics/Printer<Canvas>.h"
#include "Mock/MockPixmap.h"
#include "doctest.h"

using namespace kilipili;

using TextVDU = Printer<Canvas>;
//static_assert(TextVDU::CHAR_HEIGHT == 12);
static_assert(TextVDU::CHAR_WIDTH == 8);

// defined in main_unit_test.cpp:

namespace kilipili
{
extern doctest::String toString(Array<cstr>);
}
namespace kilipili::Graphics
{
extern doctest::String toString(ColorMode);
}


namespace kilipili::Test
{

using Pixmap	 = kilipili::Mock::Pixmap;
using RealPixmap = kilipili::Pixmap_a1w8;


TEST_CASE("Printer<Canvas>: constructor")
{
	RCPtr		pm = new Pixmap(80, 60);
	TextVDU		tv(pm);
	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)";
	CHECK_EQ(pm->log, ref);

	CHECK_EQ(Color(tv.default_bgcolor).raw, Color::uRGB(-1)); // white
	CHECK_EQ(Color(tv.default_fgcolor).raw, 0x0000u);		  // black
	CHECK_EQ(tv.pixmap.ptr(), pm.ptr());
	CHECK_EQ(tv.pixmap->colormode, uint(colormode_a1w8));
	CHECK_EQ(tv.pixmap->attrheight, 12);
	CHECK_EQ(tv.pixmap->attrwidth, 8);
	//CHECK_EQ(tv.pixmap->colordepth(), msbit(sizeof(Color) * CHAR_BIT)); //log2(16)
	//CHECK_EQ(tv.pixmap->attrmode(), attrmode_1bpp);
	//CHECK_EQ(tv.pixmap->bits_per_color(), sizeof(Color) * CHAR_BIT); // 16
	CHECK_EQ(tv.pixmap->bits_per_pixel, 1);
	CHECK_EQ(tv.cols, 80 / 8);
	CHECK_EQ(tv.rows, 60 / 12);
	CHECK_EQ(tv.bgcolor, tv.default_bgcolor);
	CHECK_EQ(tv.fgcolor, tv.default_fgcolor);
	CHECK_EQ(tv.fg_ink, 1);
	CHECK_EQ(tv.bg_ink, 0);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.dx, 1);
	CHECK_EQ(tv.dy, 1);
	CHECK_EQ(tv.getAttributes(), 0);
	CHECK_EQ(tv.cursor_visible, false);
}

TEST_CASE("Printer<Canvas>: showCursor(), hideCursor()")
{
	RCPtr	pm = new Pixmap(80, 60);
	TextVDU tv(pm);
	{
		tv.cls();
		CHECK_EQ(tv.cursor_visible, false);
		tv.showCursor(true);
		CHECK_EQ(tv.cursor_visible, true);
		tv.hideCursor();
		CHECK_EQ(tv.cursor_visible, false);

		Array<cstr> ref;
		ref << "Pixmap(80,60,a1w8,12)";	  // ctor
		ref << "clear(65535)";			  // cls
		ref << "xorRect(0,0,8,12,65535)"; // show cursor
		ref << "xorRect(0,0,8,12,65535)"; // hide cursor
		CHECK_EQ(pm->log, ref);
	}
	{
		pm->log.purge();
		tv.showCursor(true);
		tv.moveTo(3, 4); // row,col
		tv.showCursor(true);

		Array<cstr> ref;
		ref << "xorRect(0,0,8,12,65535)";	// show cursor
		ref << "xorRect(0,0,8,12,65535)";	// hide cursor before moveTo()
		ref << "xorRect(32,36,8,12,65535)"; // show cursor
		CHECK_EQ(pm->log, ref);				// TODO: CHECK_EQ was missing?
	}
}

TEST_CASE("Printer<Canvas>: reset()")
{
	RCPtr	pm = new Pixmap(80, 60);
	TextVDU tv(pm);

	tv.setAttributes(tv.bold | tv.inverted | tv.double_width | tv.double_height);
	tv.bgcolor = 1234;
	tv.fgcolor = 2345;
	tv.moveTo(5, 7);
	tv.reset();

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)";
	CHECK_EQ(pm->log, ref);

	CHECK_EQ(tv.bgcolor, tv.default_bgcolor);
	CHECK_EQ(tv.fgcolor, tv.default_fgcolor);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.dx, 1);
	CHECK_EQ(tv.dy, 1);
	CHECK_EQ(tv.getAttributes(), 0);
	CHECK_EQ(tv.cursor_visible, false);
}

TEST_CASE("Printer<Canvas>: cls()")
{
	RCPtr<RealPixmap> pm = new RealPixmap(80, 60, attrheight_12px);
	TextVDU			  tv(pm);
	constexpr uint	  BGCOLOR = Color::uRGB(12345678);
	constexpr uint	  FGCOLOR = Color::uRGB(23456789);
	tv.bgcolor				  = BGCOLOR;
	tv.fgcolor				  = FGCOLOR;
	tv.printChar('E', 80 / 8 * 60 / 12);
	tv.cls();
	bool ink_ok	  = true;
	bool color_ok = true;
	for (coord x = 0; x < 80; x++)
		for (coord y = 0; y < 60; y++)
		{
			if (pm->getColor(x, y) != BGCOLOR) color_ok = false;
			if (pm->getInk(x, y) != 0) ink_ok = false;
		}
	CHECK_EQ(ink_ok, true);
	CHECK_EQ(color_ok, true);
}

TEST_CASE("Printer<Canvas>: identify()")
{
	RCPtr<RealPixmap> pm1 = new RealPixmap(800, 60, attrheight_12px);
	RCPtr<RealPixmap> pm2 = new RealPixmap(800, 60, attrheight_12px);

	TextVDU tv1(pm1);
	//debugstr("%s\n", tv1.identify());
	tv1.cls();
	//tv1.printf("%s\n", tv1.identify());
	//tv1.showCursor();

	TextVDU tv2(pm2);
	tv2.cls();
	//tv2.print("size=800*60, text=100*5, char=8*12, colors=rgb, attr=8*12");
	//tv2.newLine();
	//tv2.showCursor();

	CHECK_EQ(*pm1, *pm2);

	CHECK_EQ(tv1.col, tv2.col);
	CHECK_EQ(tv1.row, tv2.row);
	CHECK_EQ(pm1->width, 800);
	CHECK_EQ(pm1->height, 60);
	CHECK_EQ(tv1.cols, 100);
	CHECK_EQ(tv1.rows, 5);
	CHECK_EQ(tv1.pixmap->attrwidth, 8);
	CHECK_EQ(tv1.pixmap->attrheight, 12);
	CHECK_EQ(tv1.pixmap->bits_per_pixel, 1);
	//CHECK_EQ(tv1.pixmap->colordepth(), msbit(sizeof(Color) * CHAR_BIT)); // log2(16)
}

TEST_CASE("Printer<Canvas>: moveTo()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.col, 0);
	tv.moveTo(3, 2);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.col, 2);
	tv.moveTo(4, 8);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.col, 8);
	tv.moveTo(3, 20, tv.nowrap);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.col, 9);
	tv.moveTo(2, 10, tv.wrap);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 10);
	tv.moveTo(2, 20, tv.wrap);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.col, 10);
	tv.moveTo(2, 20, tv.nowrap);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 9);
	tv.moveTo(2, -1, tv.wrap);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.col, 9);

	tv.moveTo(-1, 1, tv.nowrap);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.col, 1);
	tv.moveTo(1, -1, tv.nowrap);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.col, 0);
	tv.moveTo(10, 1, tv.nowrap);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.col, 1);
	tv.moveTo(1, 10, tv.nowrap);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.col, 9);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.moveTo(10, 0, tv.wrap);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.col, 0);
	ref << "fillRect(0,0,80,60,65535,0)";
	CHECK_EQ(pm->log, ref);

	tv.moveTo(5, 0, tv.wrap);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.col, 0);
	ref << "copyRect(0,0,0,12,80,48)";
	ref << "fillRect(0,48,80,12,65535,0)";
	CHECK_EQ(pm->log, ref);

	tv.moveTo(-1, 0, tv.wrap);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.col, 0);
	ref << "copyRect(0,12,0,0,80,48)";
	ref << "fillRect(0,0,80,12,65535,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: moveToCol()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	CHECK_EQ(tv.col, 0);
	tv.moveToCol(5);
	CHECK_EQ(tv.col, 5);
	tv.moveToCol(10, tv.nowrap);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	tv.moveToCol(-1, tv.nowrap);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.moveToCol(10, tv.wrap);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 0);
	tv.moveToCol(11, tv.wrap);
	CHECK_EQ(tv.col, 1);
	CHECK_EQ(tv.row, 1);
	tv.moveToCol(-1, tv.wrap);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(pm->log, ref);

	tv.moveToCol(-1, tv.wrap); // scrolls down 1 line
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	ref << "copyRect(0,12,0,0,80,48)";
	ref << "fillRect(0,0,80,12,65535,0)";
	CHECK_EQ(pm->log, ref);

	tv.moveToCol(51, tv.wrap); // scrolls up 1 line
	CHECK_EQ(tv.col, 1);
	CHECK_EQ(tv.row, 4);
	ref << "copyRect(0,0,0,12,80,48)";
	ref << "fillRect(0,48,80,12,65535,0)";
	CHECK_EQ(pm->log, ref);

	tv.moveToCol(10, tv.wrap); // no wrap => no scroll
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(pm->log, ref);

	tv.showCursor(); // scrolls up one line
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 4);
	ref << "copyRect(0,0,0,12,80,48)";	   // scroll
	ref << "fillRect(0,48,80,12,65535,0)"; // scroll
	ref << "xorRect(0,48,8,12,65535)";	   // show cursor
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: moveToRow()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	tv.moveTo(2, 2);
	CHECK_EQ(tv.row, 2);
	tv.moveToRow(3);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.col, 2);

	tv.moveToRow(10, tv.nowrap);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.col, 2);

	tv.moveToRow(-10, tv.nowrap);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.col, 2);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.moveToRow(8, tv.wrap); // scrolls 4 lines
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.col, 2);
	ref << "copyRect(0,0,0,48,80,12)";	   // scroll
	ref << "fillRect(0,12,80,48,65535,0)"; // scroll
	CHECK_EQ(pm->log, ref);

	tv.moveToRow(-2, tv.wrap); // scrolls 2 lines
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.col, 2);
	ref << "copyRect(0,24,0,0,80,36)";	  // scroll
	ref << "fillRect(0,0,80,24,65535,0)"; // scroll
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: setCharAttributes()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	CHECK_EQ(tv.getAttributes(), 0);
	CHECK_EQ(tv.dx, 1);
	CHECK_EQ(tv.dy, 1);

	tv.setAttributes(tv.italic);
	CHECK_EQ(tv.getAttributes(), tv.italic);
	CHECK_EQ(tv.dx, 1);
	CHECK_EQ(tv.dy, 1);

	tv.setAttributes(tv.bold, 0);
	CHECK_EQ(tv.getAttributes(), tv.italic + tv.bold);
	CHECK_EQ(tv.dx, 1);
	CHECK_EQ(tv.dy, 1);

	tv.setAttributes(tv.inverted, tv.italic);
	CHECK_EQ(tv.getAttributes(), tv.bold + tv.inverted);
	CHECK_EQ(tv.dx, 1);
	CHECK_EQ(tv.dy, 1);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.moveTo(0, 9);

	tv.setAttributes(tv.double_width);
	CHECK_EQ(tv.getAttributes(), tv.double_width);
	CHECK_EQ(tv.dx, 2);
	CHECK_EQ(tv.dy, 1);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(pm->log, ref);

	tv.setAttributes(tv.double_height, tv.double_width);
	CHECK_EQ(tv.getAttributes(), tv.double_height);
	CHECK_EQ(tv.dx, 1);
	CHECK_EQ(tv.dy, 2);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(pm->log, ref);

	tv.setAttributes(tv.double_height + tv.double_width);
	CHECK_EQ(tv.getAttributes(), tv.double_height + tv.double_width);
	CHECK_EQ(tv.dx, 2);
	CHECK_EQ(tv.dy, 2);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(pm->log, ref);

	tv.setAttributes(tv.transparent, 0);
	CHECK_EQ(tv.getAttributes(), tv.double_height + tv.double_width + tv.transparent);
	CHECK_EQ(tv.dx, 2);
	CHECK_EQ(tv.dy, 2);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(pm->log, ref);

	tv.moveTo(1, 3);
	tv.printChar('A');
	CHECK_EQ(tv.col, 5);
	CHECK_EQ(tv.row, 1);
	ref << "drawChar(24,0,bmp,12,0,1)";	 // char drawn in 4 parts
	ref << "drawChar(24,12,bmp,12,0,1)"; // color = 0 (black)
	ref << "drawChar(32,0,bmp,12,0,1)";	 // ink = 1 (foreground)
	ref << "drawChar(32,12,bmp,12,0,1)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: addCharAttributes()")
{
	// this calls setCharAttributes()

	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	tv.setAttributes(tv.bold);
	tv.addAttributes(tv.inverted);
	CHECK_EQ(tv.getAttributes(), tv.bold + tv.inverted);
}

TEST_CASE("Printer<Canvas>: removeCharAttributes()")
{
	// this calls setCharAttributes()

	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	tv.setAttributes(tv.bold + tv.inverted);
	tv.removeAttributes(tv.inverted);
	CHECK_EQ(tv.getAttributes(), tv.bold);

	tv.setAttributes(0xff);
	tv.removeAttributes();
	CHECK_EQ(tv.getAttributes(), 0);
}

TEST_CASE("Printer<Canvas>: limitCursorPosition()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	tv.col = 99;
	tv.row = 99;
	tv.limitCursorPosition();
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 4);

	tv.col = -99;
	tv.row = -99;
	tv.limitCursorPosition();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: validateCursorPosition()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor

	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 0);
	tv.col = 9;
	tv.row = 4;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 0);
	CHECK_EQ(pm->log, ref);

	tv.col = 9;
	tv.row = 6;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 2);
	ref << "copyRect(0,0,0,24,80,36)";
	ref << "fillRect(0,36,80,24,65535,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 19;
	tv.row = 3;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 2);

	tv.col = 19;
	tv.row = 4;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 3);
	ref << "copyRect(0,0,0,12,80,48)";
	ref << "fillRect(0,48,80,12,65535,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 10;
	tv.row = 3;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 3);
	CHECK_EQ(pm->log, ref);

	tv.col = 10;
	tv.row = 4;
	tv.validateCursorPosition(true);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 3);
	CHECK_EQ(pm->log, ref);

	tv.col = -10;
	tv.row = 3;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.scroll_count, 3);
	CHECK_EQ(pm->log, ref);

	tv.col = -10;
	tv.row = 3;
	tv.validateCursorPosition(true);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.scroll_count, 3);
	CHECK_EQ(pm->log, ref);

	tv.col = 10;
	tv.row = -1;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 3);
	CHECK_EQ(pm->log, ref);

	tv.col = 8;
	tv.row = -1;
	tv.validateCursorPosition(false);
	CHECK_EQ(tv.col, 8);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 2);
	ref << "copyRect(0,12,0,0,80,48)";
	ref << "fillRect(0,0,80,12,65535,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: cursorLeft()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 0);

	tv.col = 5;
	tv.cursorLeft();
	CHECK_EQ(tv.col, 4);
	tv.cursorLeft(3);
	CHECK_EQ(tv.col, 1);
	tv.cursorLeft(3, tv.nowrap);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.scroll_count, 0);
	tv.cursorLeft();
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, -1);

	tv.row = 2;
	tv.cursorLeft(20, tv.nowrap);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.scroll_count, -1);
	tv.cursorLeft(20);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, -1);
}

TEST_CASE("Printer<Canvas>: cursorRight()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight();
	CHECK_EQ(tv.col, 1);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight(3);
	CHECK_EQ(tv.col, 4);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight(10);
	CHECK_EQ(tv.col, 4);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight(10, tv.nowrap);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight();
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight();
	CHECK_EQ(tv.col, 1);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight(9);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorRight(25);
	CHECK_EQ(tv.col, 5);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 1);
}

TEST_CASE("Printer<Canvas>: cursorUp()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	tv.col = 4;
	tv.row = 4;

	tv.cursorUp(); // middle of screen
	CHECK_EQ(tv.col, 4);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorUp(2); // middle of screen, N>1
	CHECK_EQ(tv.col, 4);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorUp(2); // scroll
	CHECK_EQ(tv.col, 4);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, -1);

	tv.cursorUp(2, tv.nowrap); // no scroll
	CHECK_EQ(tv.col, 4);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, -1);

	tv.col = 10; // col = cols: cursorUp() does not affect col
	tv.row = 4;
	tv.cursorUp();
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.scroll_count, -1);
	tv.cursorUp(1, tv.nowrap);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.scroll_count, -1);
	tv.cursorUp(10, tv.nowrap);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, -1);
	tv.cursorUp(10);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, -11);
}

TEST_CASE("Printer<Canvas>: cursorDown()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	tv.cursorDown();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorDown(2);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.scroll_count, 0);

	tv.cursorDown(2);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 1);

	tv.cursorDown(2, tv.nowrap);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 1);

	tv.col = 10;
	tv.row = 0;

	tv.cursorDown();
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.scroll_count, 1);

	tv.cursorDown(2);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 3);
	CHECK_EQ(tv.scroll_count, 1);

	tv.cursorDown(2);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 2);

	tv.cursorDown(2, tv.nowrap);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 2);
}

TEST_CASE("Printer<Canvas>: cursorTab()")
{
	{
		RCPtr<Pixmap> pm = new Pixmap(80 * 8, 40 * 12); // 80*40
		TextVDU		  tv(pm);

		CHECK_EQ(tv.col, 0);
		CHECK_EQ(tv.row, 0);
		tv.cursorTab();
		CHECK_EQ(tv.col, 8);
		CHECK_EQ(tv.row, 0);
		tv.col--;
		tv.cursorTab();
		CHECK_EQ(tv.col, 8);
		CHECK_EQ(tv.row, 0);
		tv.cursorTab();
		CHECK_EQ(tv.col, 16);
		CHECK_EQ(tv.row, 0);
		tv.col++;
		tv.cursorTab();
		CHECK_EQ(tv.col, 24);
		CHECK_EQ(tv.row, 0);

		tv.cursorTab(10);
		CHECK_EQ(tv.col, 24);
		CHECK_EQ(tv.row, 1);

		tv.col = 72;
		tv.cursorTab();
		CHECK_EQ(tv.col, 80);
		CHECK_EQ(tv.row, 1);
		tv.col--;
		tv.cursorTab();
		CHECK_EQ(tv.col, 80);
		CHECK_EQ(tv.row, 1);
		tv.cursorTab();
		CHECK_EQ(tv.col, 8);
		CHECK_EQ(tv.row, 2);

		tv.row = 39;
		tv.cursorTab(8);
		CHECK_EQ(tv.col, 72);
		CHECK_EQ(tv.row, 39);
		tv.col--;
		tv.cursorTab(3);
		CHECK_EQ(tv.col, 8);
		CHECK_EQ(tv.row, 39);
		CHECK_EQ(tv.scroll_count, 1);

		auto check = [&](int oldcol, int count, int newcol, int dy) {
			tv.col = oldcol;
			tv.row = 0;
			tv.cursorTab(count);
			CHECK_EQ(tv.col, newcol);
			CHECK_EQ(tv.row, dy);
		};
		check(0, 1, 8, 0);
		check(7, 1, 8, 0);
		check(79, 1, 80, 0);
		check(80, 1, 8, 1);
		check(0, 2, 16, 0);
		check(7, 2, 16, 0);
		check(79, 2, 8, 1);
		check(80, 2, 16, 1);
		check(0, 10, 80, 0);
		check(7, 10, 80, 0);
		check(79, 10, 72, 1);
		check(80, 10, 80, 1);
	}
	{
		RCPtr<Pixmap> pm = new Pixmap(82 * 8, 40 * 12); // 82*40
		TextVDU		  tv(pm);

		auto check = [&](int oldcol, int count, int newcol, int dy) {
			tv.col = oldcol;
			tv.row = 0;
			tv.cursorTab(count);
			CHECK_EQ(tv.col, newcol);
			CHECK_EQ(tv.row, dy);
		};

		check(0, 1, 8, 0);
		check(7, 1, 8, 0);
		check(79, 1, 80, 0);
		check(80, 1, 82, 0);
		check(81, 1, 82, 0);
		check(82, 1, 8, 1);
		check(0, 2, 16, 0);
		check(7, 2, 16, 0);
		check(79, 2, 82, 0);
		check(80, 2, 8, 1);
		check(81, 2, 8, 1);
		check(82, 2, 16, 1);
		check(0, 10, 80, 0);
		check(7, 10, 80, 0);
		check(79, 10, 64, 1);
		check(80, 10, 72, 1);
		check(81, 10, 72, 1);
		check(82, 10, 80, 1);
	}
}

TEST_CASE("Printer<Canvas>: cursorReturn()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);
	tv.cursorReturn();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);

	tv.col = 8;
	tv.row = 1;
	tv.cursorReturn();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 1);

	tv.col = 80;
	tv.row = 1;
	tv.cursorReturn();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.scroll_count, 0);
}

TEST_CASE("Printer<Canvas>: newLine()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 0);
	CHECK_EQ(tv.scroll_count, 0);
	tv.newLine();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 1);

	tv.col = 8;
	tv.newLine();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 2);

	tv.col = 80;
	tv.newLine();
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 3);

	tv.newLine();
	tv.validateCursorPosition(true);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 0);

	tv.newLine();
	tv.validateCursorPosition(true);
	CHECK_EQ(tv.col, 0);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 1);
}

TEST_CASE("Printer<Canvas>: clearRect()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row	   = 1;
	tv.col	   = 2;
	tv.bgcolor = 1234;

	tv.clearRect(1, 3, 4, 5);
	ref << "fillRect(24,12,40,48,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.clearRect(1, 3, -4, 5); // no action
	tv.clearRect(1, 3, 4, -5); // no action
	tv.clearRect(1, 3, 0, 5);  // no action
	tv.clearRect(1, 3, 4, 0);  // no action
	CHECK_EQ(pm->log, ref);

	tv.bg_ink = 1;
	tv.clearRect(-1, -3, 4, 5); // limited by Pixmap function
	ref << "fillRect(-24,-12,40,48,1234,1)";

	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.col, 2);
	CHECK_EQ(tv.scroll_count, 0);
}

TEST_CASE("Printer<Canvas>: clearToStartOfLine()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row	   = 1;
	tv.col	   = 2;
	tv.bgcolor = 1234;

	tv.clearToStartOfLine(false);
	ref << "fillRect(0,12,16,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.clearToStartOfLine(true);
	ref << "fillRect(0,12,24,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 0;
	tv.clearToStartOfLine(false); // no action
	CHECK_EQ(pm->log, ref);
	tv.clearToStartOfLine(true);
	ref << "fillRect(0,12,8,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 9;
	tv.row = 4;
	tv.clearToStartOfLine(false);
	ref << "fillRect(0,48,72,12,1234,0)";
	CHECK_EQ(pm->log, ref);
	tv.clearToStartOfLine(true);
	ref << "fillRect(0,48,80,12,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.col, 9);
	CHECK_EQ(tv.row, 4);

	tv.col = 10;
	tv.clearToStartOfLine(false); // stays in line
	ref << "fillRect(0,48,80,12,1234,0)";
	CHECK_EQ(pm->log, ref);
	tv.clearToStartOfLine(true);		  // stays in line
	ref << "fillRect(0,48,88,12,1234,0)"; // clear to start of line incl. cursor position
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 0);
}

TEST_CASE("Printer<Canvas>: clearToStartOfScreen()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row	   = 0;
	tv.col	   = 4;
	tv.bgcolor = 1234;

	tv.clearToStartOfScreen(false);
	ref << "fillRect(0,0,32,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.row = 2;
	tv.clearToStartOfScreen(false);
	ref << "fillRect(0,24,32,12,1234,0)";
	ref << "fillRect(0,0,80,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 10;
	tv.row = 4;
	tv.clearToStartOfScreen(false); // stay in line
	ref << "fillRect(0,48,80,12,1234,0)";
	ref << "fillRect(0,0,80,48,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 4);

	tv.clearToStartOfScreen(true);		  // stay in line
	ref << "fillRect(0,48,88,12,1234,0)"; // clear to start of line incl. cursor position
	ref << "fillRect(0,0,80,48,1234,0)";  // clear to start of screen
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.col, 10);
	CHECK_EQ(tv.row, 4);
	CHECK_EQ(tv.scroll_count, 0);
}

TEST_CASE("Printer<Canvas>: clearToEndOfLine()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row	   = 0;
	tv.col	   = 4;
	tv.bgcolor = 1234;

	tv.clearToEndOfLine();
	ref << "fillRect(32,0,48,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.row = 4;
	tv.clearToEndOfLine();
	ref << "fillRect(32,48,48,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 9;
	tv.clearToEndOfLine();
	ref << "fillRect(72,48,8,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 10;
	tv.clearToEndOfLine();
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: clearToEndOfScreen()") //
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row	   = 0;
	tv.col	   = 4;
	tv.bgcolor = 1234;

	tv.clearToEndOfScreen();
	ref << "fillRect(32,0,48,12,1234,0)";
	ref << "fillRect(0,12,80,48,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.row = 4;
	tv.clearToEndOfScreen();
	ref << "fillRect(32,48,48,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.row = 2;
	tv.col = 9;
	tv.clearToEndOfScreen();
	ref << "fillRect(72,24,8,12,1234,0)";
	ref << "fillRect(0,36,80,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.col = 10;
	tv.clearToEndOfScreen();
	ref << "fillRect(0,36,80,24,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: copyRect()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row = 1;
	tv.col = 2;

	tv.copyRect(1, 2, 3, 4, -6, 5); // no action
	tv.copyRect(1, 2, 3, 4, 6, -5); // no action
	tv.copyRect(1, 2, 3, 4, 0, 5);	// no action
	tv.copyRect(1, 2, 3, 4, 6, 0);	// no action
	CHECK_EQ(pm->log, ref);

	tv.copyRect(1, 2, 3, 4, 5, 10);
	ref << "copyRect(16,12,32,36,80,60)";
	CHECK_EQ(pm->log, ref);

	tv.copyRect(-1, -2, -3, -4, 5, 10);
	ref << "copyRect(-16,-12,-32,-36,80,60)";
	CHECK_EQ(pm->log, ref);

	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.col, 2);
	CHECK_EQ(tv.scroll_count, 0);
}

TEST_CASE("Printer<Canvas>: scrollScreen()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row	   = 1;
	tv.col	   = 2;
	tv.bgcolor = 1234;

	tv.scrollScreen(0, 0);
	ref << "copyRect(0,0,0,0,80,60)";
	CHECK_EQ(pm->log, ref);

	tv.scrollScreen(0, 10);
	tv.scrollScreen(0, -10);
	tv.scrollScreen(5, 0);
	tv.scrollScreen(-5, 0);
	tv.scrollScreen(0, 11);
	ref << "fillRect(0,0,80,60,1234,0)";
	ref << "fillRect(0,0,80,60,1234,0)";
	ref << "fillRect(0,0,80,60,1234,0)";
	ref << "fillRect(0,0,80,60,1234,0)";
	ref << "fillRect(0,0,80,60,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.scrollScreen(0, 2); // right
	ref << "copyRect(16,0,0,0,64,60)";
	ref << "fillRect(0,0,16,60,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.scrollScreen(0, -2); // left
	ref << "copyRect(0,0,16,0,64,60)";
	ref << "fillRect(64,0,16,60,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.scrollScreen(2, 0); //down
	ref << "copyRect(0,24,0,0,80,36)";
	ref << "fillRect(0,0,80,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.scrollScreen(-2, 0); // up
	ref << "copyRect(0,0,0,24,80,36)";
	ref << "fillRect(0,36,80,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.scrollScreen(1, 2);
	ref << "copyRect(16,12,0,0,64,48)";
	ref << "fillRect(0,0,16,60,1234,0)";
	ref << "fillRect(0,0,80,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.scrollScreen(-1, -2);
	ref << "copyRect(0,0,16,12,64,48)";
	ref << "fillRect(64,0,16,60,1234,0)";
	ref << "fillRect(0,48,80,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.col, 2);
	CHECK_EQ(tv.scroll_count, 0);
}

TEST_CASE("Printer<Canvas>: scrollScreenUp()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollScreenUp(-2);
	CHECK_EQ(pm->log, ref);

	tv.scrollScreenUp(2);
	ref << "copyRect(0,0,0,24,80,36)";
	ref << "fillRect(0,36,80,24,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: scrollScreenDown()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollScreenDown(-2);
	CHECK_EQ(pm->log, ref);

	tv.scrollScreenDown(2);
	ref << "copyRect(0,24,0,0,80,36)";
	ref << "fillRect(0,0,80,24,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: scrollScreenLeft()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollScreenLeft(-2);
	CHECK_EQ(pm->log, ref);

	tv.scrollScreenLeft(2);
	ref << "copyRect(0,0,16,0,64,60)";
	ref << "fillRect(64,0,16,60,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: scrollScreenRight()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollScreenRight(-2);
	CHECK_EQ(pm->log, ref);

	tv.scrollScreenRight(2);
	ref << "copyRect(16,0,0,0,64,60)";
	ref << "fillRect(0,0,16,60,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: scrollRect()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.row	   = 1;
	tv.col	   = 2;
	tv.bgcolor = 1234;

	tv.scrollRect(1, 1, 3, 8, 0, 0);
	ref << "copyRect(8,12,8,12,64,36)";
	CHECK_EQ(pm->log, ref);

	tv.scrollRect(0, 0, 5, 10, 0, 10); // more than whole rect:
	tv.scrollRect(0, 0, 5, 10, 0, -10);
	tv.scrollRect(0, 0, 5, 10, 5, 0);
	tv.scrollRect(0, 0, 5, 10, -5, 0);
	ref << "fillRect(0,0,80,60,1234,0)";
	ref << "fillRect(0,0,80,60,1234,0)";
	ref << "fillRect(0,0,80,60,1234,0)";
	ref << "fillRect(0,0,80,60,1234,0)";
	CHECK_EQ(pm->log, ref);

	tv.scrollRect(1, 2, 5, 10, 0, 10); // more than whole rect:
	tv.scrollRect(1, 2, 5, 10, 0, -10);
	tv.scrollRect(1, 2, 5, 10, 5, 0);
	tv.scrollRect(1, 2, 5, 10, -5, 0);
	ref << "fillRect(16,12,64,48,1234,0)";
	ref << "fillRect(16,12,64,48,1234,0)";
	ref << "fillRect(16,12,64,48,1234,0)";
	ref << "fillRect(16,12,64,48,1234,0)";
	CHECK_EQ(pm->log, ref);

	ref.purge(), pm->log.purge();

	tv.scrollRect(3, 3, 3, 5, 0, 2); // right
	ref << "copyRect(40,36,24,36,24,24)";
	ref << "fillRect(24,36,16,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	ref.purge(), pm->log.purge();

	tv.scrollRect(3, 3, 3, 5, 0, -2); // left
	ref << "copyRect(24,36,40,36,24,24)";
	ref << "fillRect(48,36,16,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	ref.purge(), pm->log.purge();

	tv.scrollRect(1, 3, 3, 5, 2, 0); //down
	ref << "copyRect(24,36,24,12,40,12)";
	ref << "fillRect(24,12,40,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	ref.purge(), pm->log.purge();

	tv.scrollRect(1, 3, 3, 5, -2, 0); // up
	ref << "copyRect(24,12,24,36,40,12)";
	ref << "fillRect(24,24,40,24,1234,0)";
	CHECK_EQ(pm->log, ref);

	ref.purge(), pm->log.purge();

	tv.scrollRect(1, 3, 3, 5, 1, 2); // down+right
	ref << "copyRect(40,24,24,12,24,24)";
	ref << "fillRect(24,12,16,36,1234,0)";
	ref << "fillRect(24,12,40,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	ref.purge(), pm->log.purge();

	tv.scrollRect(1, 3, 3, 5, -1, -2); // left+up
	ref << "copyRect(24,12,40,24,24,24)";
	ref << "fillRect(48,12,16,36,1234,0)";
	ref << "fillRect(24,36,40,12,1234,0)";
	CHECK_EQ(pm->log, ref);

	ref.purge(), pm->log.purge();

	CHECK_EQ(tv.row, 1);
	CHECK_EQ(tv.col, 2);
	CHECK_EQ(tv.scroll_count, 0);
}

TEST_CASE("Printer<Canvas>: scrollRectLeft()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollRectLeft(1, 3, 3, 5, -2); // no action
	CHECK_EQ(pm->log, ref);

	tv.scrollRectLeft(1, 3, 3, 5, 2);
	ref << "copyRect(24,12,40,12,24,36)";
	ref << "fillRect(48,12,16,36,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: scrollRectRight()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollRectRight(1, 3, 3, 5, -2); // no action
	CHECK_EQ(pm->log, ref);

	tv.scrollRectRight(1, 3, 3, 5, 2);
	ref << "copyRect(40,12,24,12,24,36)";
	ref << "fillRect(24,12,16,36,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: scrollRectUp()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollRectUp(1, 3, 3, 5, -2); // no action
	CHECK_EQ(pm->log, ref);

	tv.scrollRectUp(1, 3, 3, 5, 2);
	ref << "copyRect(24,12,24,36,40,12)";
	ref << "fillRect(24,24,40,24,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: scrollRectDown()")
{
	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;

	tv.scrollRectDown(1, 3, 3, 5, -2); //no action
	CHECK_EQ(pm->log, ref);

	tv.scrollRectDown(1, 3, 3, 5, 2);
	ref << "copyRect(24,36,24,12,40,12)";
	ref << "fillRect(24,12,40,24,1234,0)";
	CHECK_EQ(pm->log, ref);
}

TEST_CASE("Printer<Canvas>: insertChars()")
{
	// implemented as scrollRectRight()

	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;
	tv.row	   = 2;
	tv.col	   = 5;
	tv.showCursor();
	tv.insertChars(3);
	ref << "xorRect(40,24,8,12,1234)"; // show cursor
	ref << "xorRect(40,24,8,12,1234)"; // hide cursor
	ref << "copyRect(64,24,40,24,16,12)";
	ref << "fillRect(40,24,24,12,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 5);
}

TEST_CASE("Printer<Canvas>: deleteChars()")
{
	// implemented as scrollRectLeft()

	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;
	tv.row	   = 2;
	tv.col	   = 5;
	tv.showCursor();
	tv.deleteChars(3);
	ref << "xorRect(40,24,8,12,1234)"; // show cursor
	ref << "xorRect(40,24,8,12,1234)"; // hide cursor
	ref << "copyRect(40,24,64,24,16,12)";
	ref << "fillRect(56,24,24,12,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 5);
}

TEST_CASE("Printer<Canvas>: insertRows()")
{
	// implemented as scrollRectUp()

	RCPtr<Pixmap> pm = new Pixmap(80, 120); // 10*10
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,120,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;
	tv.row	   = 2;
	tv.col	   = 5;
	tv.insertRows(3);
	ref << "copyRect(0,60,0,24,80,60)";
	ref << "fillRect(0,24,80,36,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 5);
}

TEST_CASE("Printer<Canvas>: deleteRows()")
{
	// implemented as scrollRectDown()

	RCPtr<Pixmap> pm = new Pixmap(80, 120); // 10*10
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,120,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;
	tv.row	   = 2;
	tv.col	   = 5;
	tv.deleteRows(3);
	ref << "copyRect(0,24,0,60,80,60)";
	ref << "fillRect(0,84,80,36,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 5);
}

TEST_CASE("Printer<Canvas>: insertColumns()")
{
	// implemented as scrollRectRight()

	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;
	tv.row	   = 2;
	tv.col	   = 5;
	tv.insertColumns(3);
	ref << "copyRect(64,0,40,0,16,60)";
	ref << "fillRect(40,0,24,60,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 5);
}

TEST_CASE("Printer<Canvas>: deleteColumns()")
{
	// implemented as scrollRectLeft()

	RCPtr<Pixmap> pm = new Pixmap(80, 60); // 10*5
	TextVDU		  tv(pm);

	Array<cstr> ref;
	ref << "Pixmap(80,60,a1w8,12)"; // ctor
	CHECK_EQ(pm->log, ref);

	tv.bgcolor = 1234;
	tv.row	   = 2;
	tv.col	   = 5;
	tv.deleteColumns(3);
	ref << "copyRect(40,0,64,0,16,60)";
	ref << "fillRect(56,0,24,60,1234,0)";
	CHECK_EQ(pm->log, ref);
	CHECK_EQ(tv.row, 2);
	CHECK_EQ(tv.col, 5);
}

TEST_CASE("Printer<Canvas>: printCharMatrix()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: printChar()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: print()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: inputLine()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: readBmp()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: writeBmp()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: getCharMatrix()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: getGraphicsCharMatrix()") //
{
	CHECK(1 == 1);
}

TEST_CASE("Printer<Canvas>: applyAttributes()") //
{
	CHECK(1 == 1);
}


} // namespace kilipili::Test


/*

































*/
