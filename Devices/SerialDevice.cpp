// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "SerialDevice.h"
#include "tempmem.h"
#include "utilities/utilities.h"
#include <memory>
#include <stdarg.h>
#include <string.h>

namespace kio::Devices
{

uint32 SerialDevice::ioctl(IoCtl cmd, void*, void*)
{
	// default implementation

	switch (cmd.cmd)
	{
	case IoCtl::FLUSH_OUT: return 0;
	default: throw INVALID_ARGUMENT;
	}
}

int SerialDevice::getc(uint timeout_us)
{
	// default implementation for getc() with timeout

	auto timeout_time = make_timeout_time_us(timeout_us);
	for (;;)
	{
		if (read(&last_char, 1, true)) return uchar(last_char);
		if (best_effort_wfe_or_timeout(timeout_time)) return -1;
	}
}

char SerialDevice::getc()
{
	while (read(&last_char, 1, true) == 0) { wfe(); }
	return last_char;
}

str SerialDevice::gets(uint line_ends)
{
	// read string from Device
	// handles DOS line ends

	char buffer[gets_max_len];
	char last_eol = this->last_char;

	uint i = 0;
	while (i < gets_max_len)
	{
		char c = getc();
		if (uchar(c) >= 32 || (1 << c) & ~line_ends)
		{
			buffer[i++] = c;
			continue;
		}

		// skip cr after nl or vice versa:
		if (i == 0 && c == 23 - last_eol)
		{
			last_eol = 0;
			continue;
		}

		// line end:
		break;
	}

	str s = tempstr(i);
	memcpy(s, buffer, i);
	return s;
}

void SerialDevice::putc(char c)
{
	write(&c, 1); //
}

void SerialDevice::puts(cstr s)
{
	if (s) write(s, strlen(s)); //
}

void SerialDevice::printf(cstr fmt, ...)
{
	char bu[256];

	va_list va;
	va_start(va, fmt);
	int n = vsnprintf(bu, sizeof(bu), fmt, va);
	va_end(va);

	assert(n >= 0);
	SIZE size = uint(n);
	if (size <= 255) return (void)write(bu, size);

	std::unique_ptr<char[]> bp {new (std::nothrow) char[size + 1]};

	if (!bp) return (void)write(bu, 256); // desperately short of memory

	va_start(va, fmt);
	vsnprintf(bp.get(), size + 1, fmt, va);
	va_end(va);

	return (void)write(bp.get(), size);
}


} // namespace kio::Devices


/*






































*/
