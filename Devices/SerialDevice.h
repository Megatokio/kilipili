// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "devices_types.h"
#include "little_big_endian.h"

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
	virtual str	 gets(uint line_ends = (1 << 0) + (1 << 10) + (1 << 13));
	virtual SIZE putc(char);
	virtual SIZE puts(cstr);
	virtual SIZE printf(cstr fmt, ...) __printflike(2, 3);

	void flushOut() { ioctl(IoCtl::FLUSH_OUT); }
	bool is_readable() const noexcept { return flags & readable; }
	bool is_writable() const noexcept { return flags & writable; }

	// ------- Convenience Methods ---------------

	template<typename T>
	T read()
	{
		T n;
		read(&n, sizeof(T));
		return n;
	}

	template<typename T>
	SIZE read(T& n)
	{
		return read(&n, sizeof(T));
	}

	template<typename T>
	SIZE write(const T& n)
	{
		return write(&n, sizeof(T));
	}

	template<typename T, bool LE>
	T reverted(T n)
	{
		if constexpr (LE == little_endian) return n;
		char* p = reinterpret_cast<char*>(&n);
		if constexpr (sizeof(T) >= 2) std::swap(p[0], p[sizeof(T) - 1]);
		if constexpr (sizeof(T) >= 4) std::swap(p[1], p[sizeof(T) - 2]);
		if constexpr (sizeof(T) >= 8) std::swap(p[2], p[sizeof(T) - 3]);
		if constexpr (sizeof(T) >= 8) std::swap(p[3], p[sizeof(T) - 4]);
		return n;
	}

	template<typename T>
	T read_BE()
	{
		return reverted<T, 0>(read<T>());
	}

	template<typename T>
	SIZE read_BE(T& n)
	{
		SIZE d = read(n);
		n	   = reverted<T, 0>(n);
		return d;
	}

	template<typename T>
	SIZE write_BE(const T& n)
	{
		return write<T>(reverted<T, 0>(n));
	}

	template<typename T>
	T read_LE()
	{
		return reverted<T, 1>(read<T>());
	}

	template<typename T>
	SIZE read_LE(T& n)
	{
		SIZE d = read(n);
		n	   = reverted<T, 1>(n);
		return d;
	}

	template<typename T>
	SIZE write_LE(const T& n)
	{
		return write<T>(reverted<T, 1>(n));
	}
};


inline SIZE SerialDevice::read(void*, SIZE, bool) { throw NOT_READABLE; }
inline SIZE SerialDevice::write(const void*, SIZE, bool) { throw NOT_WRITABLE; }

} // namespace kio::Devices
