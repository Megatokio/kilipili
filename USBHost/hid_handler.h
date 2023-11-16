// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "HidKeys.h"
#include <class/hid/hid.h>

namespace kio::USB
{

// USB keyboard report in "boot" mode
struct HidKeyboardReport
{
	Modifiers modifiers; // Modifier keys
	uint8	  reserved;	 // Reserved for OEM use, always set to 0
	HIDKey	  keys[6];	 // USB/HID Key codes of the currently pressed keys
};


// USB mouse report in "boot" mode
struct HidMouseReport
{
	uint8 buttons; // mask with currently pressed buttons
	int8  dx;	   // x movement
	int8  dy;	   // y movement
	int8  wheel;   // wheel movement
	int8  pan;	   // using AC Pan
};


using HidMouseEventHandler	  = void(const HidMouseReport&) noexcept;
using HidKeyboardEventHandler = void(const HidKeyboardReport&) noexcept;


/*
	set the mouse event handler to be called by `tuh_hid_report_received_cb()`.
	pass nullptr to reset to the default handler.
	the default handler is in USBMouse.cpp.
*/
extern void setHidMouseEventHandler(HidMouseEventHandler*) noexcept;

/*
	set the keyboard event handler to be called by `tuh_hid_report_received_cb()`.
	pass nullptr to reset to the default handler.
	the default handler is in USBKeyboard.cpp.
*/
extern void setHidKeyboardEventHandler(HidKeyboardEventHandler*) noexcept;


// the default handlers in USBMouse.cpp and USBKeyboard.cpp:
extern HidKeyboardEventHandler defaultHidKeyboardEventHandler;
extern HidMouseEventHandler	   defaultHidMouseEventHandler;


} // namespace kio::USB
