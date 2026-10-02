// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Graphics/Color.h"
#include "common/basic_math.h"
#include "common/standard_types.h"
#include <hardware/interp.h>


#ifndef VIDEO_INTERP0_MODE
  #define VIDEO_INTERP0_MODE any
#endif

#ifndef VIDEO_INTERP1_MODE
  #define VIDEO_INTERP1_MODE a1w8
#endif


namespace kilipili::Video
{

/*	Interpolator setup for the ScanlineRenderers:

	Tweakable parts:

	VIDEO_INTERP0_MODE: configure pixel size for interp0: -1=any, 0…3 -> 1,2,4,8 bit, 5=a1w8 (default)
	VIDEO_INTERP1_MODE: configure pixel size for interp1: -1=any (default), 0…3 -> 1,2,4,8 bit, 5=a1w8
	  -	these options define the default setting for interp0 and interp1 on core1.
	  - an interpolator may be set for a specific color mode or for use by any color mode.
	  - if an interpolator is set for a specific color mode then it is setup at startup by the VideoController.
	  - otherwise the ScanlineRenderer must set it up at the start of each scanline.
	  - if it uses an interpolator which is reserved for another color mode (because no interpolator is set to 'any')
		then the ScanlineRenderer must also restore it to the reserved mode at the end of the scanline.


	Interpolator settings needed for ScanlineRenderers for various color modes:
	(may change. please check source)

		i1:   a1w1 a1w2 a1w4 a1w8 
		i2:   a2w1 a2w2 a2w4 a2w8
		i4:   i4
		i8:   i8 ham
		a1w8: a1w8
		none: i1 i2 rgb


	Own scanline render functions using an Interpolator:

	If an application defines a new scanline render function which uses an interpolator, it must take care
	to avoid conflicts with the existing ScanlineRenderers it uses:
	  - either fully setup the interpolator at the start of each scanline and restore it at the end
		to the extend needed (whether it is set to 'any' or a specific mode, see helper templates below)
	  -	or reserve interp1 (not interp0) for your renderer exclusively with VIDEO_INTERP1_MODE=99 or similar.
*/


// one-time initialization:
// called by VideoController
extern void initializeInterpolators() noexcept;


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

	enum Mode {
		any = -1,
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

	__force_inline uint32 pop_lane_result(uint lane) noexcept { return pop[lane]; }
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

static_assert(SIO_INTERP1_ACCUM0_OFFSET - SIO_INTERP0_ACCUM0_OFFSET == sizeof(Interp));


// clang-format off
// check definition unchanged:
#define interp_hw_array ((interp_hw_t *)(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET))
// clang-format on
// redefine it:
#undef interp_hw_array
#define interp_hw_array reinterpret_cast<Interp*>(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET)


using InterpMode				 = Interp::Mode;
constexpr InterpMode ip0_mode	 = InterpMode::VIDEO_INTERP0_MODE;
constexpr InterpMode ip1_mode	 = InterpMode::VIDEO_INTERP1_MODE;
constexpr InterpMode ip_modes[2] = {ip0_mode, ip1_mode};


template<InterpMode mode>
constexpr int ipi = ip1_mode == mode || (ip0_mode != mode && ip1_mode == InterpMode::any);

template<InterpMode mode>
constexpr bool need_setup = (ip_modes[ipi<mode>]) != mode;

template<InterpMode mode>
constexpr bool need_cleanup = need_setup<mode> && ip_modes[ipi<mode>] != InterpMode::any;

template<InterpMode mode>
static __force_inline void setup_if_needed() noexcept
{
	if constexpr (need_setup<mode>) interp0[ipi<mode>].setup(mode);
}

template<InterpMode mode>
static __force_inline void cleanup_if_needed() noexcept
{
	if constexpr (need_cleanup<mode>) interp0[ipi<mode>].setup(ip_modes[ipi<mode>]);
}


} // namespace kilipili::Video

/*





























*/
