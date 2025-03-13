// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "basic_math.h"
#include "common/standard_types.h"
#include <hardware/clocks.h>
#include <hardware/pll.h>
#include <hardware/vreg.h>


#ifndef SYSCLOCK_fMAX
  #define SYSCLOCK_fMAX (290 MHz)
#endif


constexpr Error UNSUPPORTED_SYSTEM_CLOCK = "requested system clock is not supported";


namespace kio
{

inline uint32 get_system_clock() { return clock_get_hz(clk_sys); }
extern Error  set_system_clock(uint32 sys_clock = 125 MHz, uint32 max_error = 1 MHz);
extern void	  sysclock_changed(uint32 new_clock) noexcept;

struct sysclock_params
{
	uint		 sysclock, vco, div1, div2, err;
	vreg_voltage voltage;
};
extern constexpr sysclock_params calc_sysclock_params(uint32 f, bool full_mhz = true) noexcept;
extern constexpr vreg_voltage	 calc_vreg_voltage_for_sysclock(uint32 f) noexcept;
inline vreg_voltage				 vreg_get_voltage();


//
// **************** Implementations *************************
//


inline vreg_voltage vreg_get_voltage()
{
	uint v = (vreg_and_chip_reset_hw->vreg & VREG_AND_CHIP_RESET_VREG_VSEL_BITS) >> VREG_AND_CHIP_RESET_VREG_VSEL_LSB;
	return vreg_voltage(v);
}

constexpr vreg_voltage calc_vreg_voltage_for_sysclock(uint32 f) noexcept
{
	f /= 1 MHz;
	return vreg_voltage(f >= 100 ? VREG_VOLTAGE_1_10 + (min(f, 220u) - 100) / 30 : VREG_VOLTAGE_0_85 + f / 20);
}


constexpr sysclock_params calc_sysclock_params(uint32 f, bool full_mhz) noexcept
{
	// the system clock is derived from the crystal by scaling up for the vco and two 3-bit dividers,
	// thus system_clock = crystal * vco_cnt / div1 / div2
	// with vco_cnt = 16 .. 320
	// limited by min_vco and max_vco frequency 750 MHz and 1600 MHz resp.
	// => vco_cnt = 63 .. 133
	//      div1 = 1 .. 7
	//      div2 = 1 .. 7
	// the crystal on the Pico board is 12 MHz
	// lowest  possible sys_clock = 12*63/7/7 ~ 15.428 MHz
	// highest possible sys_clock = 12*133/1/1 ~ 1596 MHz
	//
	// possible full MHz clocks:
	//  63 .. 133 MHz	in 1 MHz steps	(f_out = 12 MHz * vco / 12)
	// 126 .. 266 MHz	in 2 MHz steps  (f_out = 12 MHz * vco / 6)
	// 189 .. 399 MHz	in 3 MHz steps	(f_out = 12 MHz * vco / 4)
	// 252 .. 532 MHz   in 4 MHz steps  (f_out = 12 MHz * vco / 3)

	// 275 MHz is not possible ( 275 > 266 && 275%3 != 0 && 275%4 != 0 )
	// 280 MHz ok              ( 280 > 266 && 280%4 == 0 => vco = 280/4 && div by 12/4 )
	// 300 MHz freezes

	constexpr uint	xtal = XOSC_KHZ / PLL_COMMON_REFDIV * 1000; // 12 MHz
	sysclock_params best {666 MHz, 0, 0, 0, 666 MHz, calc_vreg_voltage_for_sysclock(f)};

#ifndef PICO_PLL_VCO_MIN_FREQ_HZ // SDK 1.5.1
  #define PICO_PLL_VCO_MIN_FREQ_HZ (PICO_PLL_VCO_MIN_FREQ_KHZ * 1000)
  #define PICO_PLL_VCO_MAX_FREQ_HZ (PICO_PLL_VCO_MAX_FREQ_KHZ * 1000)
#endif

	uint div_min = (PICO_PLL_VCO_MIN_FREQ_HZ + f - 1) / f; // round up
	uint div_max = (PICO_PLL_VCO_MAX_FREQ_HZ) / f;		   // round down

	for (uint div1 = 2; div1 <= 7; div1++)
		for (uint div2 = 1; div2 <= div1; div2++)
		{
			uint div = div1 * div2;
			if (div < div_min) continue;
			if (div > div_max) break;

			uint vco   = (f * div + xtal / 2) / xtal * xtal; // ~ 1 GHz
			uint new_f = vco / div;
			uint err   = uint(abs(int(f) - int(new_f)));

			if (err >= best.err) continue;
			if (full_mhz && new_f % 1000000) continue;

			best.sysclock = new_f;
			best.div1	  = div1;
			best.div2	  = div2;
			best.vco	  = vco;
			best.err	  = err;
			if (err == 0) return best;
		}
	return best;
}

} // namespace kio
