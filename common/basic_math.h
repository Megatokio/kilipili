// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once


// basic math


template<class T, class T2>
inline constexpr T min(T a, T2 b) noexcept
{
	return a < b ? a : b;
}

template<class T, class T2>
inline constexpr T max(T a, T2 b) noexcept
{
	return a > b ? a : b;
}

template<class T, class T2, class T3>
inline constexpr T min(T a, T2 b, T3 c) noexcept
{
	return min(min(a, b), c);
}

template<class T, class T2, class T3>
inline constexpr T max(T a, T2 b, T3 c) noexcept
{
	return max(max(a, b), c);
}

template<class T>
inline int sign(T a) noexcept
{
	return int(a > 0) - int(a < 0);
}

template<class T>
inline T abs(T a) noexcept
{
	return a < 0 ? -a : a;
}

template<class T>
inline T minmax(T a, T n, T e) noexcept
{
	return n <= a ? a : n >= e ? e : n;
}

template<class T>
inline void limit(T a, T& n, T e) noexcept
{
	if (n < a) n = a;
	else if (n > e) n = e;
}
