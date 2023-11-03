// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <type_traits>

// basic math

namespace kio
{

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
inline constexpr int sign(T a) noexcept
{
	return int(a > 0) - int(a < 0);
}

template<class T>
inline constexpr T abs(T a) noexcept
{
	return a < 0 ? -a : a;
}
inline constexpr uint8	abs(int8 a) noexcept { return a < 0 ? uint8(-a) : uint8(a); }
inline constexpr uint16 abs(int16 a) noexcept { return a < 0 ? uint16(-a) : uint16(a); }
inline constexpr uint32 abs(int32 a) noexcept { return a < 0 ? uint32(-a) : uint32(a); }
inline constexpr uint64 abs(int64 a) noexcept { return a < 0 ? uint64(-a) : uint64(a); }

template<class T>
inline constexpr T minmax(T a, T n, T e) noexcept
{
	// note: upper limit is incl.
	return n <= a ? a : n >= e ? e : n;
}

template<class T>
inline void limit(T a, T& n, T e) noexcept
{
	// note: upper limit is incl.
	if (n < a) n = a;
	else if (n > e) n = e;
}

template<typename T, typename T2>
inline constexpr T2 map_range(T value, T qmax, T2 zmax)
{
	return T2(float(value) / float(qmax) * float(zmax) + 0.5f);
}

template<>
inline constexpr float map_range(float value, float qmax, float zmax)
{
	return value / qmax * zmax;
}

/** Calculate base 2 logarithm or 'position' of leftmost '1' bit 
	return:	msbit(n>0) = int(log(2,n)) = 0 .. 31
	note:	msbit(n=1) = 0
	caveat:	msbit(n=0) = 0		// illegal argument!
			msbit(n<0) = 31		// illegal argument!
*/
template<typename T>
inline constexpr uint msbit(T n) noexcept
{
	uint b = 0;
	for (uint i = sizeof(T) << 2; i; i >>= 1)
	{
		if (n >> (b + i)) b += i;
	}
	return b;
}


static_assert(msbit(1u) == 0);
static_assert(msbit(2) == 1);
static_assert(msbit(3u) == 1);
static_assert(msbit(4u) == 2);
static_assert(msbit(15) == 3);
static_assert(msbit(16) == 4);
static_assert(msbit(0x3fu) == 5);
static_assert(msbit(0x40u) == 6);
static_assert(msbit(0x401) == 10);
static_assert(msbit(~0u >> 1) == 30);


} // namespace kio


/*


























*/
