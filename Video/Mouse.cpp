// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Mouse.h"
#include "BucketList.h"
#include "VideoController.h"
#include "Sprites.h"
#include "USBHost/hid_handler.h"
#include "vga_types.h"
#include <memory>

// all hot video code should go into ram to allow video while flashing.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.mouse" XWRAP(__LINE__))))	  // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.mouse" XWRAP(__LINE__)))) // general ram


namespace kio::Video
{

using namespace Graphics;


// =============================================================
//		MousePointer
// =============================================================

#define _ transparent,
#define b Color(0, 0, 0),
#define F Color::fromRGB8(0xEE, 0xEE, 0xFF),

// clang-format off

__unused constexpr Color bitmap_pointerM[] =
{
	b _ _ _ _ _ _ _ _ _ _
	b b _ _ _ _ _ _ _ _ _
	b F b _ _ _ _ _ _ _ _
	b F F b _ _ _ _ _ _ _
	b F F F b _ _ _ _ _ _
	b F F F F b _ _ _ _ _
	b F F F F F b _ _ _ _
	b F F F F F F b _ _ _
	b F F F F F F F b _ _
	b F F F F F F F F b _
	b F F F F F b b b b b
	b F F b F F b _ _ _ _
	b F b _ b F F b _ _ _
	b b _ _ b F F b _ _ _
	b _ _ _ _ b F F b _ _
	_ _ _ _ _ b F F b _ _
	_ _ _ _ _ _ b b _ _ _
};
__unused constexpr Color bitmap_pointerL[] =
{
	b b _ _ _ _ _ _ _ _ _ _
	b F b _ _ _ _ _ _ _ _ _
	b F F b _ _ _ _ _ _ _ _
	b F F F b _ _ _ _ _ _ _
	b F F F F b _ _ _ _ _ _
	b F F F F F b _ _ _ _ _
	b F F F F F F b _ _ _ _
	b F F F F F F F b _ _ _
	b F F F F F F F F b _ _
	b F F F F F F F F F b _
	b F F F F F F F F F F b
	b F F F F F F b b b b b
	b F F F b F F b _ _ _ _
	b F F b _ b F F b _ _ _
	b F b _ _ b F F b _ _ _
	b b _ _ _ _ b F F b _ _
	_ _ _ _ _ _ b F F b _ _
	_ _ _ _ _ _ _ b b _ _ _
};
constexpr Color bitmap_crosshair[] =
{
	_ _ _ _ b F b _ _ _ _
	_ _ _ _ b F b _ _ _ _
	_ _ _ _ b F b _ _ _ _
	_ _ _ _ b F b _ _ _ _
	b b b b b _ b b b b b
	F F F F _ F _ F F F F
	b b b b b _ b b b b b
	_ _ _ _ b F b _ _ _ _
	_ _ _ _ b F b _ _ _ _
	_ _ _ _ b F b _ _ _ _
	_ _ _ _ b F b _ _ _ _
};
constexpr Color bitmap_ibeam[] =
{
	b b b _ b b b
	b F F b F F b
	b b b F b b b
	_ _ b F b _ _
	_ _ b F b _ _
	_ _ b F b _ _
	_ _ b F b _ _
	_ _ b F b _ _
	_ _ b F b _ _
	b b b F b b b
	b F F b F F b
	b b b _ b b b
};
constexpr Color bitmap_busy1[] =
{
	_ _ _ _ F F F _ _ _ _
	_ _ F F b b F F F _ _
	_ F b b b b F F F F _
	_ F b b b b F F F F _
	F b b b b b F F F F F
	F b b b b b b b b b F
	F F F F F b b b b b F
	_ F F F F b b b b F _
	_ F F F F b b b b F _
	_ _ F F F b b F F _ _
	_ _ _ _ F F F _ _ _ _
};
constexpr Color bitmap_busy2[] =
{
	_ _ _ _ F F F _ _ _ _
	_ _ F F b b b F F _ _
	_ F b b b b b b b F _
	_ F F b b b b b F F _
	F F F F b b b F F F F
	F F F F F b F F F F F
	F F F F b b b F F F F
	_ F F b b b b b F F _
	_ F b b b b b b b F _
	_ _ F F b b b F F _ _
	_ _ _ _ F F F _ _ _ _
};
constexpr Color bitmap_busy3[] =
{
	_ _ _ _ F F F _ _ _ _
	_ _ F F F b b F F _ _
	_ F F F F b b b b F _
	_ F F F F b b b b F _
	F F F F F b b b b b F
	F b b b b b b b b b F
	F b b b b b F F F F F
	_ F b b b b F F F F _
	_ F b b b b F F F F _
	_ _ F F b b F F F _ _
	_ _ _ _ F F F _ _ _ _
};
constexpr Color bitmap_busy4[] =
{
	_ _ _ _ F F F _ _ _ _
	_ _ F F F F F F F _ _
	_ F b F F F F F b F _
	_ F b b F F F b b F _
	F b b b b F b b b b F
	F b b b b b b b b b F
	F b b b b F b b b b F
	_ F b b F F F b b F _
	_ F b F F F F F b F _
	_ _ F F F F F F F _ _
	_ _ _ _ F F F _ _ _ _
};

// clang-format on

#undef _
#undef b
#undef F

constexpr struct
{
	uint8		 width, height, tip_x, tip_y;
	const Color* bitmap;
} images[] = {
	{11, 17, 1, 2, bitmap_pointerM},
	//{12,18, 1, 1, bitmap_pointerL},
	{11, 11, 5, 5, bitmap_crosshair},
	{7, 12, 3, 9, bitmap_ibeam},
	{11, 11, 5, 5, bitmap_busy1},
	{11, 11, 5, 5, bitmap_busy2},
	{11, 11, 5, 5, bitmap_busy3},
	{11, 11, 5, 5, bitmap_busy4},
};

static Shape::Row* shape_rows = nullptr;

static MouseEvent mouse_event;
static bool		  mouse_event_available = false;

static MouseReportHandler* mouse_report_cb			 = nullptr;
static MouseEventHandler*  mouse_event_cb			 = nullptr;
static bool				   mouse_move_events_enabled = false;


// ====================================================================
//		ctor, private methods and helpers
// ====================================================================

Mouse& Mouse::getRef() noexcept
{
	static Mouse mouse_pointer(
		[]() -> const Shape {
			constexpr auto& img = images[MOUSE_POINTER];
			constexpr uint8 w	= img.width;
			constexpr uint8 h	= img.height;

			assert(shape_rows == nullptr);
			shape_rows = Shape::Row::newShape(w, h, img.bitmap); // throws but hopefully not so early `:-)
			return Shape {w, h, shape_rows, img.tip_x, img.tip_y};
		}(),
		VideoController::width() / 2, VideoController::height() / 2);

	return mouse_pointer;
}

static void limit(coord& x, coord& y)
{
	Size size = VideoController::size;

	static_assert(int(uint(int16(-123))) == -123);

	if (uint(x) >= uint(size.width)) { x = x < 0 ? 0 : size.width - 1; }
	if (uint(y) >= uint(size.height)) { y = y < 0 ? 0 : size.height - 1; }
}

static void limit(Point& p) { limit(p.x, p.y); }


// ====================================================================
//		static Mouse methods
// ====================================================================

void Mouse::getPosition(coord& x, coord& y) noexcept // static
{
	x = mouse_event.pos.x;
	y = mouse_event.pos.y;
}

Point Mouse::getPosition() noexcept // static
{
	return mouse_event.pos;
}

int8 Mouse::getWheelCount() noexcept // static
{
	return mouse_event.wheel;
}

int8 Mouse::getPanCount() noexcept // static
{
	return mouse_event.pan;
}

MouseButtons Mouse::getButtons() noexcept // static
{
	return mouse_event.buttons;
}

void Mouse::setPosition(const Point& p) noexcept //static
{
	// set mouse position
	// getPosition() is immediately updated
	// the position of the mouse pointer on the screen is updated at the next vblank
	// mouseEventAvailable() is set and getMouseEvent() with new position is available
	// MouseEventHandler() is called if registered and MOVE_EVENTS enabled
	// MouseReportHandler() IS NOT CALLED

	assert(get_core_num() == 0);

	mouse_event.pos = p;
	limit(mouse_event.pos);

	if (mouse_report_cb)
	{
		mouse_event.toggled	  = NO_BUTTON;
		mouse_event_available = false;
		//mouse_report_cb(new_report);
	}
	else if (mouse_event_cb)
	{
		mouse_event.toggled	  = NO_BUTTON;
		mouse_event_available = false;
		if (mouse_move_events_enabled) mouse_event_cb(mouse_event);
	}
	else
	{
		if (!mouse_event_available)
		{
			mouse_event.toggled	  = NO_BUTTON;
			mouse_event_available = mouse_move_events_enabled;
		}
	}
}

void Mouse::setPosition(coord x, coord y) noexcept //static
{
	setPosition(Point(x, y));
}

void Mouse::setShape(MouseShapeID id, bool wait)
{
	if (id >= count_of(images)) id = MOUSE_POINTER;

	const auto& img = images[id];
	const uint8 w	= img.width;
	const uint8 h	= img.height;

	std::unique_ptr<Shape::Row[]> old {shape_rows};
	shape_rows = nullptr;
	shape_rows = Shape::Row::newShape(w, h, img.bitmap); // throws

	Sprite::replace(Shape {w, h, shape_rows, img.tip_x, img.tip_y}, wait);
}

void Mouse::vblank() noexcept
{
	assert(get_core_num() == 1);

	Sprite::move(mouse_event.pos);

	// TODO: animate busy pointer
}


// ====================================================================
//		callbacks
// ====================================================================

MouseEvent& MouseEvent::operator+=(const MouseReport& report) noexcept
{
	toggled = buttons ^ report.buttons;
	buttons = report.buttons;
	pos.x += report.dx;
	pos.y += report.dy;
	limit(pos);
	wheel += report.wheel;
	pan += report.pan;
	return *this;
}

bool mouseEventAvailable() noexcept { return mouse_event_available; }

const MouseEvent& getMouseEvent() noexcept
{
	mouse_event_available = false;
	return mouse_event;
}

void enableMouseMoveEvents(bool onoff) noexcept { mouse_move_events_enabled = onoff; }

void setMouseReportHandler(MouseReportHandler& handler) noexcept
{
	mouse_report_cb = handler;
	if (handler) mouse_event_cb = nullptr;
}

void setMouseEventHandler(MouseEventHandler& handler) noexcept
{
	mouse_event_cb = handler;
	if (handler) mouse_report_cb = nullptr;
}

void setMouseEventHandler(MouseEventHandler& handler, bool enable_mouse_move_events) noexcept
{
	setMouseEventHandler(handler);
	enableMouseMoveEvents(enable_mouse_move_events);
}

} // namespace kio::Video


// ====================================================================

void handle_hid_mouse_event(const hid_mouse_report_t* report) noexcept
{
	// callback for USB Host events from `tuh_hid_report_received_cb()`:

	using namespace kio::Video;
	assert(get_core_num() == 0);

	static_assert(sizeof(hid_mouse_report_t) == sizeof(MouseReport)); // minimal checking
	const MouseReport& new_report = *reinterpret_cast<const MouseReport*>(report);

	if (mouse_event_available) // old event not yet polled?
	{
		mouse_event.buttons = mouse_event.buttons ^ mouse_event.toggled;
		mouse_event.toggled = NO_BUTTON;
	}

	mouse_event += new_report;

	if (mouse_report_cb)
	{
		mouse_event_available = false;
		mouse_report_cb(new_report);
	}
	else if (mouse_event_cb)
	{
		mouse_event_available = false;
		if (mouse_event.toggled || mouse_move_events_enabled) mouse_event_cb(mouse_event);
	}
	else
	{
		if (mouse_event.toggled || mouse_move_events_enabled) mouse_event_available = true;
	}
}


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
