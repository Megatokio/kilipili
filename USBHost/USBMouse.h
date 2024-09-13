// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "geometry.h"
#include "hid_handler.h"

namespace kio::USB
{

enum MouseButtons : uint8 {
	NO_BUTTON		= 0,
	LEFT_BUTTON		= 1 << 0,
	RIGHT_BUTTON	= 1 << 1,
	MIDDLE_BUTTON	= 1 << 2,
	BACKWARD_BUTTON = 1 << 3,
	FORWARD_BUTTON	= 1 << 4,
};


struct __aligned(4) MouseEvent
{
	MouseButtons buttons = NO_BUTTON; // currently pressed buttons
	MouseButtons toggled = NO_BUTTON; // buttons which toggled
	int8		 wheel	 = 0;		  // wheel movement
	int8		 pan	 = 0;		  // using AC Pan
	int16		 x, y;				  // position on screen

	Point position() const noexcept { return Point(x, y); }

	explicit MouseEvent(const struct USB::HidMouseReport&) noexcept;
	MouseEvent() noexcept;
};


/*
	set an event handler to be called for all mouse events.
	getMouseEvent() and mouseEventAvailable() become dead.
	the events are filtered as set with enableMouseEvents().
	pass a nullptr to switch back to polling mode. (default)
*/
using MouseEventHandler = void(const MouseEvent&);
extern void setMouseEventHandler(MouseEventHandler*) noexcept;

/*
	set the limit for mouse x and y position.
	must be called after setting VGA screen size.
	note: is called by class MousePointer.
*/
extern void setScreenSize(int width, int height) noexcept;

/*
	query the current absolute position of the mouse.
*/
extern Point getMousePosition() noexcept;

/*
	set filter for mouse events:
	- btn_up: report button up events. (default=true)
	- move_w_btn_dn: report move events while any button is down. (default=true)
	- move: report all move events. (default=false)
*/
extern void enableMouseEvents(bool btn_up, bool move_w_btn_dn, bool move) noexcept;

/*
	test whether a new mouse event is available.
	always false if you have set a MouseEventHandler.
*/
extern bool mouseEventAvailable() noexcept;

/*
	get next mouse event.
	returns an event with the current mouse buttons and position if no event available
	or if you have set a MouseEventHandler.
	the events are filtered as set with enableMouseEvents().
*/
extern MouseEvent getMouseEvent() noexcept;


} // namespace kio::USB

/*


































*/
