// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "HidKeyTables.h"
#include "HidKeys.h"
#include "standard_types.h"
#include <class/hid/hid_host.h>
#include <functional>


namespace kio::USB
{

// keyboard keys can generate UCS-2 (wide) chars:
using UCS2Char = uint16;


// Modifier key masks in KeyboardReport.modifiers:
enum Modifiers : uint8 {
	LEFTCTRL   = 1 << 0, // Left Control
	LEFTSHIFT  = 1 << 1, // Left Shift
	LEFTALT	   = 1 << 2, // Left Alt
	LEFTGUI	   = 1 << 3, // Left Window
	RIGHTCTRL  = 1 << 4, // Right Control
	RIGHTSHIFT = 1 << 5, // Right Shift
	RIGHTALT   = 1 << 6, // Right Alt
	RIGHTGUI   = 1 << 7, // Right Window

	NO_MODIFIERS = 0,
	CTRL		 = LEFTCTRL + RIGHTCTRL,
	SHIFT		 = LEFTSHIFT + RIGHTSHIFT,
	ALT			 = LEFTALT + RIGHTALT,
	GUI			 = LEFTGUI + RIGHTGUI
};

inline Modifiers operator|(Modifiers a, Modifiers b) { return Modifiers(uint8(a) | b); }
inline Modifiers operator&(Modifiers a, Modifiers b) { return Modifiers(uint8(a) & b); }


// LED bit masks
// same as TinyUSB enum hid_keyboard_led_bm_t
enum KeyboardLED : uint8 {
	LED_NUMLOCK	   = 1 << 0, // Num Lock LED
	LED_CAPSLOCK   = 1 << 1, // Caps Lock LED
	LED_SCROLLLOCK = 1 << 2, // Scroll Lock LED
	LED_COMPOSE	   = 1 << 3, // Composition Mode
	LED_KANA	   = 1 << 4, // Kana mode
};


// getChar() returns non-printing keys in a page of the "private area"
// of the Unicode character set in range 0xE000..0xF8FF:
// HIDKey + HID_KEY_OTHER = UCS2Char
constexpr UCS2Char HID_KEY_OTHER = 0xE800u;


// USB keyboard report in "boot" mode
// same as TinyUSB struct hid_keyboard_report_t
struct HidKeyboardReport
{
	Modifiers modifiers; // Modifier keys
	uint8	  reserved;	 // Reserved for OEM use, always set to 0
	HIDKey	  keys[6];	 // USB/HID Key codes of the currently pressed keys
};


// serialized key event
struct KeyEvent
{
	bool	  down		= false;		// key pressed or released?
	Modifiers modifiers = NO_MODIFIERS; // modifiers after key event (in case of a modifier key per se)
	HIDKey	  hidkey	= NO_KEY;		// USB/HID keycode of key which changed
	KeyEvent() noexcept = default;
	KeyEvent(bool, Modifiers, HIDKey) noexcept;

	char getchar() const noexcept;
};


using HidKeyboardReportHandler = void(const HidKeyboardReport&);
using KeyEventHandler		   = void(const KeyEvent&);

extern void setHidKeyTranslationTable(const HidKeyTable& table);

inline bool isaModifier(HIDKey key) { return key >= KEY_CONTROL_LEFT && key <= KEY_GUI_RIGHT; }

// There are 6 methods to receive the keyboard input, 3 call backs and 3 functions.
// The application should best stick to a single method.

// callbacks:
extern void setHidKeyboardReportHandler(HidKeyboardReportHandler&);
extern void setKeyEventHandler(KeyEventHandler&);

// functions:
extern KeyEvent getKeyEvent(); // get serialized key up/down event
extern int		getChar();	   // get serialized char. TODO: auto repeat

} // namespace kio::USB

extern cstr tostr(kio::USB::Modifiers, bool left_right_unified = true) noexcept;
