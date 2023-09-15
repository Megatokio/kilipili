// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Led.h"
#include "standard_types.h"
#include <hardware/timer.h>


namespace kipili
{

#ifdef PICO_DEFAULT_LED_PIN

// clang-format off

#define BEGIN		 static uint state=0; switch(state) { default:
#define WAIT()		 do { state = __LINE__; return 0; case __LINE__:; } while(0)
#define FINISH		 return -1; }
#define SLEEP_US(us) for(timeout += us; int(timeout-time_us_32()) > 0;) WAIT() // drift-free

// clang-format on

int sm_blink_onboard_led()
{
	BEGIN
	{
		static uint32					 timeout = time_us_32();
		static Led<PICO_DEFAULT_LED_PIN> onboard_led;
		for (;;)
		{
			onboard_led.toggle();
			SLEEP_US(500 * 1000);
		}
	}
	FINISH
}

#endif

} // namespace kipili
