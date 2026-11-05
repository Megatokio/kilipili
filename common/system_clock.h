// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/standard_types.h"


#ifndef VIDEO_MAX_SYSCLOCK_MHz
  #define VIDEO_MAX_SYSCLOCK_MHz 290
#endif


constexpr char UNSUPPORTED_SYSTEM_CLOCK[] = "requested system clock is not supported";


namespace kilipili
{

#ifdef PICO

extern uint32 get_system_clock() noexcept;
extern Error  set_system_clock(uint32 sys_clock = 125 MHz, uint32 max_error = 1 MHz) noexcept;
extern uint	  calc_sysclock_error(uint32 clock) noexcept;

#else

inline uint32 get_system_clock() noexcept { return 999 MHz; }

#endif

} // namespace kilipili
