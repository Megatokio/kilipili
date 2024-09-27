// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#ifdef UNIT_TEST

  #include "hid_handler.h"
  #include <common/Queue.h>

namespace kio::USB
{
struct HidReport
{
	bool isa_keyboard_report;
	union
	{
		HidKeyboardReport keyboard_report;
		HidMouseReport	  mouse_report;
	};

	HidReport() : keyboard_report() {}
	HidReport(const HidKeyboardReport& r) : isa_keyboard_report(true), keyboard_report(r) {}
	HidReport(const HidMouseReport& r) : isa_keyboard_report(false), mouse_report(r) {}
};

static bool						mouse_present = false;
static Queue<HidReport, 32>		hid_reports;
static HidMouseEventHandler*	mouse_event_handler	   = defaultHidMouseEventHandler;
static HidKeyboardEventHandler* keyboard_event_handler = defaultHidKeyboardEventHandler;


void initUSBHost() noexcept
{
	hid_reports.flush();
	mouse_present		   = false;
	mouse_event_handler	   = defaultHidMouseEventHandler;
	keyboard_event_handler = defaultHidKeyboardEventHandler;
}

void pollUSB() noexcept
{
	while (hid_reports.avail())
	{
		HidReport r = hid_reports.get();
		if (r.isa_keyboard_report) keyboard_event_handler(r.keyboard_report);
		else mouse_event_handler(r.mouse_report);
	}
}

bool keyboardPresent() noexcept { return true; }
bool mousePresent() noexcept { return mouse_present; }

void setHidMouseEventHandler(HidMouseEventHandler* handler) noexcept
{
	mouse_event_handler = handler ? handler : defaultHidMouseEventHandler;
}

void setHidKeyboardEventHandler(HidKeyboardEventHandler* handler) noexcept
{
	keyboard_event_handler = handler ? handler : defaultHidKeyboardEventHandler;
}


namespace Mock
{
void setMousePresent(bool f) { mouse_present = f; }
void addKeyboardReport(const HidKeyboardReport& r) { hid_reports.put(HidReport(r)); }
void addMouseReport(const HidMouseReport& r) { hid_reports.put(HidReport(r)); }
} // namespace Mock

} // namespace kio::USB


#endif
