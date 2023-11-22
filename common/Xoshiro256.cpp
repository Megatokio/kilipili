/*
Written in 2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)
Modified for his own use by kio in 2020.

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>.
See	<http://prng.di.unimi.it>
*/

#include "Xoshiro256.h"
#include "basic_math.h"
#include <math.h>
#include <random>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// select generator:
#ifndef XOSHIRO256
  #define XOSHIRO256 xoshiro256pp
#endif


namespace kio
{

static inline uint64 rotl(const uint64 x, int k) noexcept { return (x << k) | (x >> (64 - k)); }


uint64 xoshiro256p(uint64* s) noexcept
{
	// This is xoshiro256+ 1.0, our best and fastest generator for floating-point
	// numbers. We suggest to use its upper bits for floating-point
	// generation, as it is slightly faster than xoshiro256++/xoshiro256**. It
	// passes all tests we are aware of except for the lowest three bits,
	// which might fail linearity tests (and just those), so if low linear
	// complexity is not considered an issue (as it is usually the case) it
	// can be used to generate 64-bit outputs, too.
	//
	// Use a sign test to extract a random Boolean value,
	// and	right shifts to extract subsets of bits.
	// The state must be seeded so that it is not everywhere zero.

	const uint64 result = s[0] + s[3];
	const uint64 t		= s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}

uint64 xoshiro256pp(uint64* s) noexcept
{
	// This is xoshiro256++ 1.0, one of our all-purpose, rock-solid generators.
	// It has excellent (sub-ns) speed, a state (256 bits) that is large
	// enough for any parallel application, and it passes all tests we are
	// aware of.
	//
	// For generating just floating-point numbers, xoshiro256+ is even faster.
	// The state must be seeded so that it is not everywhere zero.

	const uint64 result = rotl(s[0] + s[3], 23) + s[0];
	const uint64 t		= s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}

uint64 xoshiro256ss(uint64* s) noexcept
{
	// This is xoshiro256** 1.0, one of our all-purpose, rock-solid
	// generators. It has excellent (sub-ns) speed, a state (256 bits) that is
	// large enough for any parallel application, and it passes all tests we
	// are aware of.
	//
	// For generating just floating-point numbers, xoshiro256+ is even faster.
	// The state must be seeded so that it is not everywhere zero.

	const uint64 result = rotl(s[1] * 5, 7) * 9;
	const uint64 t		= s[1] << 17;

	s[2] ^= s[0];
	s[3] ^= s[1];
	s[1] ^= s[2];
	s[0] ^= s[3];

	s[2] ^= t;

	s[3] = rotl(s[3], 45);

	return result;
}

uint32 Xoshiro256::random(uint32 max) noexcept
{
	// interval [0 .. [max

	if (max == 0u) return uint32(next());

	const uint n = 31 - msbit(max);

	for (;;)
	{
		uint32 r = uint32(next()) >> n;
		if (r < max) return r;
	}
}

uint64 Xoshiro256::random(uint64 max) noexcept
{
	// interval [0 .. [max

	if (max == 0u) return next();

	const uint n = 63 - msbit(max);

	for (;;)
	{
		uint64 r = next() >> n;
		if (r < max) return r;
	}
}

void Xoshiro256::_skip(const uint64 bits[4]) noexcept
{
	// helper for jump() and jump_long():

	uint64 s0 = 0;
	uint64 s1 = 0;
	uint64 s2 = 0;
	uint64 s3 = 0;

	for (uint i = 0; i < 256 / 64; i++)
		for (uint b = 0; b < 64; b++)
		{
			if (bits[i] & UINT64_C(1) << b)
			{
				s0 ^= state[0];
				s1 ^= state[1];
				s2 ^= state[2];
				s3 ^= state[3];
			}
			next();
		}

	state[0] = s0;
	state[1] = s1;
	state[2] = s2;
	state[3] = s3;
}

void Xoshiro256::jump() noexcept
{
	// This is the short jump function for the generator. It is equivalent
	// to 2^128 calls to next(); it can be used to generate 2^128
	// non-overlapping subsequences for parallel computations.

	static const uint64 bits[4] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
	_skip(bits);
}

void Xoshiro256::long_jump() noexcept
{
	// This is the long jump function for the generator. It is equivalent to
	// 2^192 calls to next(); it can be used to generate 2^64 starting points,
	// from each of which jump() will generate 2^64 non-overlapping
	// subsequences for parallel distributed computations.

	static const uint64 bits[4] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241, 0x39109bb02acbe635};
	_skip(bits);
}

Xoshiro256::Xoshiro256(const void* q) noexcept
{
	memcpy(state, q, sizeof state);
	next();
}

Xoshiro256::Xoshiro256(uint32 seed) noexcept
{
	srand(seed);
	memset(state, 0xf5, sizeof(state));
	state[0] = unsigned(rand());
	next();
}

} // namespace kio
