// Copyright (c) 2012 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Canvas.h"
#include "Font.h"
#include "Printer.h"
#undef CHAR_WIDTH

namespace kilipili
{

template<>
class Printer<Canvas> final : public Printer<>
{
public:
	static constexpr int CHAR_WIDTH = 8;

	Printer(Canvas*, const Font* font1 = &latin1_12x8, const Font* font2 = &graphics_12x8) noexcept;
	virtual ~Printer() noexcept override = default;

	// SerialDevice methods overrides:
	// note: using a Printer as a SerialDevice is only a side quest.
	//virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;
	virtual SIZE write(const void* data, SIZE, bool partial = false) noexcept override; // any char, even chr(0)
	virtual void putc(char) noexcept override;											// supports \n, \r and \t
	virtual void puts(cstr) noexcept override;											// supports \n, \r and \t
	//virtual void   printf(cstr fmt, ...) noexcept override __printflike(2, 3);		// supports \n, \r and \t
	//virtual void   printf(cstr fmt, va_list) noexcept override __printflike(2, 0);	// supports \n, \r and \t

	// the normal stuff:
	virtual void reset(bool cls = false) noexcept override;
	virtual cstr identification() override;
	virtual void cls() noexcept override;
	virtual void clearRect(int row, int col, int rows, int cols) noexcept override;
	virtual void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept override;
	virtual void printChar(char c, int count = 1) noexcept override; // no ctl

	// attribute changes apply to following printing:
	virtual void  setAttributes(uint add, uint remove = 0xff) noexcept override;
	virtual uint8 getAttributes() const noexcept override { return attributes; }

	// font changes apply to following printing:
	virtual void		setFont(const Font*, bool gfx = 0) noexcept override;
	virtual const Font* getFont(bool gfx = 0) noexcept override;

	virtual void setPen(uint color, uint ink = 1) noexcept override;
	virtual void setPaper(uint color, uint ink = 0) noexcept override;
	virtual void setPen(Color, uint ink = 1) noexcept override;	  // color = true Color
	virtual void setPaper(Color, uint ink = 0) noexcept override; // color = true Color

	RCPtr<Canvas> pixmap;

private:
	RCPtr<const Font> font1; // default: latin-1
	RCPtr<const Font> font2; // default: graphics font

	uint default_fgcolor = 0;				// black or index
	uint default_bgcolor = 0xffffffff;		// white or index (surplus bits are chopped of)
	uint fgcolor		 = default_fgcolor; // text color
	uint bgcolor		 = default_bgcolor; // paper color
	uint fg_ink			 = 1;				// for Pixmap_wAttr
	uint bg_ink			 = 0;				// for Pixmap_wAttr

	// cursor blob:
	uint32 cursorXorColor = 0; // value used to xor the colors

private:
	using CharMatrix = uint8[24];
	void show_cursor(bool f) noexcept override;
	void printCharMatrix(CharMatrix, int count = 1) noexcept;
	void readBmp(CharMatrix, bool use_fgcolor) noexcept;
	void writeBmp(CharMatrix, uint8 attr) noexcept;
	void getCharMatrix(CharMatrix, char c) noexcept;
	void applyAttributes(CharMatrix) noexcept;
};


// _____________________________________________________________________
// deduction guides:

Printer(Canvas*) -> Printer<Canvas>;


} // namespace kilipili


/*

















*/
