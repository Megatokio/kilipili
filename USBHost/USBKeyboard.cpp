// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "USBKeyboard.h"
#include "common/Queue.h"
#include "common/tempmem.h"
#include "hid_handler.h"
#include <pico/stdlib.h>

#ifndef USB_KEY_DELAY1
  #define USB_KEY_DELAY1 600
#endif
#ifndef USB_KEY_DELAY
  #define USB_KEY_DELAY 60
#endif

static constexpr int numbits(uint8 z)
{
	z = ((z >> 1) & 0x55u) + (z & 0x55u);
	z = ((z >> 2) & 0x33u) + (z & 0x33u);
	return (z >> 4) + (z & 0x0Fu);
}

cstr tostr(kio::USB::Modifiers mod, bool unified) noexcept
{
	//LEFTCTRL   = 1 << 0, // Left Control
	//LEFTSHIFT  = 1 << 1, // Left Shift
	//LEFTALT	 = 1 << 2, // Left Alt
	//LEFTGUI	 = 1 << 3, // Left Window
	//RIGHTCTRL  = 1 << 4, // Right Control
	//RIGHTSHIFT = 1 << 5, // Right Shift
	//RIGHTALT   = 1 << 6, // Right Alt
	//RIGHTGUI   = 1 << 7, // Right Window

	if (mod == kio::USB::NO_MODIFIERS) return "NONE";
	static constexpr char names[4][7] = {"CTRL+", "SHIFT+", "ALT+", "GUI+"};

	char  bu[28]; // max 27: "lCTRL+lSHIFT+rCTRL+rSHIFT", '+', chr(0)
	char* p = bu;

	if (unified || numbits(mod) > 4)
	{
		uint mask = 0x11;
		for (uint i = 0; i < 4; i++)
		{
			if (mod & (mask << i))
			{
				strcpy(p, names[i]);
				p = strchr(p, 0);
			}
		}
	}
	else
	{
		uint mask = 0x01;
		for (uint i = 0; i < 8; i++)
		{
			if (mod & (mask << i))
			{
				*p++ = i < 4 ? 'l' : 'r';
				strcpy(p, names[i & 3]);
				p = strchr(p, 0);
			}
		}
	}

	assert(p > bu);
	*--p = 0;
	return kio::dupstr(bu);
}


namespace kio::USB
{

static HidKeyTable key_table = key_table_us;

static Queue<KeyEvent, 8, uint8> key_event_queue;
static_assert(sizeof(key_event_queue) == 2 + 3 * 8);


// ================================================================

static bool find(const HidKeyboardReport& report, HIDKey key)
{
	return memchr(report.keys, key, sizeof(report.keys)) != nullptr;
}

KeyEvent::KeyEvent(bool d, Modifiers m, HIDKey key) noexcept : down(d), modifiers(m), hidkey(key) {}

char KeyEvent::getchar() const noexcept
{
	char c = key_table.get_key(hidkey, modifiers & SHIFT, modifiers & ALT);
	if (modifiers & CTRL) c &= 0x1f;
	return c;
}


// ================================================================

void setHidKeyTranslationTable(const HidKeyTable& table) { key_table = table; }

KeyEvent getKeyEvent()
{
	// get next KeyEvent
	// if no event handler is installed
	// returns KeyEvent with event.hidkey = NO_KEY if no event available

	if (key_event_queue.avail()) return key_event_queue.get();
	else return KeyEvent {};
}

int getChar()
{
	// get next char
	// return -1 if no char available
	// keys with no entry in the translation table are returned as
	//		HID_KEY_OTHER + HidKey + modifiers<<16

	static HIDKey repeat_key  = NO_KEY;
	static int	  repeat_char = 0;
	static int32  repeat_next = 0;

	while (key_event_queue.avail())
	{
		const KeyEvent event = key_event_queue.get();

		if (event.down)
		{
			int c = uchar(event.getchar());
			if (!c) c = HID_KEY_OTHER + event.hidkey + (event.modifiers << 16); // non-priting char

			repeat_key	= event.hidkey;
			repeat_char = c;
			repeat_next = int(time_us_32()) + USB_KEY_DELAY1 * 1000;
			return c;
		}
		else
		{
			if (event.hidkey == repeat_key) repeat_key = NO_KEY;
		}
	}

	if (repeat_key && int(time_us_32()) - repeat_next >= 0)
	{
		repeat_next = int(time_us_32()) + USB_KEY_DELAY * 1000;
		return repeat_char;
	}

	return -1;
}

static void push_key_event(const KeyEvent& event)
{
	// store KeyEvent for getChar() or getKeyEvent():
	if unlikely (key_event_queue.free() == 0) key_event_queue.get(); // discard oldest
	key_event_queue.put(event);
}

static HidKeyboardReportHandler* hid_keyboard_report_cb = nullptr;
static KeyEventHandler*			 key_event_cb			= &push_key_event;

static void reset_handlers()
{
	hid_keyboard_report_cb		 = nullptr;
	key_event_cb				 = &push_key_event;
	hid_keyboard_report_t report = {0, 0, {0}};
	handle_hid_keyboard_event(&report);
	while (key_event_queue.avail()) key_event_queue.get(); // drain queue
}

void setHidKeyboardReportHandler(HidKeyboardReportHandler& handler)
{
	reset_handlers();
	hid_keyboard_report_cb = handler;
}

void setKeyEventHandler(KeyEventHandler& handler)
{
	reset_handlers();
	if (handler) key_event_cb = handler;
}

static void handle_key_event(const HidKeyboardReport& new_report, const HidKeyboardReport& old_report, bool down)
{
	for (uint i = 0; i < 6; i++)
	{
		HIDKey key = new_report.keys[i];
		if (key && !find(old_report, key)) key_event_cb(KeyEvent(down, new_report.modifiers, key));
	}
}

void handle_hid_keyboard_event(const hid_keyboard_report_t* report) noexcept
{
	// this handler is called by `tuh_hid_report_received_cb()`
	// which receives the USB Host events

	assert(report);
	static_assert(sizeof(HidKeyboardReport) == sizeof(hid_keyboard_report_t));
	const HidKeyboardReport& new_report = *reinterpret_cast<const HidKeyboardReport*>(report);

	if (hid_keyboard_report_cb) hid_keyboard_report_cb(new_report);
	else
	{
		static HidKeyboardReport old_report;
		handle_key_event(old_report, new_report, false); // find & handle key up events
		handle_key_event(new_report, old_report, true);	 // find & handle key down events
		old_report = new_report;
	}
}

} // namespace kio::USB
