// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "sm_utilities.h"
#include "LoadSensor.h"
#include "standard_types.h"
#include "utilities.h"
#include <cstdio>

namespace kio
{

int sm_blink_onboard_led() noexcept
{
#ifdef PICO_DEFAULT_LED_PIN
	BEGIN
	{
		static constexpr uint pin = PICO_DEFAULT_LED_PIN;
		gpio_init(pin);
		gpio_set_dir(pin, GPIO_OUT);

		static uint32_t timeout = time_us_32();

		for (;;)
		{
			gpio_xor_mask(1 << pin);
			SLEEP_MS(500);
		}
	}
	FINISH
#endif
}

int sm_throttle(int delay_usec) noexcept
{
	// throttle state machines if looping too fast.
	// if last call to `sm_delay()` is less than `delay_us` ago
	// then sleep until `delay_us` passed.

	static int32 next = int32(time_us_32());
	int32		 now  = int32(time_us_32());
	sleep_us(next - now);
	next = now + delay_usec;
	return 0;
}

static void print_load(uint core) noexcept
{
	uint min, max, avg;
	LoadSensor::getLoad(core, min, avg, max);
	min		 = (min + 50000) / 100000;
	max		 = (max + 50000) / 100000;
	avg		 = (avg + 50000) / 100000;
	uint sys = get_system_clock() / 100000;

	printf(
		"sys: %i.%iMHz, load#%i: %i.%i, %i.%i, %i.%iMHz (min,avg,max)\n", sys / 10, sys % 10, core, min / 10, min % 10,
		avg / 10, avg % 10, max / 10, max % 10);
}

int sm_print_load() noexcept
{
	static uint32 timeout = time_us_32();

	BEGIN
	{
		LoadSensor::start();

		for (;;)
		{
			SLEEP_MS(10000);

			print_load(0);
			print_load(1);
		}
	}
	FINISH
}

namespace Video
{
extern volatile uint32 scanlines_missed;
}

int sm_print_missed_lines() noexcept
{
	BEGIN
	{
		static uint32 timeout	= time_us_32();
		static uint32 old_count = Video::scanlines_missed;
		for (;;)
		{
			SLEEP_MS(10 * 1000);
			uint missed_lines = Video::scanlines_missed - old_count;
			if (missed_lines == 0) continue;
			old_count += missed_lines;
			printf("missed scanlines: %u (%u/frame)\n", missed_lines, uint(float(missed_lines) / 10 / 60 + 0.9f));
		}
	}
	FINISH
}


} // namespace kio
