// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "glue.h"
#include <ctime>
#include <unistd.h>


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

uint64 make_timeout_time_us(uint32 timeout_us)
{
	return time_us_64() + timeout_us; //
}

#endif

bool best_effort_wfe_or_timeout(uint64)
{
	return true; // timeout
}

int getchar_timeout_us(uint32)
{
	return -1; // no char
}
