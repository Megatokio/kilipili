// Copyright (c) 2023 - 2023 kio@little-bat.de
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
	virtual SIZE putc(char) override { return 1; }
	virtual SIZE puts(cstr s) override;
	virtual SIZE printf(cstr, ...) override __printflike(2, 3);
};

extern DevNull dev_null;

} // namespace kio::Devices
