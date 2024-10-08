// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "USBKeyboard.h"
#include "common/Queue.h"
#include "hid_handler.h"
#include <cstring>
#include <glue.h>

#ifndef USB_KEY_DELAY1
  #define USB_KEY_DELAY1 600
#endif
#ifndef USB_KEY_DELAY
  #define USB_KEY_DELAY 60
#endif
#ifndef USB_DEFAULT_KEYTABLE
  #define USB_DEFAULT_KEYTABLE key_table_us
#endif


namespace kio::USB
{

bool ctrl_alt_del_detected = false;

static HidKeyTable key_table = USB_DEFAULT_KEYTABLE;

void setHidKeyTranslationTable(const HidKeyTable& table) { key_table = table; }

// ================================================================

KeyEvent::KeyEvent(bool d, Modifiers m, HIDKey key) noexcept : down(d), modifiers(m), hidkey(key) {}

char KeyEvent::getchar() const noexcept
{
	char c = key_table.get_key(hidkey, modifiers & SHIFT, modifiers & ALT);
	if (modifiers & CTRL) c &= 0x1f;
	return c;
}

// ================================================================

static Queue<KeyEvent, 8, uint8> key_event_queue;
static_assert(sizeof(key_event_queue) == 2 + 3 * 8);

bool keyEventAvailable() noexcept
{
	return key_event_queue.avail(); //
}

KeyEvent getKeyEvent()
{
	// get next KeyEvent
	// if no event handler is installed
	// returns KeyEvent with event.hidkey = NO_KEY and event.down = false  if no event available

	if (key_event_queue.avail()) return key_event_queue.get();
	else return KeyEvent {};
}

static HIDKey getchar_repeat_key  = NO_KEY;
static int	  getchar_repeat_char = 0;
static int32  getchar_repeat_next = 0;

int getChar()
{
	// get next char
	// return -1 if no char available
	// keys with no entry in the translation table are returned as
	//		HID_KEY_OTHER + HidKey + modifiers<<16

	while (key_event_queue.avail())
	{
		const KeyEvent event = key_event_queue.get();

		if (event.down)
		{
			int c = uchar(event.getchar());
			if (!c) c = HID_KEY_OTHER + event.hidkey + (event.modifiers << 16); // non-priting char

			getchar_repeat_key	= event.hidkey;
			getchar_repeat_char = c;
			getchar_repeat_next = int(time_us_32()) + USB_KEY_DELAY1 * 1000;
			return c;
		}
		else
		{
			if (event.hidkey == getchar_repeat_key) getchar_repeat_key = NO_KEY;
		}
	}

	if (getchar_repeat_key && int(time_us_32()) - getchar_repeat_next >= 0)
	{
		getchar_repeat_next = int(time_us_32()) + USB_KEY_DELAY * 1000;
		return getchar_repeat_char;
	}

	return -1;
}

static void pushKeyEvent(const KeyEvent& event) noexcept
{
	// store KeyEvent for getChar() or getKeyEvent():

	if unlikely (event.modifiers == LEFTCTRL + LEFTALT)
		if (event.down && (event.hidkey == KEY_BACKSPACE || event.hidkey == KEY_DELETE)) ctrl_alt_del_detected = true;

	if unlikely (key_event_queue.free() == 0) key_event_queue.get(); // discard oldest
	key_event_queue.put(event);
}

static KeyEventHandler* key_event_handler = &pushKeyEvent;

void setKeyEventHandler(KeyEventHandler* handler)
{
	key_event_handler = handler ? handler : &pushKeyEvent;
	key_event_queue.flush();
	getchar_repeat_key = NO_KEY;
	setHidKeyboardEventHandler(&defaultHidKeyboardEventHandler);
}

static bool find(const HidKeyboardReport& report, HIDKey key)
{
	return memchr(report.keys, key, sizeof(report.keys)) != nullptr;
}

void __weak_symbol defaultHidKeyboardEventHandler(const HidKeyboardReport& new_report) noexcept
{
	// this handler is called by `tuh_hid_report_received_cb()`
	// which receives the USB Host events

	static HidKeyboardReport old_report;
	for (uint i = 0; i < 6; i++)
	{
		HIDKey key = old_report.keys[i];
		if (key && !find(new_report, key)) key_event_handler(KeyEvent(false, old_report.modifiers, key));
	}
	for (uint i = 0; i < 6; i++)
	{
		HIDKey key = new_report.keys[i];
		if (key && !find(old_report, key)) key_event_handler(KeyEvent(true, new_report.modifiers, key));
	}
	old_report = new_report;
}


} // namespace kio::USB


/*






























*/
