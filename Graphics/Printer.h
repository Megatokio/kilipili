// Copyright (c) 2012 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Devices/SerialDevice.h"
#include "Font.h"
#include "common/no_copy_move.h"
#include "common/standard_types.h"
#include <cstdarg>
#include <functional>


namespace kilipili
{

/*	template class Printer provides functions for printing on a Pixmap or CharMap.
	class Printer<> is the base class for the concrete implementations for the 3 variants.

	class Printer<> also implements the SerialDevice interface.

note:
	The implementations override all SerialDevice virtual functions and never call through to them.
	So it should not be neccessary to include kilipili_devices into your project, if you don't want to use them.
	But c++ may decide it needs them for the SerialDevice vtable anyway.
	Then it should be enough to add Devices/SerialDevice.cpp to your project.

color info:
	fgbits:		number of bits in foreground color map (pen)
	bgbits:		number of bits in background color map (paper)
	shared_cmaps: whether pen and paper share a common cmap
	true_color:	whether setPen() / setPaper() use true Color or a color index

					fgbits	bgbits	shared	rgb	
	SimpleCharMap	0		0		no		no		setPen() / setPaper() does nothing
	CharMap			0…4		0…4		no		no		indexed color
	Pixmap_i1 … i8	1…8		1…8		yes		no		indexed color
	Pixmap_wAttr	1…2		1…2		yes		yes		true color + ink
	Pixmap_rgb		0		0		no		yes		true color
*/

template<class = void>
class Printer;

template<>
class Printer<> : public SerialDevice
{
	NO_COPY_MOVE(Printer);
	Id("Printer");

public:
	// SerialDevice methods overrides:
	// note: using a Printer as a SerialDevice is only a side quest.
	virtual ~Printer() noexcept override = default;
	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;
	virtual SIZE   write(const void* data, SIZE, bool partial = false) noexcept override = 0; // any char, even chr(0)
	virtual void   putc(char) noexcept override											 = 0; // supports \n, \r and \t
	virtual void   puts(cstr) noexcept override											 = 0; // supports \n, \r and \t
	virtual void   printf(cstr fmt, ...) noexcept override final __printflike(2, 3);		  // supports \n, \r and \t
	virtual void   printf(cstr fmt, va_list) noexcept override final __printflike(2, 0);	  // supports \n, \r and \t
	// override all virtual functions => we don't need to have Devices/SerialDevice.cpp:
	__noreturn int	getc(uint timeout_us) override final { throw NOT_READABLE; }
	__noreturn char getc() override final { throw NOT_READABLE; }
	__noreturn str	gets(uint line_ends = 0x2401) override final { throw NOT_READABLE; }

	// the normal stuff:
	virtual void reset(bool cls = false) noexcept;
	virtual cstr identification()																			 = 0;
	virtual void cls() noexcept																				 = 0;
	virtual void clearRect(int row, int col, int rows, int cols) noexcept									 = 0;
	virtual void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept = 0;
	virtual void printChar(char c, int count = 1) noexcept = 0; // no ctl
	void		 print(cstr text) noexcept { puts(text); }		// supports \n, \r and \t
	void		 printAt(int row, int col, cstr text) noexcept; // supports \n, \r and \t

	// print attributes: (values 1:1 for Printer<Canvas>)
	enum Attributes : uint8 {
		normal		  = 0x00,
		bold		  = 0x01,
		underline	  = 0x02,
		inverted	  = 0x04,
		italic		  = 0x08,
		transparent	  = 0x10,
		double_width  = 0x20,
		double_height = 0x40,
		graphics	  = 0x80
	};

	virtual void  setAttributes(uint add, uint remove = 0xff) noexcept = 0;
	void		  addAttributes(uint a) noexcept { setAttributes(a, 0); }
	void		  removeAttributes(uint a = 0xff) noexcept { setAttributes(0, a); }
	virtual uint8 getAttributes() const noexcept = 0;

	virtual void		setFont(const Font*, bool gfx = 0) noexcept = 0;
	virtual const Font* getFont(bool gfx = 0) noexcept				= 0;

	virtual void setPen(uint color, uint ink = 1) noexcept	 = 0; // color = true Color or color index
	virtual void setPaper(uint color, uint ink = 0) noexcept = 0; // color = true Color or color index
	virtual void setPen(Color, uint ink = 1) noexcept		 = 0; // color = true Color
	virtual void setPaper(Color, uint ink = 0) noexcept		 = 0; // color = true Color

	void showCursor(bool on = true) noexcept;
	void hideCursor() noexcept;

	str inputLine(std::function<int()> getchar, str oldtext = nullptr, int epos = 0);

	enum AutoWrap : bool { nowrap, wrap };

	void validateCursorPosition(bool col80ok) noexcept;
	void limitCursorPosition() noexcept;
	void moveTo(int row, int col, AutoWrap = nowrap) noexcept;
	void moveToCol(int col, AutoWrap = nowrap) noexcept;
	void moveToRow(int row, AutoWrap = nowrap) noexcept;
	void cursorLeft(int count = 1, AutoWrap = wrap) noexcept;
	void cursorRight(int count = 1, AutoWrap = wrap) noexcept;
	void cursorUp(int count = 1, AutoWrap = wrap) noexcept;
	void cursorDown(int count = 1, AutoWrap = wrap) noexcept;
	void cursorTab(int count = 1) noexcept;
	void cursorReturn() noexcept;
	void newLine() noexcept;

	void clearToStartOfLine(bool incl_cursorpos = 0) noexcept;
	void clearToStartOfScreen(bool incl_cursorpos = 0) noexcept;
	void clearToEndOfLine() noexcept;
	void clearToEndOfScreen() noexcept;

	void scrollScreen(int dy, int dx) noexcept;
	void scrollScreenUp(int rows = 1) noexcept;
	void scrollScreenDown(int rows = 1) noexcept;
	void scrollScreenLeft(int cols = 1) noexcept;
	void scrollScreenRight(int cols = 1) noexcept;

	void scrollRect(int row, int col, int rows, int cols, int dy, int dx) noexcept;
	void scrollRectLeft(int row, int col, int rows, int cols, int dist = 1) noexcept;
	void scrollRectRight(int row, int col, int rows, int cols, int dist = 1) noexcept;
	void scrollRectUp(int row, int col, int rows, int cols, int dist = 1) noexcept;
	void scrollRectDown(int row, int col, int rows, int cols, int dist = 1) noexcept;

	void insertChars(int count = 1) noexcept;
	void deleteChars(int count = 1) noexcept;
	void insertRows(int count = 1) noexcept;
	void deleteRows(int count = 1) noexcept;
	void insertColumns(int count = 1) noexcept;
	void deleteColumns(int count = 1) noexcept;

	// Screen size: [characters]
	const int rows, cols;

	// color info:
	const uint8 fgbits; // 0 … 8:  bits per color index (pen)
	const uint8 bgbits; // 0 … 8:  bits per color index (paper)
	enum AttrInfo : uint16 { w_cmap = 1, w_sep_cmap = 3, w_rgb = 4, w_attr = 12 };
	AttrInfo attr_info;
	bool	 is_true_color() const noexcept { return attr_info & w_rgb; }
	bool	 is_indexed_color() const noexcept { return attr_info & w_cmap; }
	bool	 has_separate_cmaps() const noexcept { return attr_info & (w_sep_cmap - w_cmap); }
	bool	 is_attribute_mode() const noexcept { return attr_info & (w_attr - w_rgb); }

	// current print position:
	int	  row = 0, col = 0;
	int	  scroll_count = 0;
	uint8 dx = 1, dy = 1;		  // 1 or 2, if double width & double height
	bool  cursor_visible = false; // currently visible?

protected:
	uint8 attributes = 0;

	Printer(int rows, int cols, int fgbits, int bgbits, AttrInfo) noexcept;
	virtual void show_cursor(bool f) noexcept {}
	void		 validate_hpos(bool col80ok) noexcept;
	void		 validate_vpos() noexcept;
};


} // namespace kilipili

/*


































*/
