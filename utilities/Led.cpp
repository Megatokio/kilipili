// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Led.h"
#include <hardware/timer.h>


namespace kio
{

#ifdef PICO_DEFAULT_LED_PIN

int sm_blink_onboard_led()
{
	static Led<PICO_DEFAULT_LED_PIN> onboard_led;
	static uint32_t timeout_us = time_us_32();
	
	if (int(time_us_32() - timeout_us) < 0) return 0;
	timeout_us += 500 * 1000;
	onboard_led.toggle();
	return 0;
}

#endif

} // namespace kio
