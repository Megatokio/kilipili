// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Printer.h"
#include "SimpleCharMap.h"
#include "common/RCPtr.h"


namespace kilipili::Graphics
{

template<>
class Printer<SimpleCharMap> final : public Printer<>
{
public:
	Printer(SimpleCharMap*);

	void reset(bool cls = false) noexcept override;
	cstr identify() override;
	void clearRect(int row, int col, int rows, int cols) noexcept override;
	void copyRect(int dest_row, int dest_col, int src_row, int src_col, int rows, int cols) noexcept override;
	void printChar(char c, int count = 1) noexcept override; // no ctl
	void print(cstr text) noexcept override;				 // supports \n and \t
	void write(cptr text, int count) noexcept override;		 // any char, even chr(0)
	void setAttributes(uint add, uint remove = 0xff) noexcept override;

	RCPtr<SimpleCharMap> charmap;

	// print attributes:
	enum Attributes : uint8 {
		normal		  = 0,
		bold		  = 0x80, // SimpleCharMap::bold
		underline	  = 0,
		inverted	  = 0x80, // SimpleCharMap::inverted
		italic		  = 0,
		transparent	  = 0,
		double_width  = 0,
		double_height = 0,
		graphics	  = 0
	};

private:
	using super = Printer<>;
	void show_cursor(bool f) noexcept override;
	void hide_cursor() noexcept;
};


inline void Printer<SimpleCharMap>::setAttributes(uint add, uint remove) noexcept
{
	attributes &= ~remove;
	attributes |= add & 0x80;
}


// _____________________________________________________________________
// deduction guides:

Printer(SimpleCharMap*) -> Printer<SimpleCharMap>;
//Printer(RCPtr<SimpleCharMap>) -> Printer<SimpleCharMap>;

} // namespace kilipili::Graphics
