// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "CharMap.h"
#include "Printer.h"
#include "common/RCPtr.h"


namespace kilipili::Graphics
{

template<>
class Printer<CharMapBase> final : public Printer<>
{
public:
	Printer(CharMapBase*);

	void reset(bool cls = false) noexcept override;
	cstr identify() override;
	void clearRect(int row, int col, int rows, int cols) noexcept override;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept override;
	void printChar(char c, int count = 1) noexcept override; // no ctl
	void print(cstr text) noexcept override;				 // supports \n and \t
	void write(cptr text, int count) noexcept override;		 // any char, even chr(0)
	void setAttributes(uint add, uint remove = 0xff) noexcept override;

	RCPtr<CharMapBase> charmap;

	// print attributes:
	uint8 fg_bits	= charmap->fg_bits;
	uint8 bg_bits	= charmap->bg_bits;
	uint8 fg_mask	= charmap->fg_mask;
	uint8 bg_mask	= charmap->bg_mask;
	uint8 bold		= charmap->bold;	  // mask
	uint8 underline = charmap->underline; // mask
	uint8 graphics	= charmap->graphics;  // mask
	char  _padding	= 0;
	enum Attributes : uint8 {
		normal		  = 0,
		inverted	  = 0, // CharMap::inverted
		italic		  = 0,
		transparent	  = 0,
		double_width  = 0,
		double_height = 0,
	};

	void set_fgcolor(int i) noexcept;
	void set_bgcolor(int i) noexcept;
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


} // namespace kilipili::Graphics


/*





























*/
