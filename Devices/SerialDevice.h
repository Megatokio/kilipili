// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "devices_types.h"


namespace kio::Devices
{

constexpr uint gets_max_len = 512; // max string length handled by gets()


/* Interface class `SerialDevice`
*/
class SerialDevice
{
public:
	char  last_char = 0; // used by gets()
	Flags flags;
	char  _padding[2];

public:
	SerialDevice(Flags flags) noexcept : flags(flags) {}
	virtual ~SerialDevice() noexcept			 = default;
	SerialDevice(const SerialDevice&) noexcept	 = default;
	SerialDevice(SerialDevice&&) noexcept		 = default;
	SerialDevice& operator=(const SerialDevice&) = delete;
	SerialDevice& operator=(SerialDevice&&)		 = delete;

	// sequential read/write
	//   partial=true:  transfer as much as possible without blocking. possibly none.
	//   partial=false: transfer all data or throw. possibly blocking.
	//
	// text i/o:
	//   blocking. default implementations use read() and write().

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr);
	virtual SIZE   read(char* data, SIZE, bool partial = false);
	virtual SIZE   write(const char* data, SIZE, bool partial = false);

	virtual int	 getc(uint timeout_us);
	virtual char getc();
	virtual str	 gets();
	virtual SIZE putc(char);
	virtual SIZE puts(cstr);
	virtual SIZE printf(cstr fmt, ...) __printflike(2, 3);

	void flushOut() { ioctl(IoCtl::FLUSH_OUT); }
};


SIZE SerialDevice::read(char*, SIZE, bool) { throw NOT_READABLE; }
SIZE SerialDevice::write(const char*, SIZE, bool) { throw NOT_WRITABLE; }

} // namespace kio::Devices
