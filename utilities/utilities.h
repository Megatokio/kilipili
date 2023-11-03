// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "LoadSensor.h"
#include "standard_types.h"
#include <hardware/clocks.h>
#include <hardware/timer.h>
#include <pico/stdlib.h>


#ifndef SYSCLOCK_fMAX
  #define SYSCLOCK_fMAX (290 MHz)
#endif


constexpr Error UNSUPPORTED_SYSTEM_CLOCK = "requested system clock is not supported";


namespace kio
{

inline float now() noexcept { return float(time_us_64()) / 1e6f; }
extern void	 sleep_us(int usec) noexcept; // power saving mode
inline void	 sleep_ms(int msec) noexcept { sleep_us(msec * 1000); }
inline void	 sleep(float d) noexcept { sleep_us(int32(d * 1e6f)); }

inline void wfe() noexcept;
inline void wfi() noexcept;
extern void wfe_or_timeout(int timeout_usec) noexcept;


extern size_t flash_size();
extern size_t flash_used();
extern size_t flash_free();
extern size_t heap_size();
extern size_t calc_heap_free();
extern ptr	  core0_stack_bottom();
extern ptr	  core1_stack_bottom();
extern ptr	  stack_bottom(uint core);
extern ptr	  core0_stack_end();
extern ptr	  core1_stack_end();
extern ptr	  stack_end(uint core);
extern size_t stack_free();

extern void print_core();
extern void print_heap_free(int r = 0);
extern void print_stack_free();
extern void print_core0_scratch_y_usage();
extern void print_core1_scratch_x_usage();
extern void print_flash_usage();
extern void print_system_info(uint = ~0u);

extern void init_stack_guard();
extern void test_stack_guard(uint core);
extern uint calc_stack_guard_min_free(uint core);


inline uint32 get_system_clock() { return clock_get_hz(clk_sys); }
extern Error  set_system_clock(uint32 sys_clock = 125 MHz, uint32 max_error = 1 MHz);


//
// **************** Implementations *************************
//

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


} // namespace kio

/*


















*/
