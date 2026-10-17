// Copyright (c) 2012 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/Printer.h"
#include "SerialDevice.h"

namespace kilipili
{

/*	class PicoTerm provides a SerialDevice with a simple terminal emulation for use with a Pixmap or CharMap.
	It uses a Printer<> for the actual printing.

	You should prefer printing to the Printer<> directly, as it already provides the SerialDevice interface,
	and use the Printer<> member functions for cursor movement etc. as needed.

	class PicoTerm is deprecated for new development.
*/

class __deprecated PicoTerm : public SerialDevice
{
public:
	using super = SerialDevice;

	// Control Characters:
	enum : uchar {
		RESET				   = 0,
		CLS					   = 1,
		MOVE_TO_POSITION	   = 2,
		MOVE_TO_COL			   = 3,
		SHOW_CURSOR			   = 7,	 //				(BELL)
		CURSOR_LEFT			   = 8,	 // scrolls		(BS)
		TAB					   = 9,	 // scrolls
		CURSOR_DOWN			   = 10, // scrolls		(NL)
		CURSOR_UP			   = 11, // scrolls
		CURSOR_RIGHT		   = 12, // scrolls		(FF)
		RETURN				   = 13, // COL := 0
		CLEAR_TO_END_OF_LINE   = 14,
		CLEAR_TO_END_OF_SCREEN = 15,
		SET_ATTRIBUTES		   = 16,
		XON					   = 17,
		XOFF				   = 19,
		REPEAT_NEXT_CHAR	   = 20,
		SCROLL_SCREEN		   = 21,
		ESC					   = 27,
	};

	RCPtr<Printer<>> printer;

	// write() state machine:
	uint8  repeat_cnt;
	bool   auto_crlf = true;
	uint16 sm_state	 = 0;

	PicoTerm(RCPtr<Printer<>>) noexcept;

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;
	virtual SIZE   read(void*, SIZE, bool = false) override { throw NOT_READABLE; }
	virtual SIZE   write(const void* data, SIZE, bool partial = false) override;

	void reset() noexcept;
	using SerialDevice::printf;
	cstr identify();

protected:
	// these will stall because USB isn't polled:
	using super::getc;
	using super::gets;
};


} // namespace kilipili


/*



























*/
