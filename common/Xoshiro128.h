// Copyright (c) 2020 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <math.h>

/*
	Family of RNG to produce 32 bit random numbers
*/

namespace kio
{

using RNGState128 = uint32[4];
extern uint32 xoshiro128p(RNGState128) noexcept;
extern uint32 xoshiro128pp(RNGState128) noexcept;
extern uint32 xoshiro128ss(RNGState128) noexcept;


class Xoshiro128
{
public:
	uint32 state[4];

	Xoshiro128(const void* q) noexcept;
	Xoshiro128(uint32 seed) noexcept;

	// 0 .. uint32_max
	uint32 next() noexcept { return xoshiro128p(state); }

	// interval [0 .. [1
	float random() noexcept { return ldexp(float(next()), -32); }

	// interval [0 .. [max
	uint32 random(uint32 max) noexcept;
	float  random(float max) noexcept { return ldexp(max * float(next()), -32); }

	void jump() noexcept;	   // skip 2^64 iterations
	void long_jump() noexcept; // skip 2^96 iterations

private:
	void _skip(const uint32 bits[4]) noexcept;
};

} // namespace kio
