// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <atomic>


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

} // namespace kio
