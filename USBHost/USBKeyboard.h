// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "HidKeyTables.h"
#include "HidKeys.h"


/**	USB Keyboard interface
  
	There are 4 possible modes of operation:
	
	(1) Poll characters with `getChar()`. 
		`getChar()` returns characters from the hidKeyTranslationTable or USB HID keycodes for non-printing keys.
		`getChar()` returns non-printing keys in a page of the "private area" of the Unicode character set:
			HID_KEY_OTHER + hidkey + modifiers<<16
		The default hidKeyTranslationTable is set from `#define USB_DEFAULT_KEYTABLE` and can be changed with 
			`setHidKeyTranslationTable()`.		

	(2) Poll KeyEvents with `getKeyEvent()`. This gives you the same information as above only more directly.
		`getKeyEvent()` also returns key-up events.
	
	(3) Set a callback function with `setKeyEventHandler()` which will be called whenever a key event is received.
		The two above functions become dead.
	
	(4) Set a custom HidKeyboardEventHandler with `setHidKeyboardEventHandler()`, found in hid_handler.h.
		The three above functions become dead and you are on your own.
*/


namespace kio::USB
{

// keyboard keys can generate UCS-2 (wide) chars:
using UCS2Char = uint16;

constexpr UCS2Char HID_KEY_OTHER = 0xE800u;

// LED bit masks
// same as TinyUSB enum hid_keyboard_led_bm_t
// not used.
enum KeyboardLED : uint8 {
	LED_NUMLOCK	   = 1 << 0, // Num Lock LED
	LED_CAPSLOCK   = 1 << 1, // Caps Lock LED
	LED_SCROLLLOCK = 1 << 2, // Scroll Lock LED
	LED_COMPOSE	   = 1 << 3, // Composition Mode
	LED_KANA	   = 1 << 4, // Kana mode
};

struct KeyEvent
{
	bool	  down		= false;		// key pressed or released?
	Modifiers modifiers = NO_MODIFIERS; // modifiers after key event (in case of a modifier key per se)
	HIDKey	  hidkey	= NO_KEY;		// USB/HID keycode of key which changed

	KeyEvent() noexcept = default;
	KeyEvent(bool, Modifiers, HIDKey) noexcept;

	char getchar() const noexcept; // returns char(0) for non-printing keys
};

extern bool ctrl_alt_del_detected; // ctrl+alt+delete or ctrl+alt+bs detected

extern void setHidKeyTranslationTable(const HidKeyTable& table);

using KeyEventHandler = void(const KeyEvent&);
extern void		setKeyEventHandler(KeyEventHandler*); // set a callback, or ...
extern KeyEvent getKeyEvent();						  // ... get next key up/down event
extern int		getChar();							  // ... get next char
extern bool		keyEventAvailable() noexcept;

} // namespace kio::USB
