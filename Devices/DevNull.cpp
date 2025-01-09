// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "DevNull.h"
#include "stdio.h"
#include "string.h"
#include <stdarg.h>

namespace kio::Devices
{

DevNull dev_null;

DevNull::DevNull() noexcept : SerialDevice(writable) {}

SIZE DevNull::puts(cstr s) { return s ? strlen(s) : 0; }

SIZE DevNull::printf(cstr fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	int n = vsnprintf(nullptr, 0, fmt, va);
	va_end(va);

	return uint(n);
}

} // namespace kio::Devices
