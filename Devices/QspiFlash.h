// Copyright (c) 2021 - 2024 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "BlockDevice.h"


/*
	BlockDevice for the internal QSPI XIP Flash (program flash) on the Raspberry Pico / RP2040
	probably usable for any RP2040 board as long as the pico-sdk is used

	ATTN: writing to the program flash requires to stop whatever runs in or reads from flash!
	Callbacks suspend_system() and resume_system() should be replaced by the application!

	Note: Writing to the Qspi flash is unbuffered!
	It is recommended to use a BufferedBlockDevice for writing.
*/

namespace kio::Devices
{

// callbacks called by QspiFlash::write…() before and after writing to flash:
// may be replaced by application to stop and resume dma, interrupts, core1, ...
// the uint32 value can be cast to pointer type if required.
// default callbacks disable and restore interrupts on the calling core.
// dual core applications may use multicore_lockout_start_blocking() etc.

using SuspendSystemProc = uint32_t();
using ResumeSystemProc	= void(uint32_t);
extern SuspendSystemProc* suspend_system;
extern ResumeSystemProc*  resume_system;


class QspiFlash : public BlockDevice
{
public:
	// create BlockDevice at 'offset' with 'size' in XIP flash on Pico board.
	// if size=0 then up to end of flash.
	QspiFlash(uint32 offset = 512 * 1024, uint32 size = 0, Flags = readwrite);

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) throws override;
	virtual void   readSectors(LBA, void* data, SIZE count) throws override;
	virtual void   writeSectors(LBA, const void* data, SIZE count) throws override;
	virtual void   readData(ADDR, void* data, SIZE count) throws override;
	virtual void   writeData(ADDR, const void* data, SIZE count) throws override;

private:
	void write_sectors(uint32 addr, const uchar* data, uint32 count) throws;
	void write_bytes(uint32 addr, const uchar* data, uint32 count) throws;
	void erase_sectors(uint32 addr, uint32 count) throws;

	uint32 start;		  // offset in Flash for writing
	uptr   start_cached;  // start address in memory for reading
	uptr   start_nocache; // start address in memory for reading
};

} // namespace kio::Devices
