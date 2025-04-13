// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#ifdef LIB_PICO_STDLIB

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

// the PWM runs while the CPU is idle:

inline auto& pwm() noexcept { return pwm_hw->slice[PWM_LOAD_SENSOR_SLICE_NUM_BASE + get_core_num()]; }

inline bool is_idle() noexcept { return (pwm().csr >> PWM_CH0_CSR_EN_LSB) & 1; }

inline void idle_start() noexcept
{
	io_rw_32* csr = &pwm().csr;
	hw_set_bits(csr, PWM_CH0_CSR_EN_BITS);
}

inline void idle_end() noexcept
{
	io_rw_32* csr = &pwm().csr;
	hw_clear_bits(csr, PWM_CH0_CSR_EN_BITS);
}

// an ISR may interrupt the CPU while it is idle or while it is busy.
// in any case we want the ISR time to be accounted as 'busy'.

inline uint32_t isr_start() noexcept
{
	io_rw_32* csr = &pwm().csr;
	uint32_t  f	  = *csr & PWM_CH0_CSR_EN_BITS;
	hw_clear_bits(csr, f);
	return f;
}
inline void isr_end(uint32_t old_idle_state) noexcept
{
	io_rw_32* csr = &pwm().csr;
	hw_set_bits(csr, old_idle_state);
}

}; // namespace kio::LoadSensor


namespace kio
{

inline void idle_start() noexcept { LoadSensor::idle_start(); }
inline void idle_end() noexcept { LoadSensor::idle_end(); }

} // namespace kio

#else

namespace kio
{

inline void idle_start() noexcept {}
inline void idle_end() noexcept {}

} // namespace kio

#endif

/*




































*/
