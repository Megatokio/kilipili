// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "USBKeyboard.h"
#include "common/Queue.h"
#include "common/basic_math.h"
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

static HIDKey	 repeat_key		  = NO_KEY;
static Modifiers repeat_modifiers = NO_MODIFIERS;
static CC		 repeat_next;

bool keyEventAvailable() noexcept
{
	if (key_event_queue.avail()) return true;
	if (repeat_key == NO_KEY) return false;
	return CC(time_us_32()) >= repeat_next;
}

KeyEvent getKeyEvent()
{
	// get next KeyEvent
	// if no event handler is installed
	// returns KeyEvent with event.hidkey = NO_KEY and event.down = false  if no event available

	// if (key_event_queue.avail()) return key_event_queue.get();
	// if repeat_key avail return repeat_key.
	// else return KeyEvent {};

	if (key_event_queue.avail())
	{
		const KeyEvent event = key_event_queue.get();

		if (event.hidkey == repeat_key) repeat_key = NO_KEY;
		if (event.down && event.hidkey < KEY_CONTROL_LEFT)
		{
			repeat_key		 = event.hidkey;
			repeat_modifiers = event.modifiers;
			repeat_next		 = CC(time_us_32()) + USB_KEY_DELAY1 * 1000;
		}

		return event;
	}

	if (repeat_key != NO_KEY && CC(time_us_32()) >= repeat_next)
	{
		repeat_next = CC(time_us_32()) + USB_KEY_DELAY * 1000;
		return KeyEvent {true, repeat_modifiers, repeat_key};
	}

	return KeyEvent {};
}

int getChar()
{
	// get next char
	// return -1 if no char available
	// keys with no entry in the translation table are returned as
	//		HID_KEY_OTHER + HidKey + modifiers<<16

	while (keyEventAvailable())
	{
		const KeyEvent event = getKeyEvent();

		if (event.down)
		{
			int c = uchar(event.getchar());
			if (!c) c = HID_KEY_OTHER + event.hidkey + (event.modifiers << 16); // non-priting char
			return c;
		}
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
	repeat_key = NO_KEY;
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
