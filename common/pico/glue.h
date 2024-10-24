// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#ifdef LIB_PICO_STDLIB
  #include <hardware/timer.h>
  #include <pico/stdio.h>
  #include <pico/sync.h>
#endif


// You can either override the spinlock number or override the instance, which allows you to use
// a global instance which you acquired with one of the claiming functions from sync.h.
// Or you can override the whole lock and unlock functions.
//
// If KILIPILI_SPINLOCK_NUMBER is defined then the spinlock is acquired and initialized in glue.cpp.
// The default reserves spinlock OS1 which is reserved for OS use.

#ifndef kilipili_lock_spinlock
  #ifndef KILIPILI_SPINLOCK
	#ifndef KILIPILI_SPINLOCK_NUMBER
	  #define KILIPILI_SPINLOCK_NUMBER PICO_SPINLOCK_ID_OS1
	#endif
	#define KILIPILI_SPINLOCK spin_lock_instance(KILIPILI_SPINLOCK_NUMBER)
  #endif
  #define kilipili_lock_spinlock()	 unsigned int _irqs = spin_lock_blocking(KILIPILI_SPINLOCK)
  #define kilipili_unlock_spinlock() spin_unlock(KILIPILI_SPINLOCK, _irqs)
#endif
