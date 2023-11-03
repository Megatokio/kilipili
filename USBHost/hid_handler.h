// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "HidKeys.h"
#include <class/hid/hid.h>

namespace kio::USB
{

// callback for USB Host events from `tuh_hid_report_received_cb()`:
extern void handle_hid_mouse_event(const hid_mouse_report_t*) noexcept;

// callback for USB Host events from `tuh_hid_report_received_cb()`:
using HidKeyboardEventHandler = void(const HidKeyboardReport&) noexcept;
extern HidKeyboardEventHandler defaultHidKeyboardEventHandler;
extern void					   setHidKeyboardEventHandler(HidKeyboardEventHandler*) noexcept;

} // namespace kio::USB
