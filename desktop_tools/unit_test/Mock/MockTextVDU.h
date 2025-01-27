// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "TextVDU.h"
#include "common/Array.h"
#undef CHAR_WIDTH


namespace kio::Graphics::Mock
{

class TextVDU : public kio::Graphics::TextVDU
{
public:
	using super = kio::Graphics::TextVDU;

	static constexpr int CHAR_WIDTH	 = super::CHAR_WIDTH;
	static constexpr int CHAR_HEIGHT = super::CHAR_HEIGHT;

	//using CharMatrix						  = TextVDU::CharMatrix;
	//using Attributes						  = TextVDU::Attributes;
	//using AutoWrap							  = TextVDU::AutoWrap;
	//static constexpr AutoWrap	wrap		  = TextVDU::wrap;
	//static constexpr AutoWrap	nowrap		  = TextVDU::nowrap;
	//static constexpr Attributes NORMAL		  = TextVDU::NORMAL;
	//static constexpr Attributes BOLD		  = TextVDU::BOLD;
	//static constexpr Attributes UNDERLINE	  = TextVDU::UNDERLINE;
	//static constexpr Attributes INVERTED	  = TextVDU::INVERTED;
	//static constexpr Attributes ITALIC		  = TextVDU::ITALIC;
	//static constexpr Attributes TRANSPARENT	  = TextVDU::TRANSPARENT;
	//static constexpr Attributes DOUBLE_WIDTH  = TextVDU::DOUBLE_WIDTH;
	//static constexpr Attributes DOUBLE_HEIGHT = TextVDU::DOUBLE_HEIGHT;
	//static constexpr Attributes GRAPHICS	  = TextVDU::GRAPHICS;

	// log the mock:
	mutable Array<cstr> log; // tempstr

	TextVDU(CanvasPtr) noexcept;

	void reset() noexcept;
	void cls() noexcept;
	void identify() noexcept;
	void moveTo(int row, int col, AutoWrap = nowrap) noexcept;
	void moveToCol(int col, AutoWrap = nowrap) noexcept;
	void moveToRow(int row, AutoWrap = nowrap) noexcept;
	void setCharAttributes(uint add, uint remove = 0xff) noexcept;
	void addCharAttributes(uint a) noexcept { setCharAttributes(a, 0); }
	void removeCharAttributes(uint a = 0xff) noexcept { setCharAttributes(0, a); }
	void printCharMatrix(CharMatrix, int count = 1) noexcept;
	void printChar(char c, int count = 1) noexcept;				// no ctl
	void print(cstr text) noexcept;								// supports \n and \t
	void printf(cstr fmt, ...) noexcept __printflike(2, 3);		// supports \n and \t
	void printf(cstr fmt, va_list) noexcept __printflike(2, 0); // supports \n and \t
	str	 inputLine(std::function<int()> getchar, str oldtext = nullptr, int epos = 0);
	void cursorLeft(int count = 1, AutoWrap = wrap) noexcept;
	void cursorRight(int count = 1, AutoWrap = wrap) noexcept;
	void cursorUp(int count = 1, AutoWrap = wrap) noexcept;
	void cursorDown(int count = 1, AutoWrap = wrap) noexcept;
	void cursorTab(int count = 1) noexcept;
	void cursorReturn() noexcept;
	void newLine() noexcept;
	void showCursor(bool on = true) noexcept;
	void hideCursor() noexcept;
	void validateCursorPosition(bool col80ok) noexcept;
	void limitCursorPosition() noexcept;
	void readBmp(CharMatrix, bool use_fgcolor) noexcept;
	void writeBmp(CharMatrix, uint8 attr) noexcept;
	void getCharMatrix(CharMatrix, char c) noexcept;
	void getGraphicsCharMatrix(CharMatrix, char c) noexcept;
	void applyAttributes(CharMatrix) noexcept;

	void clearRect(int row, int col, int rows, int cols) noexcept;
	void clearToStartOfLine(bool incl_cursorpos = 0) noexcept;
	void clearToStartOfScreen(bool incl_cursorpos = 0) noexcept;
	void clearToEndOfLine() noexcept;
	void clearToEndOfScreen() noexcept;

	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept;

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

private:
	//	void show_cursor(bool f) noexcept;
	//	void validate_hpos(bool col80ok) noexcept;
	//	void validate_vpos() noexcept;
};


} // namespace kio::Graphics::Mock

/*








*/
