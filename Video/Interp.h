// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "basic_math.h"
#include "standard_types.h"
#include <hardware/interp.h>

namespace kio::Video
{

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
		return (c & ~SIO_INTERP0_CTRL_LANE0_SHIFT_BITS) |
			   ((shift << SIO_INTERP0_CTRL_LANE0_SHIFT_LSB) & SIO_INTERP0_CTRL_LANE0_SHIFT_BITS);
	}
	constexpr inline InterpConfig set_cross_input(bool cross_input = true) noexcept
	{
		return (c & ~SIO_INTERP0_CTRL_LANE0_CROSS_INPUT_BITS) |
			   (cross_input ? SIO_INTERP0_CTRL_LANE0_CROSS_INPUT_BITS : 0);
	}
};


struct Interp : public interp_hw_t
{
	static constexpr uint ss_color = msbit(sizeof(Graphics::Color));
	static constexpr uint lane0	   = 0;
	static constexpr uint lane1	   = 1;

	Interp() = delete;

	__force_inline uint32 pop_lane_result(uint lane) noexcept { return pop[lane]; }
	__force_inline void	  set_accumulator(uint lane, uint32 value) noexcept { accum[lane] = value; }

	__force_inline void setup(uint bpi, uint ss = ss_color) noexcept
	{
		// bpi = bits per index: 1, 2, 4 or 8
		// ss  = size shift for field elements

		// setup the interpolator for table look-up
		// to get the color from an indexed colormap or a color attribute:
		//		Color = table[byte & mask];
		//		byte >>= shift;

		ctrl[lane0] = InterpConfig()				   //
						  .set_shift(bpi);			   // shift right by 1 .. 8 bit
		ctrl[lane1] = InterpConfig()				   //
						  .set_cross_input()		   // read from accu lane0
						  .set_mask(ss, ss + bpi - 1); // mask to select index bits
	}

	__force_inline void set_color_base(uint32 colors) noexcept { base[lane1] = colors; }
	__force_inline void set_color_base(const void* colors) noexcept { base[lane1] = uint32(colors); }
	__force_inline void set_pixels(uint32 value, uint ss = ss_color) noexcept { accum[lane0] = value << ss; }

	template<typename T = Graphics::Color>
	const __force_inline T* next_color() noexcept
	{
		return reinterpret_cast<const T*>(pop[lane1]);
	}
};


} // namespace kio::Video
