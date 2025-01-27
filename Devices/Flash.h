// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
#include "standard_types.h"

// you may define this symbol to get a disk activity light:
extern __weak_symbol void set_disk_light(bool onoff);

// these are normally defined in Video/VideoController:
__weak_symbol void suspend_core1() noexcept;
__weak_symbol void resume_core1() noexcept;


namespace kio::Flash
{

/*
	read and write from the internal program flash on the Raspberry Pico / RP2040.
	probably usable for any RP2040 board as long as the pico-sdk is used.

	ATTN: Writing to the program flash requires to stop whatever runs in or reads from flash!
	  When data is written to flash `__weak suspend_core1()` and `resume_core1()` are called.
	  The default weak implementations are in Devices/VideoController.
	  They may be reimplemented by the application if core1 is not used for video.
*/

// Suspend core1, Disable interrupts and write to flash.
// offs and size must be a multiple of esize or wsize resp.
extern void flash_erase(uint32 offs, uint32 size) noexcept;
extern void flash_program(uint32 offs, cuptr bu, uint32 size) noexcept;

// Suspend core1, Disable interrupts and write to flash.
// offs and size may be any value.
extern void readData(uint32 offs, void* data, uint32 size) noexcept;
extern void writeData(uint32 offs, const void* data, uint32 size) throws;
extern void eraseData(uint32 offs, uint32 size) throws;

extern cuptr  flash_base() noexcept;
extern uint32 flash_size() noexcept;
extern uint32 binary_size() noexcept;

// these consts are checked in Flash.cpp:
static constexpr int  ssw	= 8;
static constexpr int  sse	= 12;
static constexpr uint esize = 1 << sse;
static constexpr uint wsize = 1 << ssw;

#if defined UNIT_TEST && UNIT_TEST
extern void setupMockFlash(uint8* flash, uint32 size) noexcept;
#endif


} // namespace kio::Flash
