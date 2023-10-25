// Copyright (c) 2022 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "USBMouse.h"
#include "hid_handler.h"
#include "utilities/BucketList.h"

namespace kio::USB
{

static bool enable_button_up_events				= true;
static bool enable_move_with_button_down_events = true;
static bool enable_move_events					= false;

static BucketList<MouseReport, 8, uint8> mouse_event_queue;
//ON_INIT([](){mouse_event_queue.hs_push();}); // push empty bucket: always one 'current' bucket in the list
static MouseReport no_report; // old buttons and dx,dy,wheel,pan all zero

static void				   queueMouseReport(const MouseReport& e) noexcept;
static MouseReportHandler* mouse_report_cb = &queueMouseReport;
static MouseEventHandler*  mouse_event_cb  = nullptr;


// ====================================================================

MouseEvent::MouseEvent(const MouseReport& r, MouseButtons oldbuttons) noexcept :
	buttons(r.buttons),
	toggled(r.buttons ^ oldbuttons),
	dx(r.dx),
	dy(r.dy),
	wheel(r.wheel),
	pan(r.pan)
{}

bool mouseReportAvailable() noexcept { return mouse_event_queue.ls_avail() > 1; }
bool mouseEventAvailable() noexcept { return mouse_event_queue.ls_avail() > 1; }

const MouseReport& getMouseReport() noexcept
{
	// return next MouseReport if available
	// else return no_report with old buttons and dx,dy,wheel,pan all zero
	// note: there is always the last report in the list!

	// return next report if available:
	if (mouse_event_queue.ls_avail() > 1)
	{
		// push the last report back into queue:
		mouse_event_queue.ls_push();

		// get next report:
		const MouseReport& r = mouse_event_queue.ls_get();
		no_report.buttons	 = r.buttons;
		return r;
	}
	else { return no_report; }
}

MouseEvent getMouseEvent() noexcept
{
	MouseButtons old_buttons = no_report.buttons;
	return MouseEvent(getMouseReport(), old_buttons);
}

static void queueMouseReport(const MouseReport& e) noexcept
{
	if (mouse_event_queue.hs_avail())
	{
		mouse_event_queue.hs_get() = e;
		mouse_event_queue.hs_push();
	}
}


void setMouseReportHandler(MouseReportHandler& handler) noexcept
{
	mouse_report_cb = handler ? handler : queueMouseReport;
	mouse_event_cb	= nullptr;
}

void setMouseEventHandler(MouseEventHandler& handler) noexcept
{
	mouse_event_cb	= handler;
	mouse_report_cb = handler ? nullptr : queueMouseReport;
}

void enableMouseEvents(bool btn_up, bool move_w_btn_dn, bool move) noexcept
{
	enable_button_up_events				= btn_up;
	enable_move_with_button_down_events = move_w_btn_dn;
	enable_move_events					= move;
}

void setMouseEventHandler(MouseEventHandler& handler, bool up, bool move_w_btn_dn, bool move) noexcept
{
	setMouseEventHandler(handler);
	enableMouseEvents(up, move_w_btn_dn, move);
}


void handle_hid_mouse_event(const hid_mouse_report_t* report) noexcept
{
	// callback for USB Host events from `tuh_hid_report_received_cb()`:

	static_assert(sizeof(hid_mouse_report_t) == sizeof(MouseReport)); // minimal checking
	const MouseReport& new_report = *reinterpret_cast<const MouseReport*>(report);

	if (mouse_report_cb) { mouse_report_cb(new_report); }
	else { mouse_event_cb(MouseEvent(new_report, NO_BUTTON)); }
}

} // namespace kio::USB


#if 0

  #include <bsp/ansi_escape.h>

// If your host terminal support ansi escape code such as TeraTerm
// it can be use to simulate mouse cursor movement within terminal
  #define USE_ANSI_ESCAPE 1

void cursor_movement(int8 x, int8 y, int8 wheel)
{
	if (USE_ANSI_ESCAPE)
	{
		if (x)	// Move X using ansi escape
		{
			if (x < 0) printf(ANSI_CURSOR_BACKWARD(%i), -x); // move left
			else	   printf(ANSI_CURSOR_FORWARD(%i),  +x); // move right
		}

		if (y)	// Move Y using ansi escape
		{
			if (y < 0) printf(ANSI_CURSOR_UP(%i),   -y); // move up
			else	   printf(ANSI_CURSOR_DOWN(%i), +y); // move down
		}

		if (wheel)	// Scroll using ansi escape
		{
			if (wheel < 0) printf(ANSI_SCROLL_UP(%i),   -wheel); // scroll up
			else		   printf(ANSI_SCROLL_DOWN(%i), +wheel); // scroll down
		}

		printf("\r\n");
	}
	else
	{
		printf("(x:%d y:%d w:%d)\r\n", x, y, wheel);
	}
}

void handle_hid_mouse_event(const hid_mouse_report_t* report)
{
	// callback for USB Host events from `tuh_hid_report_received_cb()`:

	static hid_mouse_report_t prev_report;

	uint8 button_changed_mask = report->buttons ^ prev_report.buttons;
	if (button_changed_mask & report->buttons)
	{
		printf(" %c%c%c ",
		report->buttons & MOUSE_BUTTON_LEFT   ? 'L' : '-',
		report->buttons & MOUSE_BUTTON_MIDDLE ? 'M' : '-',
		report->buttons & MOUSE_BUTTON_RIGHT  ? 'R' : '-');
	}

	cursor_movement(report->x, report->y, report->wheel);
}

#endif
