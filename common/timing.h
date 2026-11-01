// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/LoadSensor.h"
#include "common/basic_math.h"
#include "common/glue.h"
#ifdef PICO
  #include <hardware/clocks.h>
#endif


#ifndef PICO
extern "C" uint32 time_us_32(); // 32 bit usec timestamp. overflows every ~71 minutes
extern "C" uint64 time_us_64();
extern "C" uint64 make_timeout_time_us(uint32 timeout_us);
extern "C" bool	  best_effort_wfe_or_timeout(uint64);
#endif


// #######################################################################


namespace kilipili
{
#ifdef PICO
inline uint32 get_system_clock() noexcept { return clock_get_hz(clk_sys); }
#else
inline uint32 get_system_clock() noexcept { return 999 MHz; }
#endif

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
