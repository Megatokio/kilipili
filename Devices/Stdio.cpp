// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Stdio.h"
#include "tempmem.h"
#include <cstring>
#include <pico/stdio.h>
#include <stdarg.h>
#include <stdio.h>

namespace kio::Devices
{

uint32 Stdio::ioctl(IoCtl ctl, void*, void*)
{
	switch (ctl.cmd)
	{
	case IoCtl::FLUSH_OUT: stdio_flush(); break;
	case IoCtl::FLUSH_IN:
		while (getchar_timeout_us(0) >= 0) {}
		break;
	default: throw INVALID_ARGUMENT;
	}
	return 0;
}

static cstr getchar_error(int err)
{
	char bu[40];
	snprintf(bu, 40, "getchar_timeout_us returned %i", err);
	return dupstr(bu);
}

int Stdio::getc(uint timeout_us)
{
	int c	  = getchar_timeout_us(timeout_us);
	last_char = char(c);
	if (c >= 0) return c;
	if (c == PICO_ERROR_TIMEOUT) return c;
	throw getchar_error(c);
}

char Stdio::getc()
{
	for (;;)
	{
		int c	  = getchar_timeout_us(60 * 1000 * 1000);
		last_char = char(c);
		if (c >= 0) return char(c);
		if (c == PICO_ERROR_TIMEOUT) continue;
		throw getchar_error(c);
	}
}

SIZE Stdio::read(void* _data, SIZE sz, bool partial)
{
	ptr data = ptr(_data);

	for (uint i = 0; i < sz; i++)
	{
		int c = getchar_timeout_us(0);
		if (c >= 0) data[i] = char(c);
		else if (partial && c == PICO_ERROR_TIMEOUT) return i;
		else throw getchar_error(c);
	}
	return sz;
}

SIZE Stdio::write(const void* _data, SIZE sz, __unused bool partial)
{
	cptr data = cptr(_data);

	for (uint i = 0; i < sz; i++) putchar_raw(data[i]);
	return sz;
}

SIZE Stdio::putc(char c)
{
	// TODO putchar() probably inserts cr before any nl.
	// add a binary setting?

	putchar_raw(c);
	return 1;
}

SIZE Stdio::puts(cstr s)
{
	// the unix puts() always appends a line end.
	// TODO perhaps extend puts by taking a flag?

	// printf probably inserts cr before any nl.
	// TODO add a binary setting?

	return printf("%s", s);
}

SIZE Stdio::printf(cstr fmt, ...)
{
	// printf probably inserts cr before nl.
	// TODO add a binary setting?

	va_list va;
	va_start(va, fmt);
	int n = vprintf(fmt, va);
	assert(n >= 0);
	va_end(va);
	return SIZE(n);
}


} // namespace kio::Devices
