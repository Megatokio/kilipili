// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Mock/TextVDU.h"
#include "cstrings.h"
#include <stdarg.h>

#define LOG(...) log.append(usingstr(__VA_ARGS__))

namespace kio::Graphics::Mock
{
using namespace Graphics;

static inline cstr tostr(bool f) { return f ? "true" : "false"; }
static inline cstr tostr(TextVDU::AutoWrap w) { return w ? "wrap" : "nowrap"; }


TextVDU::TextVDU(CanvasPtr pixmap) noexcept : super(pixmap) //
{
	LOG("%s(pixmap)", __func__);
}

void TextVDU::reset() noexcept
{
	LOG("%s()", __func__);
	return super::reset();
}

void TextVDU::cls() noexcept
{
	LOG("%s()", __func__);
	return super::cls();
}

void TextVDU::identify() noexcept
{
	LOG("%s()", __func__);
	return super::identify();
}

void TextVDU::showCursor(bool on) noexcept
{
	LOG("%s(%s)", __func__, tostr(on));
	return super::showCursor(on);
}

void TextVDU::hideCursor() noexcept
{
	LOG("%s()", __func__);
	return super::hideCursor();
}

void TextVDU::validateCursorPosition(bool col80ok) noexcept
{
	LOG("%s(%s)", __func__, tostr(col80ok));
	return super::validateCursorPosition(col80ok);
}

void TextVDU::limitCursorPosition() noexcept
{
	LOG("%s()", __func__);
	return super::limitCursorPosition();
}

void TextVDU::moveTo(int row, int col, AutoWrap auto_wrap) noexcept
{
	LOG("%s(%i,%i,%s)", __func__, row, col, tostr(auto_wrap));
	return super::moveTo(row, col, auto_wrap);
}

void TextVDU::moveToCol(int col, AutoWrap auto_wrap) noexcept
{
	LOG("%s(%i,%s)", __func__, col, tostr(auto_wrap));
	return super::moveToCol(col, auto_wrap);
}

void TextVDU::moveToRow(int row, AutoWrap auto_wrap) noexcept
{
	LOG("%s(%i,%s)", __func__, row, tostr(auto_wrap));
	return super::moveToRow(row, auto_wrap);
}

void TextVDU::cursorLeft(int count, AutoWrap auto_wrap) noexcept
{
	LOG("%s(%i,%s)", __func__, count, tostr(auto_wrap));
	return super::cursorLeft(count, auto_wrap);
}

void TextVDU::cursorRight(int count, AutoWrap auto_wrap) noexcept
{
	LOG("%s(%i,%s)", __func__, count, tostr(auto_wrap));
	return super::cursorRight(count, auto_wrap);
}

void TextVDU::cursorUp(int count, AutoWrap auto_wrap) noexcept
{
	LOG("%s(%i,%s)", __func__, count, tostr(auto_wrap));
	return super::cursorUp(count, auto_wrap);
}

void TextVDU::cursorDown(int count, AutoWrap auto_wrap) noexcept
{
	LOG("%s(%i,%s)", __func__, count, tostr(auto_wrap));
	return super::cursorDown(count, auto_wrap);
}

void TextVDU::cursorTab(int count) noexcept
{
	LOG("%s(%i)", __func__, count);
	return super::cursorTab(count);
}

void TextVDU::cursorReturn() noexcept
{
	LOG("%s()", __func__);
	return super::cursorReturn();
}

void TextVDU::newLine() noexcept
{
	LOG("%s()", __func__);
	return super::newLine();
}

void TextVDU::clearRect(int row, int col, int rows, int cols) noexcept
{
	LOG("%s(%i,%i,%i,%i)", __func__, row, col, rows, cols);
	return super::clearRect(row, col, rows, cols);
}

void TextVDU::scrollRect(int row, int col, int rows, int cols, int dy, int dx) noexcept
{
	LOG("%s(%i,%i,%i,%i,%i,%i)", __func__, row, col, rows, cols, dy, dx);
	return super::scrollRect(row, col, rows, cols, dy, dx);
}

void TextVDU::scrollRectLeft(int row, int col, int rows, int cols, int dist) noexcept
{
	LOG("%s(%i,%i,%i,%i,%i)", __func__, row, col, rows, cols, dist);
	return super::scrollRectLeft(row, col, rows, cols, dist);
}

void TextVDU::scrollRectRight(int row, int col, int rows, int cols, int dist) noexcept
{
	LOG("%s(%i,%i,%i,%i,%i)", __func__, row, col, rows, cols, dist);
	return super::scrollRectRight(row, col, rows, cols, dist);
}

void TextVDU::scrollRectUp(int row, int col, int rows, int cols, int dist) noexcept
{
	LOG("%s(%i,%i,%i,%i,%i)", __func__, row, col, rows, cols, dist);
	return super::scrollRectUp(row, col, rows, cols, dist);
}

void TextVDU::scrollRectDown(int row, int col, int rows, int cols, int dist) noexcept
{
	LOG("%s(%i,%i,%i,%i,%i)", __func__, row, col, rows, cols, dist);
	return super::scrollRectDown(row, col, rows, cols, dist);
}

void TextVDU::insertRows(int n) noexcept
{
	LOG("%s(%i)", __func__, n);
	return super::insertRows(n);
}

void TextVDU::deleteRows(int n) noexcept
{
	LOG("%s(%i)", __func__, n);
	return super::deleteRows(n);
}

void TextVDU::insertColumns(int n) noexcept
{
	LOG("%s(%i)", __func__, n);
	return super::insertColumns(n);
}

void TextVDU::deleteColumns(int n) noexcept
{
	LOG("%s(%i)", __func__, n);
	return super::deleteColumns(n);
}

void TextVDU::insertChars(int n) noexcept
{
	LOG("%s(%i)", __func__, n);
	return super::insertChars(n);
}

void TextVDU::deleteChars(int n) noexcept
{
	LOG("%s(%i)", __func__, n);
	return super::deleteChars(n);
}

void TextVDU::clearToStartOfLine(bool incl_cpos) noexcept
{
	LOG("%s(%s)", __func__, tostr(incl_cpos));
	return super::clearToStartOfLine(incl_cpos);
}

void TextVDU::clearToStartOfScreen(bool incl_cpos) noexcept
{
	LOG("%s(%s)", __func__, tostr(incl_cpos));
	return super::clearToStartOfScreen(incl_cpos);
}

void TextVDU::clearToEndOfLine() noexcept
{
	LOG("%s()", __func__);
	return super::clearToEndOfLine();
}

void TextVDU::clearToEndOfScreen() noexcept
{
	LOG("%s()", __func__);
	return super::clearToEndOfScreen();
}

void TextVDU::copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept
{
	LOG("%s(%i,%i,%i,%i,%i,%i)", __func__, dest_row, dest_col, src_row, src_col, rows, cols);
	return super::copyRect(dest_row, dest_col, src_row, src_col, rows, cols);
}

void TextVDU::scrollScreen(int dy /*chars*/, int dx /*chars*/) noexcept
{
	LOG("%s(%i,%i)", __func__, dy, dx);
	return super::scrollScreen(dy, dx);
}

void TextVDU::setCharAttributes(uint add, uint remove) noexcept
{
	LOG("%s(%02x,%x)", __func__, add, remove);
	return super::setAttributes(add, remove);
}

void TextVDU::applyAttributes(CharMatrix bmp) noexcept
{
	LOG("%s(bmp)", __func__);
	return super::applyAttributes(bmp);
}

void TextVDU::readBmp(CharMatrix bmp, bool use_fgcolor) noexcept
{
	LOG("%s(bmp,%s)", __func__, tostr(use_fgcolor));
	return super::readBmp(bmp, use_fgcolor);
}

void TextVDU::writeBmp(CharMatrix bmp, uint8 attr) noexcept
{
	LOG("%s(bmp,%x)", __func__, attr);
	return super::writeBmp(bmp, attr);
}

void TextVDU::getCharMatrix(CharMatrix charmatrix, char cc) noexcept
{
	LOG("%s(bu,'%c')", __func__, cc);
	return super::getCharMatrix(charmatrix, cc);
}

void TextVDU::getGraphicsCharMatrix(CharMatrix charmatrix, char cc) noexcept
{
	LOG("%s(bu,'%c')", __func__, cc);
	return super::getGraphicsCharMatrix(charmatrix, cc);
}

void TextVDU::printCharMatrix(CharMatrix charmatrix, int count) noexcept
{
	LOG("%s(bu,%i)", __func__, count);
	return super::printCharMatrix(charmatrix, count);
}

void TextVDU::printChar(char c, int count) noexcept
{
	if (c >= 0x20 && c < 0x7f) LOG("%s('%c',%i)", __func__, c, count);
	else LOG("%s(0x%02x,%i)", __func__, uchar(c), count);

	return super::printChar(c, count);
}

void TextVDU::print(cstr s) noexcept
{
	LOG("%s(%s)", __func__, s);
	return super::print(s);
}

void TextVDU::printf(cstr fmt, va_list va) noexcept
{
	LOG("%s(fmt,va)", __func__);
	return super::printf(fmt, va);
}

void TextVDU::printf(cstr fmt, ...) noexcept
{
	//LOG("%s(fmt,...)", __func__);

	va_list va;

	va_start(va, fmt);
	LOG("printf(%s)", usingstr(fmt, va));
	va_end(va);

	va_start(va, fmt);
	super::printf(fmt, va);
	va_end(va);
}

str TextVDU::inputLine(std::function<int()> getc, str oldtext, int epos)
{
	LOG("%s(f(),\"%s\",%i)", __func__, oldtext, epos);
	return super::inputLine(getc, oldtext, epos);
}

void TextVDU::scrollScreenUp(int rows) noexcept
{
	LOG("%s(%i)", __func__, rows);
	return super::scrollScreenUp(rows);
}

void TextVDU::scrollScreenDown(int rows) noexcept
{
	LOG("%s(%i)", __func__, rows);
	return super::scrollScreenDown(rows);
}

void TextVDU::scrollScreenLeft(int cols) noexcept
{
	LOG("%s(%i)", __func__, cols);
	return super::scrollScreenLeft(cols);
}

void TextVDU::scrollScreenRight(int cols) noexcept
{
	LOG("%s(%i)", __func__, cols);
	return super::scrollScreenRight(cols);
}


} // namespace kio::Graphics::Mock

/*




























*/
