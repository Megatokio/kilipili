// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include<atomic>

/* get 32 bit microsecond timestamp. overflows every ~71 minutes
*/
extern uint32 time_us_32();

/* get char from stdin  
*/
extern int getchar_timeout_us(uint32 timeout_us);


namespace kio
{

template<typename T>
T pp_atomic(T& value) noexcept
{
	std::atomic_ref<T> v(value);
	return ++v;
}

template<typename T>
T mm_atomic(T& value) noexcept
{
	std::atomic_ref<T> v(value);
	return --v;
}

}
