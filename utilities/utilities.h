// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
#ifdef MAKE_TOOLS
  #include "glue.h"
#else

  #include "LoadSensor.h"
  #include "basic_math.h"
  #include "standard_types.h"
  #include <hardware/clocks.h>
  #include <hardware/pll.h>
  #include <hardware/timer.h>
  #include <hardware/vreg.h>
  #include <pico/stdlib.h>


namespace kio
{

inline CC	now() noexcept { return CC(time_us_32()); }
extern void sleep_us(int usec) noexcept; // power saving mode
inline void sleep_ms(int msec) noexcept { sleep_us(msec * 1000); }

inline void wfe() noexcept;
inline void wfi() noexcept;
extern void wfe_or_timeout(int timeout_usec) noexcept;


extern size_t			   heap_start() noexcept;
extern size_t			   heap_end() noexcept;
extern __deprecated size_t heap_size() noexcept; // use heap_total_size() in malloc.h
extern __deprecated size_t heap_free() noexcept; // use heap_largest_free_block()

extern size_t core0_scratch_y_start() noexcept;
extern size_t core0_scratch_y_end() noexcept;
extern size_t core1_scratch_x_start() noexcept;
extern size_t core1_scratch_x_end() noexcept;

extern size_t core0_stack_bottom() noexcept;
extern size_t core1_stack_bottom() noexcept;
extern size_t stack_bottom(uint core) noexcept;
extern size_t core0_stack_top() noexcept;
extern size_t core1_stack_top() noexcept;
extern size_t stack_top(uint core) noexcept;
extern size_t stack_free() noexcept;

extern size_t flash_binary_end() noexcept;
extern size_t flash_binary_size() noexcept;
extern size_t flash_start() noexcept;
extern size_t flash_end() noexcept;
extern size_t flash_size() noexcept;
extern size_t flash_used() noexcept;
extern size_t flash_free() noexcept;


extern void print_core();
extern void print_heap_free(int r = 0);
extern void print_stack_free();
extern void print_core0_scratch_y_usage();
extern void print_core1_scratch_x_usage();
extern void print_flash_usage();
extern void print_system_info(uint = ~0u);


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

#endif

/*


















*/
