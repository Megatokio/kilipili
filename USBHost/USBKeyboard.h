// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "HidKeyTables.h"
#include "HidKeys.h"

namespace kio::USB
{

// keyboard keys can generate UCS-2 (wide) chars:
using UCS2Char = uint16;


// getChar() returns non-printing keys in a page of the "private area"
// of the Unicode character set in range 0xE000..0xF8FF:
//		HID_KEY_OTHER + hidkey + modifiers<<16
constexpr UCS2Char HID_KEY_OTHER = 0xE800u;


// LED bit masks
// same as TinyUSB enum hid_keyboard_led_bm_t
// not yet used.
enum KeyboardLED : uint8 {
	LED_NUMLOCK	   = 1 << 0, // Num Lock LED
	LED_CAPSLOCK   = 1 << 1, // Caps Lock LED
	LED_SCROLLLOCK = 1 << 2, // Scroll Lock LED
	LED_COMPOSE	   = 1 << 3, // Composition Mode
	LED_KANA	   = 1 << 4, // Kana mode
};


// The following USBKeyboard functions are used by the defaultHidKeyboardEventHandler().
// A custom HidKeyboardEventHandler can be set with setHidKeyboardEventHandler().


// serialized key event
struct KeyEvent
{
	bool	  down		= false;		// key pressed or released?
	Modifiers modifiers = NO_MODIFIERS; // modifiers after key event (in case of a modifier key per se)
	HIDKey	  hidkey	= NO_KEY;		// USB/HID keycode of key which changed

	KeyEvent() noexcept = default;
	KeyEvent(bool, Modifiers, HIDKey) noexcept;

	char getchar() const noexcept; // returns char(0) for non-printing keys
};

extern void setHidKeyTranslationTable(const HidKeyTable& table); // set localization

using KeyEventHandler = void(const KeyEvent&);
extern void		setKeyEventHandler(KeyEventHandler&); // set a callback, or ...
extern KeyEvent getKeyEvent();						  // ... get next key up/down event
extern int		getChar();							  // ... get next char


} // namespace kio::USB
