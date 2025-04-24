// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "USBKeyboard.h"
#include "common/Queue.h"
#include "common/basic_math.h"
#include "common/cdefs.h"
#include "common/timing.h"
#include "hid_handler.h"
#include <cstdio>
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

bool keyEventAvailable(bool autorepeat) noexcept
{
	if (key_event_queue.avail()) return true;
	return (autorepeat && repeat_key != NO_KEY && now() >= repeat_next);
}

KeyEvent getKeyEvent(bool autorepeat)
{
	// get next KeyEvent
	// if no KeyEventHandler is installed

	// if (key_event_queue.avail()) return key_event_queue.get();
	// if autorepeat and repeat_key available return repeat_key.
	// else return KeyEvent with event.hidkey = NO_KEY and event.down = false

	if (key_event_queue.avail())
	{
		const KeyEvent event = key_event_queue.get();

		if (event.hidkey == repeat_key) repeat_key = NO_KEY;
		if (event.down && !isaModifier(event.hidkey))
		{
			repeat_key		 = event.hidkey;
			repeat_modifiers = event.modifiers;
			repeat_next		 = now() + USB_KEY_DELAY1 * 1000;
		}

		return event;
	}

	if (autorepeat && repeat_key != NO_KEY && now() >= repeat_next)
	{
		repeat_next = now() + USB_KEY_DELAY * 1000;
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
	// does not return key up and modifier events

	while (keyEventAvailable())
	{
		const KeyEvent event = getKeyEvent();

		if (event.down && !isaModifier(event.hidkey))
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
	__sev();
}

static KeyEventHandler* key_event_handler = &pushKeyEvent;

KeyEventHandler* setKeyEventHandler(KeyEventHandler* handler)
{
	auto* old_handler = key_event_handler;
	key_event_handler = handler ? handler : &pushKeyEvent;
	key_event_queue.flush();
	repeat_key = NO_KEY;
	setHidKeyboardEventHandler(&defaultHidKeyboardEventHandler);
	return old_handler;
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
	Modifiers				 modifiers = old_report.modifiers;

	for (uint i = 0; i < 6; i++)
	{
		HIDKey key = old_report.keys[i];
		if (key && !find(new_report, key)) key_event_handler(KeyEvent(false, modifiers, key));
	}

	while (uint8 toggled = modifiers ^ new_report.modifiers)
	{
		uint i	  = msbit(toggled);
		modifiers = Modifiers(modifiers ^ (1 << i));
		bool down = modifiers & (1 << i);

		key_event_handler(KeyEvent(down, modifiers, HIDKey(KEY_CONTROL_LEFT + i)));
	}

	for (uint i = 0; i < 6; i++)
	{
		HIDKey key = new_report.keys[i];
		if (key && !find(old_report, key)) key_event_handler(KeyEvent(true, modifiers, key));
	}

	old_report = new_report;
}


} // namespace kio::USB


/*






























*/
