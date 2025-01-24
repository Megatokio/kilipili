// Copyright (c) 2021 - 2025 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "cdefs.h"
#include "standard_types.h"


// you may define this symbol to get a disk activity light:
extern __weak_symbol void set_disk_light(bool onoff);


namespace kio
{
// Wrappers which disable interrupts.
// The other core must already been locked out.
// eventually you only need these:
extern void flash_erase(uint32 offs, uint32 size) noexcept;
extern void flash_program(uint32 offs, cuptr bu, uint32 size) noexcept;

} // namespace kio


namespace kio::Devices
{
/*
	BlockDevice for the internal program flash on the Raspberry Pico / RP2040.
	probably usable for any RP2040 board as long as the pico-sdk is used.

	ATTN: writing to the program flash requires to stop whatever runs in or reads from flash!
	Callbacks suspend_system() and resume_system() should be implemented by the application!
	see common/pico/SuspendSystem.h.

	Note: Writing to the QspiFlash is unbuffered!
	It is recommended to do some buffering for writing.
*/
class QspiFlash
{
public:
	static void readData(uint32 offs, void* data, uint32 size) noexcept;
	static void writeData(uint32 offs, const void* data, uint32 size) throws;
	static void eraseData(uint32 offs, uint32 size) throws;

	static constexpr int ssw = 8;
	static constexpr int sse = 12;
	static cuptr		 flash_base() noexcept;
	static uint32		 flash_size() noexcept;
	static uint32		 flash_binary_size() noexcept;

#if defined UNIT_TEST && UNIT_TEST
	QspiFlash(uint8* flash, uint32 size) noexcept;
#endif
};
} // namespace kio::Devices
