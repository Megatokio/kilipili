// Copyright (c) 2020 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Var.h"
#include "basic_math.h"
#include "standard_types.h"

namespace kio::Vcc
{

class Memory;


/* ==== union used for data register in Runner::execute() ====
*/
union DReg
{
	uint32	u32;
	int32	i32;
	void*	ptr;
	uint8*	u8ptr;
	uint16* u16ptr;
	uint32* u32ptr;
	uint64* u64ptr;
	int8*	i8ptr;
	int16*	i16ptr;
	int32*	i32ptr;
	int64*	i64ptr;
	float*	f32ptr;
	float	f32;
	Memory* memptr;

	DReg(uint32 n) : u32(n) {}
	DReg(int32 n) : i32(n) {}
	DReg(uint16 n) : u32(n) {}
	DReg(int16 n) : i32(n) {}
	DReg(uint8 n) : u32(n) {}
	DReg(int8 n) : i32(n) {}
	DReg(float n) : f32(n) {}
	DReg(void* p) : ptr(p) {}
	DReg(const Var& v) : u32(v.u32) {}

	template<typename T>
	operator T*()
	{
		return reinterpret_cast<T*>(ptr);
	}

	operator uint8() { return uint8(u32); }
	operator uint16() { return uint16(u32); }
	operator uint32() { return u32; }
	operator int8() { return int8(i32); }
	operator int16() { return int16(i32); }
	operator int32() { return i32; }
	operator float() { return f32; }
	operator Var() { return Var(u32); }

	DReg operator~() { return ~u32; }
	//void cpl() { u32 = ~u32; }
	//void sign() { i32 = kio::sign(i32); }
	DReg operator-() { return -i32; }
	//void neg() { i32 = -i32; }
	//void abs() { u32 = kio::abs(i32); }

	DReg& operator+=(int16 n)
	{
		i32 += n;
		return *this;
	}
	DReg& operator+=(uint16 n)
	{
		u32 += n;
		return *this;
	}

	DReg& operator+=(int32 n)
	{
		i32 += n;
		return *this;
	}
	DReg& operator-=(int32 n)
	{
		i32 -= n;
		return *this;
	}
	DReg& operator&=(uint32 n)
	{
		u32 &= n;
		return *this;
	}
	DReg& operator|=(uint32 n)
	{
		u32 |= n;
		return *this;
	}
	DReg& operator^=(uint32 n)
	{
		u32 ^= n;
		return *this;
	}
	DReg& operator*=(int32 n)
	{
		i32 *= n;
		return *this;
	}
	DReg& operator*=(uint32 n)
	{
		u32 *= n;
		return *this;
	}
	DReg& operator*=(int16 n)
	{
		i32 *= n;
		return *this;
	}
	DReg& operator/=(uint32 n)
	{
		u32 /= n;
		return *this;
	}
	DReg& operator/=(int32 n)
	{
		i32 /= n;
		return *this;
	}
	DReg& operator/=(uint16 n)
	{
		u32 /= n;
		return *this;
	}
	DReg& operator/=(int16 n)
	{
		i32 /= n;
		return *this;
	}
	DReg& operator%=(uint32 n)
	{
		u32 %= n;
		return *this;
	}
	DReg& operator%=(int32 n)
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


} // namespace kio::Vcc
