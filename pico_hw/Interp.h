// Copyright (c) 2025 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/basic_math.h"
#include "common/cdefs.h"
#include "common/standard_types.h"
#ifndef LIB_PICO_STDLIB

#else
  #include <hardware/interp.h>
#endif


/*
	HW Interpolator or SW emulation thereof

	current implementation:
	RP2040 (no ROTATE instead of SHIFT)
	no clamping mode (INTERP1)
	no interpolation mode (INTERP0)
	no reading from ctrl register
	on 64bit the the base registers and results are 64bit. this may cause problems.
*/

#ifndef LIB_PICO_STDLIB

struct interp_hw_t
{
	uint32 accum[2] {0}; // read/write
	size_t base[3] {0};	 // read/write
						 //	uint32 pop[3];	// read/write
						 //	uint32 peek[3]; // read/write
	uint32 ctrl[2] {0};	 // read/write
						 // uint32 add_raw[2]; // read/write
						 //	uint32 base01;	   // write only

	// Control register for lane 0
	// 0x02000000 [25]    OVERF        (0) Set if either OVERF0 or OVERF1 is set
	// 0x01000000 [24]    OVERF1       (0) Indicates if any masked-off MSBs in ACCUM1 are set
	// 0x00800000 [23]    OVERF0       (0) Indicates if any masked-off MSBs in ACCUM0 are set
	// 0x00200000 [21]    BLEND        (0) Only present on INTERP0 on each core
	// 0x00180000 [20:19] FORCE_MSB    (0) ORed into bits 29:28 of the lane result presented to the...
	// 0x00040000 [18]    ADD_RAW      (0) If 1, mask + shift is bypassed for LANE0 result
	// 0x00020000 [17]    CROSS_RESULT (0) If 1, feed the opposite lane's result into this lane's...
	// 0x00010000 [16]    CROSS_INPUT  (0) If 1, feed the opposite lane's accumulator into this...
	// 0x00008000 [15]    SIGNED       (0) If SIGNED is set, the shifted and masked accumulator...
	// 0x00007c00 [14:10] MASK_MSB     (0) The most-significant bit allowed to pass by the mask...
	// 0x000003e0 [9:5]   MASK_LSB     (0) The least-significant bit allowed to pass by the mask (inclusive)
	// 0x0000001f [4:0]   SHIFT        (0) Logical right-shift applied to accumulator before masking
};
#endif


struct Interp : public interp_hw_t
{
#ifndef LIB_PICO_STDLIB

	static constexpr int SIGNED		  = 15;
	static constexpr int CROSS_INPUT  = 16;
	static constexpr int CROSS_RESULT = 17;
	static constexpr int ADD_RAW	  = 18;

	void set_base01(uint32 n) noexcept
	{
		base[0] = uint16(n);
		base[1] = n >> 16;
	}

	void add_raw(int i, uint32 n) noexcept
	{
		assert(uint(i) < 2);
		accum[i] += n;
	}

	size_t calc_lane_result(int i, size_t* result2) noexcept
	{
		uint32 ctrl = this->ctrl[i];

		int	   cross_input = (ctrl >> CROSS_INPUT) & 1;
		uint32 a		   = accum[i ^ cross_input];

		bool add_raw = (ctrl >> ADD_RAW) & 1;
		if (result2 == nullptr && add_raw) return base[i] + a;

		int	   shift = ctrl & 31; // bits 4:0
		uint32 b	 = a >> shift;

		int mask_lsb = (ctrl >> 5) & 31; // bits 9:5
		b &= 0xFFFFFFFF << mask_lsb;

		int mask_msb = (ctrl >> 10) & 31; // bits 14:10
		b &= 0xFFFFFFFF >> (31 - mask_msb);

		bool sign_extend = (ctrl >> SIGNED) & 1;
		if (sign_extend && (b & (1 << mask_msb))) b |= size_t(~0) << mask_msb;
		if (result2) result2 += b;

		return base[i] + (add_raw ? a : b);
	}

	size_t peek(int i) noexcept
	{
		assert(uint(i) < 3);
		if (i < 2) return calc_lane_result(i, nullptr);
		size_t result2 = base[2];
		calc_lane_result(0, &result2);
		calc_lane_result(1, &result2);
		return result2;
	}

	size_t pop(int i) noexcept
	{
		assert(uint(i) < 3);
		size_t result[3];
		result[2] = base[2];
		result[0] = calc_lane_result(0, i == 2 ? &result[2] : nullptr);
		result[1] = calc_lane_result(1, i == 2 ? &result[2] : nullptr);

		accum[0] = uint32(result[(ctrl[0] >> CROSS_RESULT) & 1]);
		accum[1] = uint32(result[(~ctrl[1] >> CROSS_RESULT) & 1]);
		return result[i];
	}


#else // pico
	Interp() = delete;
	uint32 __always_inline pop(int i) noexcept { return interp_hw_t::pop[i]; }
	uint32 __always_inline peek(int i) noexcept { return interp_hw_t::peek[i]; }
	void __always_inline   set_base01(uint32 n) noexcept { interp_hw_t::base01 = n; }
	void __always_inline   add_raw(int i, uint32 n) noexcept { interp_hw_t::add_raw[i] = n; }
#endif
};


#ifdef LIB_PICO_STDLIB
static_assert(sizeof(interp_hw_t) == 16 * 4);
static_assert(SIO_INTERP1_ACCUM0_OFFSET - SIO_INTERP0_ACCUM0_OFFSET == sizeof(Interp));
#endif


/*





























*/
