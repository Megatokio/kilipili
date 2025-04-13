// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
using uint = unsigned int;


namespace kio
{

#ifdef LIB_PICO_STDLIB
extern void init_stack_guard();
extern void test_stack_guard(uint core);
extern uint calc_stack_guard_min_free(uint core);
#else
inline void init_stack_guard() {}
inline void test_stack_guard(uint) {}
inline uint calc_stack_guard_min_free(uint) { return 0; }
#endif

} // namespace kio
