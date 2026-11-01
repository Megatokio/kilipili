// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#ifdef SETTINGS_FILE
  #include SETTINGS_FILE
#endif
#include "common/SpinLock.h"
#include "common/standard_types.h"
#ifdef PICO
  #include <hardware/timer.h>
  #include <pico/stdio.h>
  #include <pico/sync.h>
#else
  #include <atomic>
#endif


// define the functions to use for locking the global fast-access spinlock:
//
// if you don't #define anything, then spinlock PICO_SPINLOCK_ID_OS1 will be used.
// This spinlock will be claimed and initialized during statics initialization phase in glue.cpp.
//
// you can #define KILIPILI_SPINLOCK_NUMBER to use your own spinlock number of choice.
// This spinlock will be claimed and initialized during statics initialization phase in glue.cpp.
//
// you can #define kilipili_spinlock_lock() and kilipili_spinlock_unlock() to use your own implementation.
// This spinlock must be initialized very early before it can be used.

#if !defined kilipili_spinlock_lock && !defined kilipili_spinlock_unlock
  #ifdef PICO
	#ifndef KILIPILI_SPINLOCK_NUMBER
	  #define KILIPILI_SPINLOCK_NUMBER PICO_SPINLOCK_ID_OS1
	#endif
	#define kilipili_spinlock		   spin_lock_instance(KILIPILI_SPINLOCK_NUMBER)
	#define kilipili_spinlock_lock()   unsigned int _sr = spin_lock_blocking(kilipili_spinlock)
	#define kilipili_spinlock_unlock() spin_unlock(kilipili_spinlock, _sr)
  #else
extern SpinLock kilipili_spinlock;
	#define kilipili_spinlock_lock()   kilipili_spinlock.lock()
	#define kilipili_spinlock_unlock() kilipili_spinlock.unlock(0)
  #endif
#endif


#ifndef PICO
inline void __wfe() noexcept {}
inline void __wfi() noexcept {}
inline void __sev() noexcept {}
inline void __dmb() noexcept { std::atomic_thread_fence(std::memory_order_acq_rel); }

extern thread_local uint core_num;
inline uint				 get_core_num() noexcept { return core_num; }

struct interp_hw_t
{
	uint32 accum[2] {0};
	size_t base[3] {0};
	uint32 ctrl[2] {0};
};

extern thread_local interp_hw_t interp_hw_array[2];
  #define interp0_hw (&interp_hw_array[0])
  #define interp1_hw (&interp_hw_array[1])

extern "C" void sleep_us(uint delay_usec);
extern "C" void stdio_init_all();
extern "C" int	stdio_getchar_timeout_us(uint32);
  #define save_and_disable_interrupts() 0
   #define restore_interrupts(o)		 (void)(o)

// alias for backward compatibility:
static inline int getchar_timeout_us(uint32_t timeout_us) { return stdio_getchar_timeout_us(timeout_us); }

// for unit test: these are in unit_test/Mock/MockFlash.cpp
extern "C" void flash_range_erase(uint32 flash_offs, uint32 count);
extern "C" void flash_range_program(uint32 flash_offs, const uint8* data, uint32 count);
#endif


namespace kilipili
{
extern volatile int	 hot_row;		   // Sprite
extern SpinLock		 sprites_spinlock; // Sprite
extern bool			 hotlist_overflow; // MultiSpritesPlane
extern uint			 scanlines_missed; // VideoController
extern volatile bool locked_out;	   // VideoController
} // namespace kilipili


/*







































*/
