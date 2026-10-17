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
	void reset(bool cls = false) noexcept override;
	cstr identify() override;
	void cls() noexcept override;
	void clearRect(int row, int col, int rows, int cols) noexcept override;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept override;
	void printChar(char c, int count = 1) noexcept override; // no ctl

	// attribute changes apply to following printing:
	void  setAttributes(uint add, uint remove = 0xff) noexcept override;
	uint8 getAttributes() const noexcept override { return attributes; }

	// font changes apply to following printing:
	void		setFont(const Font*, bool gfx = 0) noexcept override;
	const Font* getFont(bool gfx = 0) noexcept override;

	RCPtr<Canvas>	  pixmap;
	RCPtr<const Font> font1; // default: latin-1
	RCPtr<const Font> font2; // default: graphics font

	uint default_bgcolor = 0x0000ffff; // white paper (note: ic1 has inverted default colormap)
	uint default_fgcolor = 0;		   // black ink   (      => black paper & green ink)

	uint bgcolor = default_bgcolor; // paper color
	uint fgcolor = default_fgcolor; // text color
	uint fg_ink	 = 1;				// for Pixmap_wAttr
	uint bg_ink	 = 0;				// for Pixmap_wAttr

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
