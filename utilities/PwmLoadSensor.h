// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <hardware/pwm.h>
#include <pico/stdlib.h>


#ifndef PWM_LOAD_SENSOR_SLICE_NUM_BASE
  #define PWM_LOAD_SENSOR_SLICE_NUM_BASE 6
#endif


namespace kio::LoadSensor
{

void start() noexcept;
void stop() noexcept;
void recalibrate() noexcept;
void getLoad(uint core, uint& min, uint& avg, uint& max) noexcept; // unit: [0.1MHz]
void printLoad(uint core);										   // convenience

extern float pwm_frequency;

}; // namespace kio::LoadSensor


namespace kio
{

inline void idle_start() noexcept;
inline void idle_end() noexcept;
inline void wfe() noexcept;
inline void wfi() noexcept;
void		sleepy_us(int usec) noexcept;


//
//
// **************** Implementations *************************
//
//

inline void idle_start() noexcept
{
	pwm_set_enabled(PWM_LOAD_SENSOR_SLICE_NUM_BASE + get_core_num(), true); //
}

inline void idle_end() noexcept
{
	pwm_set_enabled(PWM_LOAD_SENSOR_SLICE_NUM_BASE + get_core_num(), false); //
}

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

} // namespace kio
