// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <cmath>

namespace kipili::Graphics
{

struct fixint
{
	static constexpr int ss = 2;

	int n;

	constexpr fixint(int n) noexcept : n(n << ss) {}
	fixint(float n) noexcept : n(int(roundf(n * (1 << ss)))) {}

	constexpr operator int() const { return n >> ss; }

	constexpr fixint operator+=(fixint b)
	{
		n += b.n;
		return *this;
	}
	constexpr fixint operator-=(fixint b)
	{
		n -= b.n;
		return *this;
	}
	constexpr fixint operator*=(fixint b)
	{
		n = (n * b.n) >> ss;
		return *this;
	}
	constexpr fixint operator/=(fixint b)
	{
		n = (n << ss) / b.n;
		return *this;
	}

	constexpr fixint operator+(fixint b) const { return fixint(*this) += b; }
	constexpr fixint operator-(fixint b) const { return fixint(*this) -= b; }
	constexpr fixint operator*(fixint b) const { return fixint(*this) *= b; }
	constexpr fixint operator/(fixint b) const { return fixint(*this) /= b; }

	constexpr fixint operator+=(int b)
	{
		n += b << ss;
		return *this;
	}
	constexpr fixint operator-=(int b)
	{
		n -= b << ss;
		return *this;
	}
	constexpr fixint operator*=(int b)
	{
		n *= b;
		return *this;
	}
	constexpr fixint operator/=(int b)
	{
		n /= b;
		return *this;
	}

	constexpr fixint operator+(int b) const { return fixint(*this) += b; }
	constexpr fixint operator-(int b) const { return fixint(*this) -= b; }
	constexpr fixint operator*(int b) const { return fixint(*this) *= b; }
	constexpr fixint operator/(int b) const { return fixint(*this) /= b; }

	constexpr fixint operator&=(fixint b)
	{
		n &= b.n;
		return *this;
	}
	constexpr fixint operator|=(fixint b)
	{
		n |= b.n;
		return *this;
	}
	constexpr fixint operator^=(fixint b)
	{
		n ^= b.n;
		return *this;
	}
	constexpr fixint operator&(fixint b) const { return b &= *this; }
	constexpr fixint operator|(fixint b) const { return b |= *this; }
	constexpr fixint operator^(fixint b) const { return b ^= *this; }

	constexpr fixint operator>>=(int b)
	{
		n >>= b;
		return *this;
	}
	constexpr fixint operator<<=(int b)
	{
		n <<= b;
		return *this;
	}
	constexpr fixint operator>>(int b) const { return fixint(*this) >>= b; }
	constexpr fixint operator<<(int b) const { return fixint(*this) <<= b; }

	constexpr bool operator==(fixint b) const { return n == b.n; }
	constexpr bool operator!=(fixint b) const { return n != b.n; }
	constexpr bool operator>(fixint b) const { return n > b.n; }
	constexpr bool operator>=(fixint b) const { return n >= b.n; }
	constexpr bool operator<(fixint b) const { return n < b.n; }
	constexpr bool operator<=(fixint b) const { return n <= b.n; }

	constexpr bool operator==(int b) const { return n == b << ss; }
	constexpr bool operator!=(int b) const { return n != b << ss; }
	constexpr bool operator>(int b) const { return n > b << ss; }
	constexpr bool operator>=(int b) const { return n >= b << ss; }
	constexpr bool operator<(int b) const { return n < b << ss; }
	constexpr bool operator<=(int b) const { return n <= b << ss; }
};

constexpr fixint null = 0;
constexpr fixint one  = 1;

inline fixint abs(fixint v) { return v < null ? null - v : v; }


} // namespace kipili::Graphics
