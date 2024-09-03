// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#ifdef UNIT_TEST
  #include "glue.h"
#else
  
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
void getLoad(uint core, uint& min, uint& avg, uint& max) noexcept; // Hz

}; // namespace kio::LoadSensor


namespace kio
{

inline void idle_start() noexcept { pwm_set_enabled(PWM_LOAD_SENSOR_SLICE_NUM_BASE + get_core_num(), true); }
inline void idle_end() noexcept { pwm_set_enabled(PWM_LOAD_SENSOR_SLICE_NUM_BASE + get_core_num(), false); }

} // namespace kio
  
#endif
