// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap_wAttr.h"
#include "SingleSprite.h"
#include "USBMouse.h"

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
	int8		 wheel	 = 0;		  // accumulated wheel position (up/down)
	int8		 pan	 = 0;		  // accumulated pan position (left/right)
	Point		 position {160, 120}; // accumulated position

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

#if 1

template<Animation ANIM, Softening SOFT>
class MousePointer : public SingleSprite<ANIM, SOFT>
{
	NO_COPY_MOVE(MousePointer);

public:
	using super		= SingleSprite<ANIM, SOFT>;
	using ColorMode = Graphics::ColorMode;
	template<ColorMode CM>
	using Pixmap			= Graphics::Pixmap<CM>;
	using Shape				= typename super::Shape;
	using MouseEventHandler = USB::MouseEventHandler;

	Shape* sprite;

	MousePointer(MouseShapeID, coord x, coord y) noexcept;
	~MousePointer() noexcept;

	void moveTo(coord x, coord y) noexcept;
	void moveTo(const Point&) noexcept;

	template<typename Sprite>
	void setSprite(Sprite*);

	template<ColorMode CM>
	void setSprite(const Pixmap<CM>&, uint transparent_color, const Point& hotspot);

	bool		 isVisible() noexcept;
	void		 getPosition(coord& x, coord& y) noexcept;
	Point		 getPosition() noexcept;
	int8		 getWheelCount() noexcept;
	int8		 getPanCount() noexcept;
	MouseButtons getButtons() noexcept;

	bool			  mouseEventAvailable() noexcept;
	const MouseEvent& getMouseEvent() noexcept;

	void setMouseEventHandler(MouseEventHandler&) noexcept;
	void enableMouseMoveEvents(bool = true) noexcept;
	void setMouseEventHandler(MouseEventHandler&, bool enable_mouse_move_events) noexcept;

	static MouseEvent		  mouse_event;
	static bool				  mouse_event_available;
	static MouseEventHandler* mouse_event_cb;
	static bool				  mouse_move_events_enabled;
};


#endif


} // namespace kio::Video

/*


































*/
