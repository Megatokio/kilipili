// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/Color.h"
#include "common/basic_math.h"
#include "common/standard_types.h"
#include <hardware/interp.h>


namespace kilipili
{

/*	Interpolator setup for the ScanlineRenderers
*/


struct InterpConfig
{
	uint32 c;

	constexpr inline InterpConfig() noexcept : c(SIO_INTERP0_CTRL_LANE0_MASK_MSB_BITS) {} // = set_mask(0, 31)
	constexpr inline InterpConfig(uint32 c) noexcept : c(c) {}
	constexpr inline operator uint32() noexcept { return c; }

	constexpr inline InterpConfig set_mask(uint mask_lsb, uint mask_msb) noexcept
	{
		return (c & ~(SIO_INTERP0_CTRL_LANE0_MASK_LSB_BITS | SIO_INTERP0_CTRL_LANE0_MASK_MSB_BITS)) |
			   ((mask_lsb << SIO_INTERP0_CTRL_LANE0_MASK_LSB_LSB) & SIO_INTERP0_CTRL_LANE0_MASK_LSB_BITS) |
			   ((mask_msb << SIO_INTERP0_CTRL_LANE0_MASK_MSB_LSB) & SIO_INTERP0_CTRL_LANE0_MASK_MSB_BITS);
	}
	constexpr inline InterpConfig set_shift(uint shift) noexcept
	{
		return c | ((shift << SIO_INTERP0_CTRL_LANE0_SHIFT_LSB) & SIO_INTERP0_CTRL_LANE0_SHIFT_BITS);
	}
	constexpr inline InterpConfig set_cross_input() noexcept { return c | SIO_INTERP0_CTRL_LANE0_CROSS_INPUT_BITS; }
};


struct Interp : public interp_hw_t
{
	static constexpr uint ss_color = msbit(sizeof(Color));
	static constexpr uint lane0	   = 0;
	static constexpr uint lane1	   = 1;

	enum Mode {
		//     log2(element size)  + log2(array size)
		i1	 = (1 * ss_color) * 16 + 1,
		i2	 = (1 * ss_color) * 16 + 2,
		i4	 = (1 * ss_color) * 16 + 4,
		i8	 = (1 * ss_color) * 16 + 8,
		a1w8 = (2 * ss_color) * 16 + 2,
		// custom:
		i2u32 = 0x22, // 2 bit element size, 2 bit index => e.g. uint32 foo[4];
		i4u32 = 0x24, // 2 bit element size, 4 bit index => e.g. uint32 foo[16];
	};

	Interp() = delete;

	__force_inline uint32 pop_lane_result(uint lane) noexcept { return interp_hw_t::pop[lane]; }
	__force_inline void	  set_accumulator(uint lane, uint32 value) noexcept { accum[lane] = value; }

	__force_inline void setup(Mode mode) noexcept { setup(uint(mode) & 0x0f, uint(mode) >> 4); }

	__force_inline void setup(uint bpi, uint ss = ss_color) noexcept
	{
		// bpi = bits per index: 1, 2, 4 or 8
		// ss  = size shift for field elements

		// setup the interpolator for table look-up
		// to get the color from an indexed colormap or a color attribute:
		//		Color = table[byte & mask];
		//		byte >>= shift;

		base[lane0] = 0;							   // add nothing
		ctrl[lane0] = InterpConfig()				   //
						  .set_shift(bpi);			   // shift right by 1 .. 8 bit
		ctrl[lane1] = InterpConfig()				   //
						  .set_cross_input()		   // read from accu lane0
						  .set_mask(ss, ss + bpi - 1); // mask to select index bits
	}

	__force_inline void set_color_base(uint32 colors) noexcept { base[lane1] = colors; }
	__force_inline void set_color_base(const void* colors) noexcept { base[lane1] = uint32(colors); }
	__force_inline void set_pixels(uint32 value, uint ss = ss_color) noexcept { accum[lane0] = value << ss; }

	template<typename T = Color>
	const __force_inline T* next_color() noexcept
	{
		return reinterpret_cast<const T*>(interp_hw_t::pop[lane1]);
	}

	template<typename T>
	const __force_inline T peek(int lane) noexcept
	{
		uint32 z = interp_hw_t::peek[lane];
		return reinterpret_cast<T>(z);
	}
	template<typename T>
	const __force_inline T pop(int lane) noexcept
	{
		uint32 z = interp_hw_t::pop[lane];
		return reinterpret_cast<T>(z);
	}
};

static_assert(SIO_INTERP1_ACCUM0_OFFSET - SIO_INTERP0_ACCUM0_OFFSET == sizeof(Interp));


// clang-format off
// check definition unchanged:
#define interp_hw_array ((interp_hw_t *)(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET))
// clang-format on
// redefine it:
#undef interp_hw_array
#define interp_hw_array reinterpret_cast<Interp*>(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET)

using InterpMode = Interp::Mode;


} // namespace kilipili

/*





























*/
