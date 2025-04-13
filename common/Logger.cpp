// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Logger.h"
#include "tempmem.h"
#include <cstdio>

namespace kio
{

Logger logger;


void Logger::purge() noexcept
{
	while (avail()) delete[] (msgs[ri++ & mask]);
}

str Logger::gets() noexcept
{
	mutex.lock();
	if (avail())
	{
		cstr s = msgs[ri++ & mask];
		mutex.unlock();
		str t = dupstr(s);
		delete[] s;
		return t;
	}
	else
	{
		mutex.unlock();
		return nullptr;
	}
}

void Logger::log(cstr s) noexcept
{
	if (free())
	{
		try
		{
			s = newcopy(s);
			mutex.lock();
			if (free())
			{
				msgs[wi++ & mask] = s;
				mutex.unlock();
			}
			else
			{
				mutex.unlock();
				delete[] s;
			}
		}
		catch (...)
		{}
	}
}

void Logger::log(cstr fmt, va_list va) noexcept
{
	if (free())
	{
		va_list va2;
		va_copy(va2, va);

		int n = vsnprintf(nullptr, 0, fmt, va);

		assert(n >= 0);
		uint size = uint(n);

		try
		{
			str s = new char[size + 1];

			vsnprintf(s, size + 1, fmt, va);
			va_end(va2);

			mutex.lock();
			if (free())
			{
				msgs[wi++ & mask] = s;
				mutex.unlock();
			}
			else
			{
				mutex.unlock();
				delete[] s;
			}
		}
		catch (...)
		{}
	}
}

template<>
void Logger::log<0>(cstr msg, ...) noexcept
{
	va_list va;
	va_start(va, msg);
	log(msg, va);
	va_end(va);
}


} // namespace kio
