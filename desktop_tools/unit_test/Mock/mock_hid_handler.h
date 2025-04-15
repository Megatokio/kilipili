
// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "USBHost/hid_handler.h"

namespace kio::USB::Mock
{
extern void setMousePresent(bool f = true);
extern void addKeyboardReport(const HidKeyboardReport&);
extern void addMouseReport(const HidMouseReport&);
} // namespace kio::USB::Mock
