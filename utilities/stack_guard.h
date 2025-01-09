// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"


namespace kio
{
extern void init_stack_guard();
extern void test_stack_guard(uint core);
extern uint calc_stack_guard_min_free(uint core);

} // namespace kio
