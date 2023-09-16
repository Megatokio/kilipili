// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "kipili_common.h"
#include <hardware/pwm.h>


#ifndef PWM_LOAD_SENSOR_SLICE_NUM_BASE
  #define PWM_LOAD_SENSOR_SLICE_NUM_BASE 6
#endif


namespace kipili
{

class PwmLoadSensor
{
	NO_COPY_MOVE(PwmLoadSensor);

public:
	PwmLoadSensor() noexcept {}
	~PwmLoadSensor() { stop(); }

	uint32	   sys_clock = 0; // measured in start()
	float	   pwm_freq;	  // calibrated in start()
	alarm_id_t alarm_id = -1;

	struct CoreData
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

	void start() noexcept;
	void stop() noexcept;
	void recalibrate() noexcept
	{
		if (is_running()) calibrate();
	}
	bool is_running() const noexcept { return alarm_id != -1; }

	void resetLoad() noexcept
	{
		core[0].reset_load();
		core[1].reset_load();
	}
	void getLoad(uint core, uint& min, uint& avg, uint& max) noexcept; // unit: [0.1MHz]
	void printLoad(uint core);										   // convenience

private:
	void calibrate() noexcept;
};


extern PwmLoadSensor loadsensor;

inline void idle_start() noexcept { pwm_set_enabled(PWM_LOAD_SENSOR_SLICE_NUM_BASE + get_core_num(), true); }

inline void idle_end() noexcept { pwm_set_enabled(PWM_LOAD_SENSOR_SLICE_NUM_BASE + get_core_num(), false); }

inline void wfe() noexcept
{
	idle_start();
	__asm volatile("wfe");
	idle_end();
}

inline void wfi() noexcept
{
	idle_start();
	__asm volatile("wfi");
	idle_end();
}

inline void sleepy_us(int32 usec) noexcept
{
	if (usec > 0)
	{
		idle_start();
		sleep_us(uint(usec));
		idle_end();
	}
}

} // namespace kipili
