// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once


#include "common/standard_types.h"
#ifndef PICO
  #include <mutex>
#else
  #include <pico/sync.h>
#endif


struct SpinLock
{
	enum { STRIPED = -10, UNUSED };
	SpinLock(int num_or_set = STRIPED) noexcept;
	~SpinLock() noexcept;

	uint32 lock(); // blocking
	void   unlock(uint32) noexcept;
	bool   try_lock_unsafe() noexcept;

#ifndef PICO

	std::mutex mutex;
#else
	spin_lock_t* spinlock;
#endif
};


#ifndef PICO
inline SpinLock::SpinLock(int) noexcept {}
inline SpinLock::~SpinLock() noexcept {}
inline uint32 SpinLock::lock()
{
	mutex.lock();
	return 0;
}
inline void SpinLock::unlock(uint32) noexcept { mutex.unlock(); }
#else

inline SpinLock::SpinLock(int id) noexcept
{
	if (id == STRIPED) spinlock = spin_lock_instance(next_striped_spin_lock_num());
	else
	{
		if (id == UNUSED) id = spin_lock_claim_unused(true);
		else spin_lock_claim(uint(id));
		spinlock = spin_lock_init(uint(id));
	}
}
inline SpinLock::~SpinLock() noexcept { spin_lock_unclaim(spin_lock_get_num(spinlock)); }

inline uint32 SpinLock::lock() { return spin_lock_blocking(spinlock); }
inline void	  SpinLock::unlock(uint32 sr) noexcept { spin_unlock(spinlock, sr); }
inline bool	  SpinLock::try_lock_unsafe() noexcept { return spin_try_lock_unsafe(spinlock); }

#endif
