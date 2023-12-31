// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

namespace kio
{
extern int sm_throttle(int delay_usec = 1000) noexcept;
extern int sm_blink_onboard_led() noexcept;
extern int sm_print_load() noexcept;
extern int sm_print_missed_lines() noexcept;


// ==================================================
//	State Machine Macros
// ==================================================

#define BEGIN             \
  static uint _state = 0; \
  switch (_state)         \
  {                       \
  default:
#define FINISH \
  return -1;   \
  }
#define WAIT()         \
  do {                 \
	_state = __LINE__; \
	return 0;          \
  case __LINE__:;      \
  }                    \
  while (0)

// drift-free definition: needs static var `timeout` initialized to current time:
// static uint32 timeout = time_us_32();
#define SLEEP_US(usec) \
  for (timeout += usec; int(timeout - time_us_32()) > 0;) WAIT()
#define SLEEP_MS(msec) SLEEP_US(msec * 1000)

// drifting definition: needs static var `timeout`:
// static uint32 timeout;
#define SLEEPY_US(usec) \
  for (timeout = time_us_32() + usec; int(timeout - time_us_32()) > 0;) WAIT()
#define SLEEPY_MS(msec) SLEEPY_US(msec * 1000)

} // namespace kio
