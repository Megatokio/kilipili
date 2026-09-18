// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

// based on:
// Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
// SPDX-License-Identifier: BSD-3-Clause


#pragma once
#include "common/standard_types.h"
#include <hardware/pio.h>


namespace kilipili::Devices
{

// these functions are only implemented if SDCARD_SPI is #defined in the boards.h file

extern void sdcard_spi_read_blocking(uint8* data, size_t cnt) noexcept;
extern void sdcard_spi_write_blocking(const uint8* data, size_t cnt) noexcept;
extern void sdcard_spi_write_read_blocking(const uint8* src, uint8* dest, size_t cnt) noexcept;
extern void sdcard_spi_init() noexcept;

} // namespace kilipili::Devices
