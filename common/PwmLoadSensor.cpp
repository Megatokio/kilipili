// Copyright (c) 2022 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "PwmLoadSensor.h"
#include "utilities.h"
#include <hardware/pwm.h>
#include <pico/multicore.h>


namespace kipili
{

static constexpr uint pwm0 = PWM_LOAD_SENSOR_SLICE_NUM_BASE + 0;
static constexpr uint pwm1 = PWM_LOAD_SENSOR_SLICE_NUM_BASE + 1;

static constexpr uint pwm_max_count = 0xffff;

static constexpr uint timer_freq	  = 100; // frequency of measurement timer
static constexpr uint timer_period_us = uint((1000000 + timer_freq / 2) / timer_freq);


PwmLoadSensor loadsensor;


static uint map_range(uint value, uint qmax, uint zmax) { return min(zmax, (value * zmax + qmax / 2) / qmax); }

void PwmLoadSensor::CoreData::reset_load() noexcept
{
	do {
		count = 0;
		min	  = 0xffff;
		max	  = 0;
		sum	  = 0;
	}
	while (count != 0);
}

void PwmLoadSensor::CoreData::init(uint16 pwm) noexcept
{
	pwm_slice = pwm;
	pwm_set_wrap(pwm, 0xffff);
	pwm_set_clkdiv_mode(pwm, PWM_DIV_FREE_RUNNING);
	pwm_set_counter(pwm, last_pwm_count);
	reset_load();
}

void PwmLoadSensor::CoreData::update() noexcept
{
	// callback for measurement timer

	uint16 pwm_count = pwm_get_counter(pwm_slice) - last_pwm_count;
	last_pwm_count += pwm_count;

	if (pwm_count < min) min = pwm_count;
	if (pwm_count > max) max = pwm_count;
	sum	  = sum + pwm_count;
	count = count + 1;
}

void PwmLoadSensor::calibrate() noexcept
{
	// called for initialization and also
	// whenever the systerm clock is changed

	sys_clock = system_clock;

	float prediv = sys_clock / timer_freq / pwm_max_count + 1;
	prediv		 = prediv + prediv / 2; // some safety
	pwm_freq	 = float(sys_clock) / prediv;

	pwm_set_clkdiv(pwm0, prediv);
	pwm_set_clkdiv(pwm1, prediv);
}

void PwmLoadSensor::start() noexcept
{
	if (is_running()) return;

	calibrate();

	core[0].init(pwm0);
	core[1].init(pwm1);

	// start polling timer:

	alarm_id = add_alarm_in_us(
		timer_period_us,
		[](alarm_id_t, void*) -> int64 {
			loadsensor.core[0].update();
			loadsensor.core[1].update();
			return timer_period_us;
		},
		nullptr, false);
}

void PwmLoadSensor::stop() noexcept
{
	if (!is_running()) return;

	cancel_alarm(alarm_id);
	alarm_id = -1;
}

void PwmLoadSensor::getLoad(uint core_num, uint& min, uint& avg, uint& max) noexcept
{
	CoreData& core = this->core[core_num];

	uint max_pwm_count = uint(pwm_freq / timer_freq + 0.5f);
	uint sysclock	   = sys_clock / 100000; // 0.1 MHz

	for (;;)
	{
		uint count = core.count; // number of measurements since last reset
		max		   = sysclock - map_range(core.min, max_pwm_count, sysclock);
		min		   = sysclock - map_range(core.max, max_pwm_count, sysclock);
		avg		   = sysclock - map_range((core.sum + count / 2) / count, max_pwm_count, sysclock);
		if (core.count == count) break;
	}

	core.reset_load();
}

void PwmLoadSensor::printLoad(uint core)
{
	uint min, max, avg;
	loadsensor.getLoad(core, min, avg, max);
	uint sys = loadsensor.sys_clock / 100000;
	printf(
		"sys: %i.%iMHz, load#%i: %i.%i, %i.%i, %i.%iMHz (min,avg,max)\n", sys / 10, sys % 10, core, min / 10, min % 10,
		avg / 10, avg % 10, max / 10, max % 10);
}

} // namespace kipili
