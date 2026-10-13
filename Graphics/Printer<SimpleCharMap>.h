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

	void  reset(bool cls = false) noexcept override;
	cstr  identify() override;
	void  clearRect(int row, int col, int rows, int cols) noexcept override;
	void  copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept override;
	void  printChar(char c, int count = 1) noexcept override; // no ctl
	void  print(cstr text) noexcept override;				  // supports \n and \t
	void  write(cptr text, int count) noexcept override;	  // any char, even chr(0)
	void  setAttributes(uint add, uint remove = 0xff) noexcept override;
	uint8 getAttributes() const noexcept override;

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
