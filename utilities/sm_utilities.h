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


} // namespace kio
