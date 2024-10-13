// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "LoadSensor.h"
#include "basic_math.h"
#include "cdefs.h"
#include "system_clock.h"
#include <stdio.h>

namespace kio::LoadSensor
{

static constexpr uint pwm0 = PWM_LOAD_SENSOR_SLICE_NUM_BASE + 0;
static constexpr uint pwm1 = PWM_LOAD_SENSOR_SLICE_NUM_BASE + 1;

static constexpr uint pwm_max_count = 0xffff;

static constexpr uint timer_frequency = 100; // frequency of measurement timer
static constexpr uint timer_period_us = uint((1000000 + timer_frequency / 2) / timer_frequency);


static alarm_id_t alarm_id = -1;
static float	  pwm_frequency; // calibrated in start()


static struct CoreData
{
	uint16 pwm_slice;
	uint16 last_pwm_count = 0;

	volatile uint	count = 0;		// measurement counter
	volatile uint16 min	  = 0xffff; // min count seen
	volatile uint16 max	  = 0;		// max count seen
	volatile uint32 sum	  = 0;		// sum of all counts for avg calculation

	void reset_load() noexcept;
	void update() noexcept; // callback for measurement timer
	void init(uint16 pwm) noexcept;
} core[2];

void CoreData::reset_load() noexcept
{
	do {
		count = 0;
		min	  = 0xffff;
		max	  = 0;
		sum	  = 0;
	}
	while (count != 0);
}

void CoreData::init(uint16 pwm) noexcept
{
	pwm_slice = pwm;
	pwm_set_wrap(pwm, 0xffff);
	pwm_set_clkdiv_mode(pwm, PWM_DIV_FREE_RUNNING);
	pwm_set_counter(pwm, last_pwm_count);
	reset_load();
}

void CoreData::update() noexcept
{
	// callback for measurement timer

	uint16 pwm_count = pwm_get_counter(pwm_slice) - last_pwm_count;
	last_pwm_count += pwm_count;

	if (pwm_count < min) min = pwm_count;
	if (pwm_count > max) max = pwm_count;
	sum	  = sum + pwm_count;
	count = count + 1;
}

static void calibrate() noexcept;

bool is_running() noexcept
{
	return alarm_id != -1; //
}

void recalibrate() noexcept
{
	if (is_running()) calibrate();
}


void calibrate() noexcept
{
	// called for initialization and also
	// whenever the systerm clock is changed

	float prediv  = get_system_clock() / timer_frequency / pwm_max_count + 1;
	prediv		  = prediv + prediv / 2; // some safety
	pwm_frequency = float(get_system_clock()) / prediv;

	pwm_set_clkdiv(pwm0, prediv);
	pwm_set_clkdiv(pwm1, prediv);

	uint f = uint(pwm_frequency + 500) / 1000;
	debugstr("LoadSensor pwm_frequency = %u.%03u MHz\n", f / 1000, f % 1000);
}

void start() noexcept
{
	if (is_running()) return;

	calibrate();

	core[0].init(pwm0);
	core[1].init(pwm1);

	// start polling timer:

	alarm_id = add_alarm_in_us(
		timer_period_us,
		[](alarm_id_t, void*) -> int64 {
			core[0].update();
			core[1].update();
			return timer_period_us;
		},
		nullptr, false);
}

void stop() noexcept
{
	if (!is_running()) return;

	cancel_alarm(alarm_id);
	alarm_id = -1;
}

void getLoad(uint core_num, uint& min, uint& avg, uint& max) noexcept
{
	CoreData& my_core = core[core_num];
	uint	  count, core_min, core_max, core_sum;
	do {
		count	 = my_core.count;
		core_min = my_core.min;
		core_max = my_core.max;
		core_sum = my_core.sum;
	}
	while (count != my_core.count);
	my_core.reset_load();

	uint max_pwm_count = uint(pwm_frequency / timer_frequency + 0.5f);
	uint sysclock	   = get_system_clock();

	max = sysclock - map_range(core_min, max_pwm_count, sysclock);
	min = sysclock - map_range(core_max, max_pwm_count, sysclock);
	avg = sysclock - map_range((core_sum + count / 2) / count, max_pwm_count, sysclock);

	if unlikely (min > avg) min = avg; // after set_system_clock(): core.max > max_pwm_count
}

} // namespace kio::LoadSensor


/* 









































*/
