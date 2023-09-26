// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "kilipili_common.h"
#include <hardware/timer.h>
#include <pico/stdlib.h>


#ifndef SYSCLOCK_fMAX
  #define SYSCLOCK_fMAX (290 MHz)
#endif


constexpr Error UNSUPPORTED_SYSTEM_CLOCK = "requested system clock is not supported";


namespace kio
{

inline float now() noexcept { return float(time_us_64()) / 1e6f; }
inline void	 sleep(float d) noexcept { sleep_us(uint64(d * 1e6f)); }

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


extern int sm_print_load(); // state machine


extern uint32 system_clock;

extern Error checkSystemClock(uint32 new_clock);
extern Error checkSystemClock(uint32 new_clock, uint* f_vco, uint* div1, uint* div2);
extern Error setSystemClock(uint32 sys_clock = 125 MHz);


inline constexpr bool is_hex_digit(uchar c) noexcept
{
	return uchar(c - '0') <= '9' - '0' || uchar((c | 0x20) - 'a') <= 'f' - 'a';
}
inline constexpr uint hex_digit_value(uchar c) noexcept // non-digits ≥ 36
{
	return c <= '9' ? uchar(c - '0') : uchar((c | 0x20) - 'a') + 10;
}
inline constexpr bool is_fup(uchar c) noexcept { return schar(c) < schar(0xc0); }


} // namespace kio
