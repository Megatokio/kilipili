// Copyright (c) 2012 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Devices/SerialDevice.h"
#include "Canvas.h"
#include "TextVDU.h"

namespace kio::Graphics
{

class PicoTerm : public Devices::SerialDevice
{
public:
	using super = Devices::SerialDevice;
	using SIZE	= Devices::SIZE;
	using IoCtl = Devices::IoCtl;

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

	RCPtr<TextVDU> text;

	// write() state machine:
	uint8  repeat_cnt;
	bool   auto_crlf = true;
	uint16 sm_state	 = 0;

	PicoTerm(CanvasPtr);
	PicoTerm(RCPtr<TextVDU>) noexcept;

	/* SerialDevice Interface:
	*/
	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;
	virtual SIZE   read(void*, SIZE, bool = false) override { throw Devices::NOT_READABLE; }
	virtual SIZE   write(const void* data, SIZE, bool partial = false) override;
	//virtual SIZE putc(char) override;
	//virtual SIZE puts(cstr) override;
	//virtual SIZE printf(cstr fmt, ...) override __printflike(2, 3);

	int getc(void (*sm)(), int timeout_us = 0);
	str input_line(void (*sm)(), str oldtext = nullptr, int epos = 0);

	void reset() noexcept;
	using SerialDevice::printf;
	char* identify();

protected:
	// these will stall because USB isn't polled:
	using super::getc;
	using super::gets;
	//virtual char getc() override;
	//virtual str  gets() override;
	//virtual int  getc(uint timeout) override;
};


} // namespace kio::Graphics


/*



























*/
