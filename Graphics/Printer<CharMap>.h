// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "CharMap.h"
#include "Printer.h"
#include "common/RCPtr.h"


namespace kilipili
{

template<>
class Printer<CharMapBase> final : public Printer<>
{
public:
	Printer(CharMapBase*);
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
	virtual cstr identify() override;
	virtual void cls() noexcept override;
	virtual void clearRect(int row, int col, int rows, int cols) noexcept override;
	virtual void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept override;
	virtual void printChar(char c, int count = 1) noexcept override; // no ctl

	// attribute changes apply to following printing:
	virtual void  setAttributes(uint add, uint remove = 0xff) noexcept override;
	virtual uint8 getAttributes() const noexcept override;

	// font changes apply immediately to the whole CharMap:
	virtual void		setFont(const Font*, bool gfx = 0) noexcept override;
	virtual const Font* getFont(bool gfx = 0) noexcept override;

	virtual void setPen(uint color, uint ink = 1) noexcept override { set_fgcolor(color); }
	virtual void setPaper(uint color, uint ink = 0) noexcept override { set_bgcolor(color); }
	virtual void setPen(Color, uint ink = 1) noexcept override;
	virtual void setPaper(Color, uint ink = 0) noexcept override;

	RCPtr<CharMapBase> charmap;

	// print attributes:
	//uint8 fg_bits = charmap->fg_bits;
	//uint8 bg_bits = charmap->bg_bits;
	//uint8 fg_mask = charmap->fg_mask;
	//uint8 bg_mask = charmap->bg_mask;

	void set_fgcolor(uint i) noexcept;
	void set_bgcolor(uint i) noexcept;
	void set_bold(bool) noexcept;
	void set_underline(bool) noexcept;
	void set_graphics(bool) noexcept;

private:
	void show_cursor(bool f) noexcept override;
	void hide_cursor() noexcept;
	void set_attr(int fg, int bg, bool bold, bool ul, bool gra = false) noexcept;
};


// _____________________________________________________________________
// deduction guides:

Printer(CharMapBase*) -> Printer<CharMapBase>;


} // namespace kilipili


/*





























*/
