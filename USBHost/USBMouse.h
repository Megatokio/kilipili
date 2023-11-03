// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <class/hid/hid_host.h>
#include <functional>


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

inline MouseButtons operator^(MouseButtons a, uint8 b) { return MouseButtons(uint8(a) ^ b); }
inline MouseButtons operator&(MouseButtons a, uint8 b) { return MouseButtons(uint8(a) & b); }
inline MouseButtons operator|(MouseButtons a, MouseButtons b) { return MouseButtons(uint8(a) | b); }


// USB mouse report in "boot" mode
// matches TinyUSB::hid_mouse_report_t
struct MouseReport
{
	MouseButtons buttons; // mask with currently pressed buttons
	int8		 dx;	  // x movement
	int8		 dy;	  // y movement
	int8		 wheel;	  // wheel movement
	int8		 pan;	  // using AC Pan
};


// serialized, filtered mouse event
struct MouseEvent
{
	MouseButtons buttons = NO_BUTTON; // currently pressed buttons
	MouseButtons toggled = NO_BUTTON; // buttons which toggled
	int8		 dx		 = 0;		  // x movement
	int8		 dy		 = 0;		  // y movement
	int8		 wheel	 = 0;		  // wheel movement
	int8		 pan	 = 0;		  // using AC Pan

	MouseEvent(const MouseReport&, MouseButtons oldbuttons) noexcept;
};


extern const MouseReport& getMouseReport() noexcept;
extern MouseEvent		  getMouseEvent() noexcept;
extern bool				  mouseReportAvailable() noexcept;
extern bool				  mouseEventAvailable() noexcept;

using MouseReportHandler = void(const MouseReport&);
using MouseEventHandler	 = void(const MouseEvent&);

extern void setMouseReportHandler(MouseReportHandler&) noexcept;
extern void setMouseEventHandler(MouseEventHandler&) noexcept;
extern void enableMouseEvents(bool btn_up, bool move_w_btn_dn, bool move) noexcept;
extern void setMouseEventHandler(MouseEventHandler&, bool btn_up, bool move_w_btn_dn, bool move) noexcept;

} // namespace kio::USB
