// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "system_clock.h"
#include "common/cdefs.h"
#include "common/timing.h"
#include <cstdio>
#include <hardware/clocks.h>
#include <hardware/pll.h>
#include <hardware/vreg.h>
#include <pico/stdio.h>


// possible callback for the application:
__weak_symbol void sysclockChanged(uint32 new_clock) noexcept;

namespace kilipili
{
__weak_symbol void audioSysclockChanged(uint32 new_clock) noexcept;
__weak_symbol void devicesSysclockChanged(uint32 new_clock) noexcept;
} // namespace kilipili
namespace kilipili::LoadSensor
{
__weak_symbol void recalibrate() noexcept;
}


namespace kilipili
{
struct sysclock_params
{
	uint		 sysclock, vco, div1, div2, err;
	vreg_voltage voltage;
};

static __unused vreg_voltage vreg_get_voltage()
{
	uint v = (vreg_and_chip_reset_hw->vreg & VREG_AND_CHIP_RESET_VREG_VSEL_BITS) >> VREG_AND_CHIP_RESET_VREG_VSEL_LSB;
	return vreg_voltage(v);
}

static constexpr vreg_voltage calc_vreg_voltage_for_sysclock(uint32 f) noexcept
{
	f /= 1 MHz;
	return vreg_voltage(f >= 100 ? VREG_VOLTAGE_1_10 + (min(f, 220u) - 100) / 30 : VREG_VOLTAGE_0_85 + f / 20);
}

static constexpr sysclock_params calc_sysclock_params(uint32 f) noexcept
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
			if (new_f % 1000000) continue; // must be full MHz

			best.sysclock = new_f;
			best.div1	  = div1;
			best.div2	  = div2;
			best.vco	  = vco;
			best.err	  = err;
			if (err == 0) return best;
		}
	return best;
}

uint32 calc_sysclock_error(uint32 clock) noexcept
{
	return calc_sysclock_params(clock).err; //
}

void set_system_clock(uint32 new_clock) noexcept
{
	assert(new_clock <= VIDEO_MAX_SYSCLOCK_MHz MHz);

	uint32 old_clock = get_system_clock();
	if (new_clock == old_clock) return;

	auto params = calc_sysclock_params(new_clock);
	assert(params.err <= 1 MHz);							 // guaranteed up to 399 MHz
	uint cv = 85 + (params.voltage - VREG_VOLTAGE_0_85) * 5; // centivolt
	debugstr("set system clock = %u MHz and Vcore = %u.%02u V\n", new_clock / 1000000, cv / 100, cv % 100);

	if (params.err)
		debugstr(
			"new system clock = %u kHz, error = %u kHz (0.%03u%%)\n", params.vco / params.div1 / params.div2,
			(params.err + 500) / 1000, (params.err * 1000 + new_clock / 200) / (new_clock / 100));
	stdio_flush();

	if (new_clock < old_clock)
	{
		sleep_ms(5);
		set_sys_clock_pll(params.vco, params.div1, params.div2);
		sleep_ms(1);
	}

	vreg_set_voltage(params.voltage);

	if (new_clock > old_clock)
	{
		sleep_ms(5);
		set_sys_clock_pll(params.vco, params.div1, params.div2);
		sleep_ms(1);
	}

	new_clock = get_system_clock();
#if defined PICO_DEFAULT_UART_BAUD_RATE && defined PICO_DEFAULT_UART_INSTANCE
	// if the baudrate was changed at runtime then ::sysclockChanged() can fix it:
	uart_set_baudrate(PICO_DEFAULT_UART_INSTANCE(), PICO_DEFAULT_UART_BAUD_RATE);
#endif
	if (LoadSensor::recalibrate) LoadSensor::recalibrate();
	if (audioSysclockChanged) audioSysclockChanged(new_clock);
	if (devicesSysclockChanged) devicesSysclockChanged(new_clock);
	if (::sysclockChanged) ::sysclockChanged(new_clock);
}


static_assert(calc_vreg_voltage_for_sysclock(19 MHz) == VREG_VOLTAGE_0_85);
static_assert(calc_vreg_voltage_for_sysclock(20 MHz) == VREG_VOLTAGE_0_90);
static_assert(calc_vreg_voltage_for_sysclock(99 MHz) == VREG_VOLTAGE_1_05);
static_assert(calc_vreg_voltage_for_sysclock(100 MHz) == VREG_VOLTAGE_1_10);
static_assert(calc_vreg_voltage_for_sysclock(129 MHz) == VREG_VOLTAGE_1_10);
static_assert(calc_vreg_voltage_for_sysclock(130 MHz) == VREG_VOLTAGE_1_15);
static_assert(calc_vreg_voltage_for_sysclock(219 MHz) == VREG_VOLTAGE_1_25);
static_assert(calc_vreg_voltage_for_sysclock(220 MHz) == VREG_VOLTAGE_1_30);
static_assert(calc_vreg_voltage_for_sysclock(300 MHz) == VREG_VOLTAGE_1_30);

static_assert(calc_sysclock_params(10 MHz).err == 666 MHz);
static_assert(calc_sysclock_params(15300 kHz).err == 666 MHz);

static_assert(calc_sysclock_params(20 MHz).err == 0);
static_assert(calc_sysclock_params(20 MHz).vco == 20 MHz * 7 * 6);
static_assert(calc_sysclock_params(20 MHz).div1 == 7);
static_assert(calc_sysclock_params(20 MHz).div2 == 6);

static_assert(calc_sysclock_params(48 MHz).err == 0);
static_assert(calc_sysclock_params(48 MHz).vco == 48 MHz * 16);
static_assert(calc_sysclock_params(48 MHz).div1 == 4);
static_assert(calc_sysclock_params(48 MHz).div2 == 4);

static_assert(calc_sysclock_params(125 MHz).err == 0);
static_assert(calc_sysclock_params(125 MHz).vco == 125 MHz * 12);
static_assert(calc_sysclock_params(125 MHz).div1 == 4);
static_assert(calc_sysclock_params(125 MHz).div2 == 3);

static_assert(calc_sysclock_params(250 MHz).err == 0);
static_assert(calc_sysclock_params(250 MHz).vco == 250 MHz * 6);
static_assert(calc_sysclock_params(250 MHz).div1 == 3);
static_assert(calc_sysclock_params(250 MHz).div2 == 2);

static_assert(calc_sysclock_params(280 MHz).err == 0);
static_assert(calc_sysclock_params(280 MHz).vco == 280 MHz * 3);
static_assert(calc_sysclock_params(280 MHz).div1 == 3);
static_assert(calc_sysclock_params(280 MHz).div2 == 1);

static_assert(calc_sysclock_params(273 MHz).err == 0);
static_assert(calc_sysclock_params(273 MHz).vco == 273 MHz * 4);
static_assert(calc_sysclock_params(273 MHz).div1 == 2);
static_assert(calc_sysclock_params(273 MHz).div2 == 2);

static_assert(calc_sysclock_params(274 MHz).err == 1 MHz);
static_assert(calc_sysclock_params(274 MHz).vco == 274 MHz * 4 - 4 MHz);
static_assert(calc_sysclock_params(274 MHz).div1 == 2);
static_assert(calc_sysclock_params(274 MHz).div2 == 2);

static_assert(calc_sysclock_params(275 MHz).err == 1 MHz);
static_assert(calc_sysclock_params(275 MHz).vco == 275 MHz * 4 + 4 MHz);
static_assert(calc_sysclock_params(275 MHz).div1 == 2);
static_assert(calc_sysclock_params(275 MHz).div2 == 2);

static_assert(calc_sysclock_params(276 MHz).err == 0);
static_assert(calc_sysclock_params(276 MHz).vco == 276 MHz * 4);
static_assert(calc_sysclock_params(276 MHz).div1 == 2);
static_assert(calc_sysclock_params(276 MHz).div2 == 2);

static_assert(calc_sysclock_params(300 MHz).err == 0);
static_assert(calc_sysclock_params(300 MHz).vco == 300 MHz * 4);
static_assert(calc_sysclock_params(300 MHz).div1 == 2);
static_assert(calc_sysclock_params(300 MHz).div2 == 2);

static_assert(calc_sysclock_params(325 MHz).err == 1 MHz);
static_assert(calc_sysclock_params(325 MHz).vco == 325 MHz * 4 - 4 MHz);
static_assert(calc_sysclock_params(325 MHz).div1 == 2);
static_assert(calc_sysclock_params(325 MHz).div2 == 2);


} // namespace kilipili


/*






 


















*/
