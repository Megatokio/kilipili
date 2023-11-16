// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "USBMouse.h"
#include "USBHost/hid_handler.h"
#include "basic_math.h"
#include "common/Queue.h"
#include <memory>

// all hot video code should go into ram to allow video while flashing.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.mouse" XWRAP(__LINE__))))	  // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.mouse" XWRAP(__LINE__)))) // general ram


// ====================================================================

namespace kio::Video
{
using namespace USB;

static Queue<MouseEvent, 4, uint16> mouse_event_queue;
static_assert(sizeof(mouse_event_queue) == 4 + 8 * 4);

static bool enable_button_up_events				= true;
static bool enable_move_with_button_down_events = true;
static bool enable_move_events					= false;

static MouseButtons old_buttons = NO_BUTTON;
static int16		old_x = 0, old_y = 0;
static int16		screen_width  = 320;
static int16		screen_height = 240;


// =============================================================

void setScreenSize(int width, int height) noexcept
{
	screen_width  = int16(width);
	screen_height = int16(height);
}

void getMousePosition(int& x, int& y) noexcept
{
	x = old_x;
	y = old_y;
}

MouseEvent::MouseEvent() noexcept : // MouseEvent with no changes to the current state
	buttons(old_buttons),
	toggled(NO_BUTTON),
	wheel(0),
	pan(0),
	x(old_x),
	y(old_y)
{}

MouseEvent::MouseEvent(const HidMouseReport& report) noexcept :
	buttons(MouseButtons(report.buttons)),
	toggled(MouseButtons(report.buttons ^ old_buttons)),
	wheel(report.wheel),
	pan(report.pan),
	x(int16(minmax(0, old_x + report.dx, screen_width - 1))),
	y(int16(minmax(0, old_y + report.dy, screen_height - 1)))
{
	old_buttons = buttons;
	old_x		= x;
	old_y		= y;
}

// =============================================================

bool mouseEventAvailable() noexcept
{
	return mouse_event_queue.avail(); //
}

MouseEvent getMouseEvent() noexcept
{
	if (mouse_event_queue.avail()) return mouse_event_queue.get();
	else return MouseEvent(); // event with the current state
}

static void pushMouseEvent(const MouseEvent& event) noexcept
{
	if unlikely (mouse_event_queue.free() == 0) mouse_event_queue.get(); // discard oldest
	mouse_event_queue.put(event);
}

static MouseEventHandler* mouse_event_handler = &pushMouseEvent;

void setMouseEventHandler(MouseEventHandler* handler) noexcept
{
	mouse_event_handler					= handler ? handler : &pushMouseEvent;
	enable_button_up_events				= true;
	enable_move_with_button_down_events = true;
	enable_move_events					= false;
	old_buttons							= NO_BUTTON;
	setHidMouseEventHandler(&defaultHidMouseEventHandler);
}

void enableMouseEvents(bool btn_up, bool move_w_btn_dn, bool move) noexcept
{
	enable_button_up_events				= btn_up;
	enable_move_with_button_down_events = move_w_btn_dn;
	enable_move_events					= move;
}

} // namespace kio::Video

namespace kio::USB
{
void __weak_symbol defaultHidMouseEventHandler(const HidMouseReport& report) noexcept
{
	// this handler is called by `tuh_hid_report_received_cb()`
	// which receives the USB Host events

	using namespace Video;

	auto event = MouseEvent(report); // always calculate the event wg. side effects in ctor!

	bool f = enable_move_events || (report.buttons & uint8(~old_buttons)) ||
			 (enable_move_with_button_down_events && report.buttons != NO_BUTTON && (report.dx || report.dy)) ||
			 (enable_button_up_events && (report.buttons != old_buttons)) || report.pan || report.wheel;

	if (f) mouse_event_handler(event);
}

} // namespace kio::USB
