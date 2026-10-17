// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Printer.h"
#include "SimpleCharMap.h"
#include "common/RCPtr.h"


namespace kilipili
{

template<>
class Printer<SimpleCharMap> final : public Printer<>
{
public:
	Printer(SimpleCharMap*);
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
	uint8 getAttributes() const noexcept override;

	// font changes apply immediately to the whole CharMap:
	void		setFont(const Font*, bool gfx = 0) noexcept override;
	const Font* getFont(bool gfx = 0) noexcept override;

	RCPtr<SimpleCharMap> charmap;

private:
	using super = Printer<>;
	void show_cursor(bool f) noexcept override;
	void hide_cursor() noexcept;
};


// _____________________________________________________________________
// deduction guides:

Printer(SimpleCharMap*) -> Printer<SimpleCharMap>;


} // namespace kilipili
