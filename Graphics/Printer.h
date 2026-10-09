// Copyright (c) 2012 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/RCPtr.h"
#include "common/no_copy_move.h"
#include "common/standard_types.h"
#include <cstdarg>
#include <functional>


namespace kilipili::Graphics
{

template<class = void>
class Printer;

template<>
class Printer<> : public RCObject
{
	NO_COPY_MOVE(Printer);
	Id("Printer");

public:
	virtual void reset(bool cls = false) noexcept;
	virtual cstr identify()																					 = 0;
	virtual void clearRect(int row, int col, int rows, int cols) noexcept									 = 0;
	virtual void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept = 0;
	virtual void printChar(char c, int count = 1) noexcept = 0; // no ctl
	virtual void print(cstr text) noexcept;						// supports \n, \r and \t
	virtual void write(cptr text, int count) noexcept;			// any char, even chr(0)

	void printAt(int row, int col, cstr text) noexcept;			// supports \n, \r and \t
	void printf(cstr fmt, ...) noexcept __printflike(2, 3);		// supports \n, \r and \t
	void printf(cstr fmt, va_list) noexcept __printflike(2, 0); // supports \n, \r and \t

	virtual void setAttributes(uint add, uint remove = 0xff) noexcept = 0;
	void		 addAttributes(uint a) noexcept { setAttributes(a, 0); }
	void		 removeAttributes(uint a = 0xff) noexcept { setAttributes(0, a); }

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

	void cls() noexcept { reset(true); }
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

	// current print position:
	int	  row = 0, col = 0;
	int	  scroll_count = 0;
	uint8 dx = 1, dy = 1;		  // 1 or 2, if double width & double height
	uint8 attributes	 = 0;	  //
	bool  cursor_visible = false; // currently visible?

protected:
	Printer(int rows, int cols) noexcept;
	virtual void show_cursor(bool f) noexcept {}
	void		 validate_hpos(bool col80ok) noexcept;
	void		 validate_vpos() noexcept;
};


} // namespace kilipili::Graphics

/*


































*/
