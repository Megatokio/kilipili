// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
//#include "cdefs.h"
#ifdef MAKE_TOOLS
  #include "glue.h"
#else

  #include "LoadSensor.h"
  #include "basic_math.h"
//  #include "standard_types.h"
//  #include <hardware/clocks.h>
//  #include <hardware/pll.h>
//  #include <hardware/timer.h>
//  #include <hardware/vreg.h>
//  #include <pico/stdlib.h>


namespace kio
{

inline CC	now() noexcept { return CC(time_us_32()); }
extern void sleep_us(int usec) noexcept; // power saving mode
inline void sleep_ms(int msec) noexcept { sleep_us(msec * 1000); }

inline void wfe() noexcept
{
	idle_start();
	__asm volatile("wfe");
	idle_end();
}

inline void wfi() noexcept
{
	idle_start();
	__asm volatile("wfi");
	idle_end();
}

extern void wfe_or_timeout(int timeout_usec) noexcept;


} // namespace kio

#endif

/*


















*/
