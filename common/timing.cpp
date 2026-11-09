// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "timing.h"
#include <stdlib.h>
#ifndef PICO
  #include "standard_types.h"
  #include <cerrno>
  #include <chrono>
  #include <ctime>
  #include <thread>
  #include <unistd.h>
#endif


#ifdef _POSIX_TIMERS
uint32 time_us_32()
{
	struct timespec tv;
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &tv);
	return uint32(tv.tv_sec * 1000000 + tv.tv_nsec / 1000);
}

uint64 time_us_64()
{
	struct timespec tv;
	clock_gettime(_POSIX_MONOTONIC_CLOCK, &tv);
	return uint64(tv.tv_sec * 1000000 + tv.tv_nsec / 1000);
}
#endif

#ifndef PICO
void sleep_us(uint delay)
{
	std::this_thread::sleep_for(std::chrono::microseconds(delay)); //
}

//uint64 make_timeout_time_us(uint32 timeout_us)
//{
//	return time_us_64() + timeout_us; //
//}

//bool best_effort_wfe_or_timeout(uint64)
//{
//	return true; // timeout
//}
#endif


namespace kilipili
{

void sleep_us(int delay_usec) noexcept
{
	if (delay_usec > 0)
	{
		idle_start();
		::sleep_us(uint(delay_usec));
		idle_end();
	}
}

void wfe_or_timeout(int timeout_usec) noexcept
{
	if (timeout_usec > 0)
	{
		idle_start();
#ifdef PICO
		::best_effort_wfe_or_timeout(from_us_since_boot(time_us_64() + uint(timeout_usec)));
#else
		::wfe_or_timeout(timeout_usec);
#endif
		idle_end();
	}
}


} // namespace kilipili


/*































*/
