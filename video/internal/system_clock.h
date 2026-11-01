// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/standard_types.h"


#ifndef VIDEO_MAX_SYSCLOCK_MHz
  #define VIDEO_MAX_SYSCLOCK_MHz 290
#endif


namespace kilipili
{

extern uint32 get_system_clock() noexcept; // --> glue.cpp
extern void	  set_system_clock(uint32 sys_clock) noexcept;
extern uint32 calc_sysclock_error(uint32 clock) noexcept;

} // namespace kilipili
