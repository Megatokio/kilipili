// Copyright (c) 2020 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"


namespace kio::Vcc
{

using vptr = void*;


/* ==== union used for data in globals and stack ====
*/
union Var
{
	int32		i32;
	uint32		u32;
	float		f32;
	void*		ptr;
	const void* cptr;
	cstr		string;

	Var() = default;
	Var(uint q) : u32(q) {}
	Var(int q) : i32(q) {}
	//Var(uint32 q) : u32(q) {}
	//Var(int32 q) : i32(q) {}
	Var(uint16 q) : u32(q) {}
	Var(int16 q) : i32(q) {}
	Var(uint8 q) : u32(q) {}
	Var(int8 q) : i32(q) {}
	Var(float q) : f32(q) {}
	Var(const void* p) : cptr(p) {}

	template<typename T>
	operator T*()
	{
		return reinterpret_cast<T*>(ptr);
	}

	operator int8() { return int8(i32); }
	operator uint8() { return uint8(u32); }
	operator int16() { return int16(i32); }
	operator uint16() { return uint16(u32); }
	operator int32() { return i32; }
	operator uint32() { return u32; }
	operator float() { return f32; }

	Var& operator+=(int16 n)
	{
		i32 += n;
		return *this;
	}
	Var& operator+=(uint16 n)
	{
		u32 += n;
		return *this;
	}

	Var& operator+=(int32 n)
	{
		i32 += n;
		return *this;
	}
	Var& operator-=(int32 n)
	{
		i32 -= n;
		return *this;
	}
	Var& operator&=(uint32 n)
	{
		u32 &= n;
		return *this;
	}
	Var& operator|=(uint32 n)
	{
		u32 |= n;
		return *this;
	}
	Var& operator^=(uint32 n)
	{
		u32 ^= n;
		return *this;
	}
	Var& operator*=(int32 n)
	{
		i32 *= n;
		return *this;
	}
	Var& operator*=(uint32 n)
	{
		u32 *= n;
		return *this;
	}
	Var& operator*=(int16 n)
	{
		i32 *= n;
		return *this;
	}
	Var& operator/=(uint32 n)
	{
		u32 /= n;
		return *this;
	}
	Var& operator/=(int32 n)
	{
		i32 /= n;
		return *this;
	}
	Var& operator/=(uint16 n)
	{
		u32 /= n;
		return *this;
	}
	Var& operator/=(int16 n)
	{
		i32 /= n;
		return *this;
	}
	Var& operator%=(uint32 n)
	{
		u32 %= n;
		return *this;
	}
	Var& operator%=(int32 n)
	{
		i32 %= n;
		return *this;
	}

	bool operator<(uint32 n) { return u32 < n; }
	bool operator>(uint32 n) { return u32 > n; }
	bool operator<=(uint32 n) { return u32 <= n; }
	bool operator>=(uint32 n) { return u32 >= n; }
	bool operator==(uint32 n) { return u32 == n; }
	bool operator!=(uint32 n) { return u32 != n; }

	bool operator<(int32 n) { return i32 < n; }
	bool operator>(int32 n) { return i32 > n; }
	bool operator<=(int32 n) { return i32 <= n; }
	bool operator>=(int32 n) { return i32 >= n; }
	bool operator==(int32 n) { return i32 == n; }
	bool operator!=(int32 n) { return i32 != n; }

	bool operator<(float n) { return f32 < n; }
	bool operator>(float n) { return f32 > n; }
	bool operator<=(float n) { return f32 <= n; }
	bool operator>=(float n) { return f32 >= n; }
	bool operator==(float n) { return f32 == n; }
	bool operator!=(float n) { return f32 != n; }
};

using VarPtr = Var*;


} // namespace kio::Vcc
