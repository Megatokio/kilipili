// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "SerialDevice.h"


namespace kio::Devices
{

class DevNull final : public SerialDevice
{
public:
	DevNull() noexcept;

	virtual SIZE write(const void*, SIZE size, bool) override { return size; }
	virtual void putc(char) override {}
	virtual void puts(cstr) override {}
	virtual void printf(cstr, ...) override __printflike(2, 3) {}
};

extern DevNull dev_null;

} // namespace kio::Devices
