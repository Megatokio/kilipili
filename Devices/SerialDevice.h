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
class SerialDevice : public RCObject
{
public:
	char   last_char = 0; // used by gets()
	Flags  flags;
	uint16 _padding;

public:
	SerialDevice(Flags flags) noexcept : flags(flags) {}
	virtual ~SerialDevice() noexcept override	 = default;
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
	virtual SIZE   read(void* data, SIZE, bool partial = false);
	virtual SIZE   write(const void* data, SIZE, bool partial = false);

	virtual int	 getc(uint timeout_us);
	virtual char getc();
	virtual str	 gets();
	virtual SIZE putc(char);
	virtual SIZE puts(cstr);
	virtual SIZE printf(cstr fmt, ...) __printflike(2, 3);

	void flushOut() { ioctl(IoCtl::FLUSH_OUT); }
};


inline SIZE SerialDevice::read(void*, SIZE, bool) { throw NOT_READABLE; }
inline SIZE SerialDevice::write(const void*, SIZE, bool) { throw NOT_WRITABLE; }

} // namespace kio::Devices
