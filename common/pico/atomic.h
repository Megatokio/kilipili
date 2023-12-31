// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <pico/sync.h>

#ifndef SPINLOCK_ATOMIC
  #define SPINLOCK_ATOMIC 30
#endif

namespace kio
{

template<typename T>
T pp_atomic(T& value) noexcept
{
	uint irqs = spin_lock_blocking(spin_lock_instance(SPINLOCK_ATOMIC));
	T	 rval = ++value;
	spin_unlock(spin_lock_instance(SPINLOCK_ATOMIC), irqs);
	return rval;
}

template<typename T>
T mm_atomic(T& value) noexcept
{
	uint irqs = spin_lock_blocking(spin_lock_instance(SPINLOCK_ATOMIC));
	T	 rval = --value;
	spin_unlock(spin_lock_instance(SPINLOCK_ATOMIC), irqs);
	return rval;
}


} // namespace kio
