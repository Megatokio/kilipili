// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Mutex.h"
#include <stdarg.h>

namespace kio
{

/*
   Log message store 
   for use while messages can't be displayed, e.g. during file transfer over the serial line.

   only supports puts() and printf() to store messages
   and gets() to retrieve them.
*/

class Logger
{
public:
	Logger() noexcept = default;
	~Logger() noexcept { purge(); }

	str	 gets() noexcept;
	void log(cstr) noexcept;
	void log(cstr, va_list) noexcept __printflike(2, 0);

	template<int = 0>
	void log(cstr msg, ...) noexcept __printflike(2, 3);

	void purge() noexcept;

private:
	static constexpr uint	qsize = 8;
	static constexpr uint16 mask  = qsize - 1;

	Mutex  mutex;
	uint16 ri = 0, wi = 0;
	cstr   msgs[qsize] = {nullptr};

	bool avail() const noexcept { return wi != ri; }
	bool free() const noexcept { return wi - ri != qsize; }
};

extern Logger logger;

#define logline logger.log

template<>
void Logger::log<0>(cstr msg, ...) noexcept;

} // namespace kio
