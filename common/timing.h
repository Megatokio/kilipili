// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

//#include "cdefs.h"
#include "basic_math.h"
#include "glue.h"
#include "pico_hw/LoadSensor.h"


#ifndef LIB_PICO_STDLIB

extern "C" uint32 time_us_32(); // 32 bit usec timestamp. overflows every ~71 minutes
extern "C" uint64 time_us_64();
extern "C" uint64 make_timeout_time_us(uint32 timeout_us);
extern "C" bool	  best_effort_wfe_or_timeout(uint64);
#endif


// #######################################################################


namespace kilipili
{
inline CC	now() noexcept { return CC(time_us_32()); }
extern void sleep_us(int usec) noexcept; // power saving mode
inline void sleep_ms(int msec) noexcept { sleep_us(msec * 1000); }
extern void wfe_or_timeout(int timeout_usec) noexcept;

inline void wfe() noexcept
{
	idle_start();
	__wfe(); // __asm volatile("wfe");
	idle_end();
}

inline void wfi() noexcept
{
	idle_start();
	__wfi(); // __asm volatile("wfi");
	idle_end();
}

} // namespace kilipili


/*


















*/
