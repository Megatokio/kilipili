// Copyright (c) 2012 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Canvas.h"
#undef CHAR_WIDTH
#include "../Devices/SerialDevice.h"

namespace kio::Graphics
{

class PicoTerm : public Devices::SerialDevice
{
public:
	using super = Devices::SerialDevice;
	using SIZE	= Devices::SIZE;
	using IoCtl = Devices::IoCtl;

	static constexpr int CHAR_HEIGHT = 12;
	static constexpr int CHAR_WIDTH	 = 8;

	static constexpr uint default_bgcolor = 0x0000ffff; // white paper (note: ic1 has inverted default colormap)
	static constexpr uint default_fgcolor = 0;			// black ink   (      => black paper & green ink)

	using CharMatrix = uint8[CHAR_HEIGHT];

	// print attributes:
	enum : uint8 {
		ATTR_BOLD				 = 1 << 0,
		ATTR_UNDERLINE			 = 1 << 1,
		ATTR_INVERTED			 = 1 << 2,
		ATTR_ITALIC				 = 1 << 3,
		ATTR_OVERPRINT			 = 1 << 4,
		ATTR_DOUBLE_WIDTH		 = 1 << 5,
		ATTR_DOUBLE_HEIGHT		 = 1 << 6,
		ATTR_GRAPHICS_CHARACTERS = 1 << 7
	};

	// Control Characters:
	enum : uchar {
		RESET				   = 0,
		CLS					   = 1,
		MOVE_TO_POSITION	   = 2,
		MOVE_TO_COL			   = 3,
		SHOW_CURSOR			   = 7,	 //				(BELL)
		CURSOR_LEFT			   = 8,	 // scrolls		(BS)
		TAB					   = 9,	 // scrolls
		CURSOR_DOWN			   = 10, // scrolls		(NL)
		CURSOR_UP			   = 11, // scrolls
		CURSOR_RIGHT		   = 12, // scrolls		(FF)
		RETURN				   = 13, // COL := 0
		CLEAR_TO_END_OF_LINE   = 14,
		CLEAR_TO_END_OF_SCREEN = 15,
		SET_ATTRIBUTES		   = 16,
		XON					   = 17,
		XOFF				   = 19,
		REPEAT_NEXT_CHAR	   = 20,
		SCROLL_SCREEN		   = 21,
		ESC					   = 27,
	};

	Canvas& pixmap;

	const ColorMode	 colormode;
	const AttrHeight attrheight;
	const ColorDepth colordepth;	 // 0 .. 4  log2 of bits per color in attributes[]
	const AttrMode	 attrmode;		 // 0 .. 2  log2 of bits per color in pixmap[]
	const AttrWidth	 attrwidth;		 // 0 .. 3  log2 of width of tiles
	const uint8		 bits_per_color; // bits per color in pixmap[] or attributes[]
	const uint8		 bits_per_pixel; // bpp in pixmap[]
	char			 _padding = 0;

	// Screen size:
	int screen_width;  // [characters]
	int screen_height; // [characters]

	// foreground and background color:
	uint bgcolor;	 // paper color
	uint fgcolor;	 // text color
	uint fg_ink = 1; // for attribute pixmaps
	uint bg_ink = 0; // for attribute pixmaps

	// current print position:
	int	  row;
	int	  col;
	uint8 dx, dy; // 1 or 2, if double width & double height
	uint8 attributes;

	// cursor blob:
	bool   cursorVisible;  // currently visible?
	uint32 cursorXorColor; // value used to xor the colors

	// write() state machine:
	uint8  repeat_cnt;
	bool   auto_crlf = true;
	uint16 sm_state	 = 0;

	PicoTerm(Canvas&);

	/* SerialDevice Interface:
	*/
	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;
	virtual SIZE   read(char*, SIZE, bool = false) override { throw Devices::NOT_READABLE; }
	virtual SIZE   write(const char* data, SIZE, bool partial = false) override;
	//virtual SIZE putc(char) override;
	//virtual SIZE puts(cstr) override;
	//virtual SIZE printf(cstr fmt, ...) override __printflike(2, 3);

	int getc(void (*sm)(), int timeout_us = 0);
	str input_line(void (*sm)(), str oldtext = nullptr, int epos = 0);

	void reset();
	void cls();
	void moveTo(int row, int col) noexcept;
	void moveToCol(int col) noexcept;
	void setPrintAttributes(uint8 attr);
	void printCharMatrix(CharMatrix, int count = 1);
	void printChar(char c, int count = 1); // no ctl
	void printText(cstr text);			   // no ctl
	void cursorLeft(int count = 1);
	void cursorRight(int count = 1);
	void cursorUp(int count = 1);
	void cursorDown(int count = 1);
	void cursorTab(int count = 1);
	void cursorReturn();
	void newLine();
	void clearToEndOfLine();
	void clearToEndOfScreen();
	void copyRect(int src_row, int src_col, int dest_row, int dest_col, int rows, int cols);
	void showCursor(bool on = true);
	void hideCursor();
	void validateCursorPosition();
	void scrollScreen(coord dx, coord dy);
	void scrollScreenUp(int rows /*char*/);
	void scrollScreenDown(int rows /*char*/);
	void scrollScreenLeft(int cols /*char*/);
	void scrollScreenRight(int cols /*char*/);

	char* identify();

	void readBmp(CharMatrix, bool use_fgcolor);
	void writeBmp(CharMatrix, uint8 attr);
	void getCharMatrix(CharMatrix, char c);
	void getGraphicsCharMatrix(CharMatrix, char c);
	void applyAttributes(CharMatrix);
	void eraseRect(int row, int col, int rows, int cols);

private:
	void show_cursor(bool f);

protected:
	// these will stall because USB isn't polled:
	using super::getc;
	using super::gets;
	//virtual char getc() override;
	//virtual str  gets() override;
	//virtual int  getc(uint timeout) override;
};


} // namespace kio::Graphics


/*



























*/
