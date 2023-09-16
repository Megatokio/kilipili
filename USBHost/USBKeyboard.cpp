// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "USBKeyboard.h"
#include "hid_handler.h"
#include "kipili_cdefs.h"
#include "utilities/BucketList.h"


namespace kio::USB
{

static KeyboardReportHandler* kbd_report_cb = nullptr;
static KeyEventHandler*		  key_event_cb	= nullptr;
static CharEventHandler*	  char_event_cb = nullptr;

static KeyboardReport old_report; // most recent report

static const Char* key_table[2][2] = // [w/shift][w/alt]
	{{
		 key_table_us, // solo
		 key_table_us  // alt
	 },
	 {
		 key_table_us_shift, // shift
		 key_table_us_shift	 // shift+alt
	 }};

struct KeyEventInfo
{
	bool	  down;
	Modifiers modifiers;
	HIDKey	  key;
};

static BucketList<KeyEventInfo, 8, uint8> key_event_queue;
static_assert(sizeof(key_event_queue) == 2 + 3 * 8);


// ================================================================

static int calc_char(bool down, Modifiers modifiers, HIDKey hid_key)
{
	// calculate character code from key event
	// key not calculated for key-up event. may differ from key-down because of changed modifiers anyway.
	// keys are looked up with their `shift` and `alt` state in the translation tables.
	// non-printing keys with no entry in the translation table are returned as `HID_KEY_OTHER + HidKey + modifiers<<16`.
	// `control` modifier masks the character code with 0x1F.
	// `gui` modifier is ignored.
	//
	// if application wishes to handle gui key and non-printing keys with modifiers
	//   then it should use KeyEvent or KeyboardReport functions.
	//   else it must keep track of the modifier keys and apply them itself.

	if (unlikely(!down)) // 'no key' for key-up event.
		return -1;

	//if (unlikely(modifiers & GUI))  	// ignore `gui` modifier
	//	return -1;

	int c = hid_key < KEY_TRANSLATION_TABLE_SIZE ?
				key_table[modifiers & SHIFT ? 1 : 0][modifiers & ALT ? 1 : 0][hid_key] :
				0;

	if (unlikely(c == 0)) // non-priting key
		return HID_KEY_OTHER + hid_key + (modifiers << 16);

	if (unlikely(modifiers & CTRL)) // apply `control` modifier
		c = c & 0x1f;

	return c;
}

static bool find(HIDKey key, const HIDKey keys[6]) { return key != 0 && memchr(keys, key, 6) != nullptr; }

KeyEvent::KeyEvent(bool d, Modifiers m, HIDKey key) noexcept :
	down(d),
	modifiers(m),
	key(key),
	ucs2char(UCS2Char(calc_char(d, m, key)))
{}


// ================================================================

void setKeyTranslationTables(const KeyTable solo, const KeyTable shifted, const KeyTable alt, const KeyTable altshift)
{
	assert(solo);
	if (shifted == nullptr) shifted = solo;

	key_table[0][0] = solo;
	key_table[1][0] = shifted;
	key_table[0][1] = alt ? alt : solo;
	key_table[1][1] = altshift ? altshift : shifted;
}

const KeyboardReport& getKeyboardReport()
{
	// get KeyboardReport
	// returns the latest KeyboardReport which reflects the current state of keys

	return old_report;
}

KeyEvent getKeyEvent()
{
	// get next KeyEvent
	// if no event handler is installed
	//
	// returns KeyEvent with event.key = NO_KEY if no event available

	if (key_event_queue.ls_avail())
	{
		const KeyEventInfo& i	= key_event_queue.ls_get();
		bool				d	= i.down;
		Modifiers			m	= i.modifiers;
		HIDKey				key = i.key;
		key_event_queue.ls_push();
		return KeyEvent {d, m, key};
	}

	return KeyEvent {};
}

int getChar()
{
	// get next char
	// return -1 if no char available
	// function keys with no entry in the translation tables are returned as HID_KEY_OTHER + HidKey + modifiers<<16

	while (key_event_queue.ls_avail())
	{
		const KeyEventInfo& i	= key_event_queue.ls_get();
		bool				d	= i.down;
		Modifiers			m	= i.modifiers;
		HIDKey				key = i.key;
		key_event_queue.ls_push();
		int c = calc_char(d, m, key);
		if (c != -1) return c;
	}

	return -1;
}

static void reset_handlers()
{
	kbd_report_cb = nullptr;
	key_event_cb  = nullptr;
	char_event_cb = nullptr;
	while (key_event_queue.ls_avail()) key_event_queue.ls_push(); // drain queue
}

void setKeyboardReportHandler(KeyboardReportHandler& handler)
{
	reset_handlers();
	kbd_report_cb = handler;
}

void setKeyEventHandler(KeyEventHandler& handler)
{
	reset_handlers();
	key_event_cb = handler;
}

void setCharEventHandler(CharEventHandler& handler)
{
	reset_handlers();
	char_event_cb = handler;
}

static void handle_key_event(
	const KeyboardReport& new_report, const KeyboardReport& old_report, bool down,
	void (*handler)(bool, Modifiers, HIDKey))
{
	for (uint i = 0; i < 6; i++)
	{
		HIDKey key = new_report.keys[i];
		if (key && !find(key, old_report.keys))
		{
			Modifiers mod = new_report.modifiers;
			handler(down, mod, key);
		}
	}
}

static void handle_key_event(const KeyboardReport& new_report, void (*handler)(bool, Modifiers, HIDKey))
{
	handle_key_event(old_report, new_report, false, handler); // find & handle key up events
	handle_key_event(new_report, old_report, true, handler);  // find & handle key down events
}

} // namespace kio::USB


void handle_hid_keyboard_event(const hid_keyboard_report_t* report) noexcept
{
	// this handler is called by `tuh_hid_report_received_cb()`
	// which receives the USB Host events

	using namespace kio::USB;

	assert(report);
	static_assert(sizeof(KeyboardReport) == sizeof(hid_keyboard_report_t));
	const KeyboardReport& new_report = *reinterpret_cast<const KeyboardReport*>(report);

	if (kbd_report_cb) { kbd_report_cb(new_report); }

	else if (key_event_cb)
	{
		handle_key_event(new_report, [](bool down, Modifiers modifiers, HIDKey key) {
			key_event_cb(KeyEvent(down, modifiers, key));
		});
	}

	else if (char_event_cb)
	{
		handle_key_event(new_report, old_report, true, [](bool down, Modifiers modifiers, HIDKey key) {
			assert(down);
			int c = calc_char(down, modifiers, key);
			if (c != -1) char_event_cb(c);
		});
	}

	else // no callback handler installed
	{
		handle_key_event(new_report, [](bool down, Modifiers modifiers, HIDKey key) {
			if (unlikely(key_event_queue.hs_avail() == 0)) key_event_queue.ls_push(); // discard oldest

			// store key into key_event_queue for later use by getKeyEvent() or getChar():

			KeyEventInfo& i = key_event_queue.hs_get();
			i.down			= down;
			i.modifiers		= modifiers;
			i.key			= key;
			key_event_queue.hs_push();
		});
	}

	old_report = new_report;
}
