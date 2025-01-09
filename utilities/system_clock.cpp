// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "system_clock.h"
#include "cdefs.h"
#include "utilities.h"
#include <cstdio>

// possible callback for the application:
__weak_symbol void sysclockChanged(uint32 new_clock) noexcept;

namespace kio::Audio
{
__weak_symbol void sysclockChanged(uint32 new_clock) noexcept;
}
namespace kio::LoadSensor
{
__weak_symbol void recalibrate() noexcept;
}


namespace kio
{
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

static_assert(calc_sysclock_params(15400 kHz).err > 0);
static_assert(calc_sysclock_params(15400 kHz).err < 30 kHz);
static_assert(calc_sysclock_params(15400 kHz).vco <= 12 MHz * 63);
static_assert(calc_sysclock_params(15400 kHz).div1 == 7);
static_assert(calc_sysclock_params(15400 kHz).div2 == 7);

static_assert(calc_sysclock_params(16 MHz).err < 100 kHz);
static_assert(calc_sysclock_params(16 MHz).vco == 16 MHz * 7 * 7 - 4 MHz);
static_assert(calc_sysclock_params(16 MHz).div1 == 7);
static_assert(calc_sysclock_params(16 MHz).div2 == 7);

static_assert(calc_sysclock_params(20 MHz).err == 0);
static_assert(calc_sysclock_params(20 MHz).vco == 20 MHz * 7 * 6);
static_assert(calc_sysclock_params(20 MHz).div1 == 7);
static_assert(calc_sysclock_params(20 MHz).div2 == 6);

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

static_assert(calc_sysclock_params(274 MHz).err > 0);
static_assert(calc_sysclock_params(274 MHz).err == 1 MHz);
static_assert(calc_sysclock_params(274 MHz).vco == 274 MHz * 4 - 4 MHz);
static_assert(calc_sysclock_params(274 MHz).div1 == 2);
static_assert(calc_sysclock_params(274 MHz).div2 == 2);

static_assert(calc_sysclock_params(275 MHz).err > 0);
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


Error set_system_clock(uint32 new_clock, uint32 max_error)
{
	uint32 old_clock = get_system_clock();
	if (new_clock == old_clock) return NO_ERROR;
	if (new_clock > SYSCLOCK_fMAX) return UNSUPPORTED_SYSTEM_CLOCK;

	auto params = calc_sysclock_params(new_clock);
	uint cv		= 85 + (params.voltage - VREG_VOLTAGE_0_85) * 5; // centivolt
	debugstr("set system clock = %u MHz and Vcore = %u.%02u V\n", new_clock / 1000000, cv / 100, cv % 100);

	if (params.err)
		debugstr(
			"new system clock = %u kHz, error = %i kHz (0.%03i%%)\n", params.vco / params.div1 / params.div2,
			(params.err + 500) / 1000, (params.err * 1000 + new_clock / 200) / (new_clock / 100));
	if (params.err > max_error) return UNSUPPORTED_SYSTEM_CLOCK;
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

	sysclock_changed(new_clock);

	return NO_ERROR;
}

void sysclock_changed(uint32 new_clock) noexcept
{
#if defined PICO_DEFAULT_UART_BAUD_RATE && defined PICO_DEFAULT_UART_INSTANCE
	// if the baudrate was changed at runtime then ::sysclockChanged() can fix it:
	uart_set_baudrate(PICO_DEFAULT_UART_INSTANCE(), PICO_DEFAULT_UART_BAUD_RATE);
#endif
	if (LoadSensor::recalibrate) LoadSensor::recalibrate();
	if (Audio::sysclockChanged) Audio::sysclockChanged(new_clock);
	if (::sysclockChanged) ::sysclockChanged(new_clock);
}

} // namespace kio

/*






 


















*/
