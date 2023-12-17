// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "File.h"

namespace kio::Devices
{

class Stdio final : public SerialDevice
{
public:
	virtual uint32 ioctl(IoCtl ctl, void* arg1 = nullptr, void* arg2 = nullptr) override;
	virtual SIZE   read(void* data, SIZE, bool partial = false) override;
	virtual SIZE   write(const void* data, SIZE, bool partial = false) override;

	virtual int	 getc(uint timeout_us) override;
	virtual char getc() override;
	//virtual str gets(char* buffer, uint sz) = default
	virtual SIZE putc(char) override;
	virtual SIZE puts(cstr) override;
	virtual SIZE printf(cstr fmt, ...) override __printflike(2, 3);
};

} // namespace kio::Devices
