// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once


// #if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))

#ifdef __unix__
  #include <mutex>
#else
  #include "standard_types.h"
  #include <pico/sync.h>
#endif


struct SpinLock
{
	enum { STRIPED = -1 };
	SpinLock(int num_or_set = STRIPED);

	void lock(); // blocking
	void unlock();

#ifdef __unix__
	std::mutex mutex;
#else
	spin_lock_t* spinlock;
	uint32		 sr;
#endif
};


#ifdef __unix__
inline SpinLock::SpinLock(int) {}
inline void SpinLock::lock() { mutex.lock(); }
inline void SpinLock::unlock() { mutex.unlock(); }
#else

inline SpinLock::SpinLock(int id)
{
	if (id == STRIPED) spinlock = spin_lock_instance(next_striped_spin_lock_num());
	else TODO();
}

inline void SpinLock::lock() noexcept { sr = spin_lock_blocking(spinlock); }
inline void SpinLock::unlock() noexcept { spin_unlock(spinlock, sr); }

#endif
