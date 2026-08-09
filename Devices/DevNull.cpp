// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "DevNull.h"

namespace kilipili::Devices
{

DevNull dev_null;

DevNull::DevNull() noexcept : SerialDevice(writable) {}


} // namespace kilipili::Devices
