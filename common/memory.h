// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/standard_types.h"


namespace kilipili
{
#ifdef PICO
extern size_t heap_start() noexcept;
extern size_t heap_end() noexcept;

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
#endif

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


void assert_stack_free(cstr function_name, int required_free_size) noexcept;

} // namespace kilipili


/*


















*/
