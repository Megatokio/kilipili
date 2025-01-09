// Copyright (c) 2023 - 2024 kio@little-bat.de
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
	return a < 0 ? -1 : a > 0;
}

template<class T>
inline constexpr T abs(T a) noexcept
{
	return a < 0 ? -a : a;
}
inline constexpr int abs(int8 a) noexcept { return a < 0 ? int8(-a) : int8(a); }
inline constexpr int abs(int16 a) noexcept { return a < 0 ? int16(-a) : int16(a); }
//inline constexpr int32 abs(int32 a) noexcept { return a < 0 ? int32(-a) : int32(a); }
//inline constexpr int64 abs(int64 a) noexcept { return a < 0 ? int64(-a) : int64(a); }

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
	if (n < 0) return sizeof(T) * CHAR_BIT - 1;

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


/*	circular ints can be viewed as points on the circular range of an int,
	wrapping from max_int to min_int.
	when comparing two circular ints "which is before" or "which is after"
	overflow is handled differently to signed or unsigned ints.
	circular ints are great for comparing time stamps which may roll over time and time again,
	as long as the expected time distance is smaller than 1/2 of the representable range.

	viewing circular ints as "points" on the circular ring tells which operations are reasonable.
	plain ints are the distance between two circular int points:

	c_int + int = c_int			point + distance = point
	c_int - c_int = int			distance between two points
	c_int << int = c_int		e.g. scale int to fixed point
	c_int >> int = c_int		makes sense but is not possible because we don't know the msbits.

	ATTN:  int(a)-int(b)  !=  int(uint(a)-uint(b))  !!!  thanks god we have the C++ committee!
*/
struct CC
{
	CC() noexcept {}
	template<typename T>
	constexpr explicit CC(T n) noexcept : value(uint(n))
	{}

	constexpr explicit operator uint() const noexcept { return value; }
	constexpr explicit operator int() const noexcept { return int(value); }

	constexpr void operator+=(int o) noexcept { value += uint(o); }
	constexpr void operator-=(int o) noexcept { value -= uint(o); }
	constexpr void operator+=(uint o) noexcept { value += o; }
	constexpr void operator-=(uint o) noexcept { value -= o; }

	constexpr CC  operator+(int d) const noexcept { return CC(value + uint(d)); }
	constexpr CC  operator-(int d) const noexcept { return CC(value - uint(d)); }
	constexpr CC  operator+(uint d) const noexcept { return CC(value + d); }
	constexpr CC  operator-(uint d) const noexcept { return CC(value - d); }
	constexpr int operator-(CC d) const noexcept { return int(value - d.value); }
	constexpr CC  operator<<(int d) const noexcept { return CC(value << d); }

	constexpr bool operator==(CC o) const noexcept { return value - o.value == 0; }
	constexpr bool operator!=(CC o) const noexcept { return value - o.value != 0; }
	constexpr bool operator>(CC o) const noexcept { return int(value - o.value) > 0; }
	constexpr bool operator>=(CC o) const noexcept { return int(value - o.value) >= 0; }
	constexpr bool operator<(CC o) const noexcept { return int(value - o.value) < 0; }
	constexpr bool operator<=(CC o) const noexcept { return int(value - o.value) <= 0; }

private:
	uint value = 0;
};

} // namespace kio


/*


























*/
