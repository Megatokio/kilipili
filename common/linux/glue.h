// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"

/* get 32 bit microsecond timestamp. overflows every ~71 minutes
*/
extern uint32 time_us_32();
extern uint64 time_us_64();

/* get char from stdin  
*/
extern int getchar_timeout_us(uint32 timeout_us);

extern uint64 make_timeout_time_us(uint32 timeout_us);
extern bool	  best_effort_wfe_or_timeout(uint64);


// #######################################################################

namespace kio
{

inline void sleep_us(int usec) noexcept { (void)usec; }
inline void wfe() noexcept {}

} // namespace kio


#define kilipili_lock_spinlock()   (void)0
#define kilipili_unlock_spinlock() (void)0
