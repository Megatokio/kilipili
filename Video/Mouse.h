// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Sprites.h"


namespace kio::Video
{

using coord = Graphics::coord;
using Point = Graphics::Point;

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
inline MouseButtons operator|(MouseButtons a, uint8 b) { return MouseButtons(uint8(a) | b); }


// USB mouse report in "boot" mode
// matches hid_mouse_report_t
struct MouseReport
{
	MouseButtons buttons; // currently pressed buttons
	int8		 dx;	  // dx movement
	int8		 dy;	  // dy movement
	int8		 wheel;	  // wheel movement
	int8		 pan;	  // using AC Pan
};


// MouseEvent with absolute positions
struct MouseEvent
{
	MouseButtons buttons = NO_BUTTON; // currently pressed buttons
	MouseButtons toggled = NO_BUTTON; // buttons which toggled
	int8		 wheel	 = 0;		  // accumulated wheel position
	int8		 pan	 = 0;		  // accumulated pan position
	Point		 pos {160, 120};	  // accumulated position

	MouseEvent& operator+=(const MouseReport&) noexcept;
};


enum MouseShapeID {
	MOUSE_POINTER,
	MOUSE_CROSSHAIR,
	MOUSE_IBEAM_CURSOR,
	MOUSE_BUSY1,
	MOUSE_BUSY2,
	MOUSE_BUSY3,
	MOUSE_BUSY4,
	MOUSE_FINGER,
	MOUSE_SLIDER_H,
	MOUSE_SLIDER_V,
	MOUSE_SLIDER_X,
	MOUSE_MAGNIFY,
};

#if 0 

class Mouse : public Sprite
{
	NO_COPY_MOVE(Mouse);

	Mouse(const Shape& s, coord x, coord y) noexcept : Sprite(s, x, y, zMouse) {}

public:
	virtual ~Mouse() override = default;
	static Mouse& getRef() noexcept;

	//void show() noexcept;
	//void hide(bool wait) noexcept;
	//bool is_visible() noexcept;
	static void			setPosition(coord x, coord y) noexcept;
	static void			setPosition(const Point&) noexcept;
	static void			getPosition(coord& x, coord& y) noexcept;
	static Point		getPosition() noexcept;
	static int8			getWheelCount() noexcept;
	static int8			getPanCount() noexcept;
	static MouseButtons getButtons() noexcept;

	void setShape(const Shape& s, bool wait = 0) noexcept { Sprite::replace(s, wait); }
	void setShape(MouseShapeID = MOUSE_POINTER, bool wait = 0);

	virtual void vblank() noexcept override;
};


extern bool				 mouseEventAvailable() noexcept;
extern const MouseEvent& getMouseEvent() noexcept;

using MouseReportHandler = void(const MouseReport&) noexcept;
using MouseEventHandler	 = void(const MouseEvent&) noexcept;

extern void setMouseReportHandler(MouseReportHandler&) noexcept;
extern void setMouseEventHandler(MouseEventHandler&) noexcept;
extern void enableMouseMoveEvents(bool = true) noexcept;
extern void setMouseEventHandler(MouseEventHandler&, bool enable_mouse_move_events) noexcept;


#endif


} // namespace kio::Video
