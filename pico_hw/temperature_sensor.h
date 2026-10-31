// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <hardware/adc.h>
#include <pico/stdlib.h>

namespace kilipili
{

/*	Read the onboard temperature sensor.
	Result depends heavily on correct value of Vref!
	Actually, temperature readings are at most roughly accurate.

	init_temperature() must be called before first reading
	and also after the ADC was used otherwise.
*/

template<int unit = 'C'>
inline float read_temperature() noexcept
{
	// 12-bit conversion, assume max value == ADC_VREF == 3.3 V

	/* The temperature sensor measures the Vbe voltage of a biased bipolar diode,
	   connected to the fifth ADC channel (AINSEL=4).
		Typically, Vbe = 0.706V at 27 degrees C, with a slope of -1.721mV per degree.
		Therefore the temperature can be approximated as follows:

		temp = 27 - (ADC_voltage - 0.706) / 0.001721

		the formula can be simplified to 3 float calculations for both °C and °F:

		temp = 27 - (adc_voltage - 0.706) / 0.001721
		temp = 27 - (adc_read * 3.3 / 2^12 - 0.706) / 0.001721
		temp = 27 - (adc_read * 3.3 / 4096 - 0.706) / 0.001721
		temp = 27 - adc_read * 3.3 / 4096 / 0.001721 + 0.706 / 0.001721
		temp = (27 + 0.706 / 0.001721) - adc_read * (3.3 / 4096 / 0.001721)
		temp = 437.2266 - adc_read * 0.468137

		temF = 32 + temp * 1.8
		temF = 32 + (27 + 0.706 / 0.001721) * 1.8 - adc_read * (3.3 / 4096 / 0.001721) * 1.8
		temF = (32 + 27 + 0.706 / 0.001721 * 1.8) - adc_read * (3.3 / 4096 / 0.001721 * 1.8)
		temF = 797.4079 - adc_read * 0.842647
	*/

	if constexpr ((0))
	{
		float adc_voltage = float(adc_read()) * 3.3f / float(1 << 12);
		float temperature = 27.0f - (adc_voltage - 0.706f) / 0.001721f;

		if constexpr (unit == 'F') return 32.0f + temperature * 1.8f; // Farenheit
		else return temperature;									  // Celsius
	}
	else if constexpr (unit == 'F') // Farenheit
	{
		return 797.4079f - float(adc_read()) * 0.842647f;
	}
	else // Celsius
	{
		return 437.2266f - float(adc_read()) * 0.468137f;
	}
}

inline void init_temperature() noexcept
{
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);
}

} // namespace kilipili
