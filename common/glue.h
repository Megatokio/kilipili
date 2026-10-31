// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#ifdef SETTINGS_FILE
  #include SETTINGS_FILE
#endif
#ifndef LIB_PICO_STDLIB
  #include "pico_hw/SpinLock.h"
  #include "standard_types.h"
  #include <atomic>
#else
  #include <hardware/timer.h>
  #include <pico/stdio.h>
  #include <pico/sync.h>
#endif


#ifndef LIB_PICO_STDLIB
inline void __wfe() noexcept {}
inline void __wfi() noexcept {}
inline void __sev() noexcept {}
inline void __dmb() noexcept { std::atomic_thread_fence(std::memory_order_acq_rel); }

extern thread_local int core_num;
inline int				get_core_num() noexcept { return core_num; }

extern int getchar_timeout_us(uint32 timeout_us);
  #define save_and_disable_interrupts() 0
  #define restore_interrupts(o)			(void)(o)
#endif


// define the functions to use for locking the global kilipili fast-access spinlock:
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
  #ifndef LIB_PICO_STDLIB
extern SpinLock kilipili_spinlock;
	#define kilipili_spinlock_lock()   kilipili_spinlock.lock()
	#define kilipili_spinlock_unlock() kilipili_spinlock.unlock(0)

  #else
	#ifndef KILIPILI_SPINLOCK_NUMBER
	  #define KILIPILI_SPINLOCK_NUMBER PICO_SPINLOCK_ID_OS1
	#endif
	#define kilipili_spinlock		   spin_lock_instance(KILIPILI_SPINLOCK_NUMBER)
	#define kilipili_spinlock_lock()   unsigned int _sr = spin_lock_blocking(kilipili_spinlock)
	#define kilipili_spinlock_unlock() spin_unlock(kilipili_spinlock, _sr)
  #endif
#endif
