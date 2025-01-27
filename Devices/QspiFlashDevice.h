// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BlockDevice.h"


namespace kio::Devices
{

/*
	BlockDevice for the internal QSPI XIP Flash (program flash) on the Raspberry Pico / RP2040
	probably usable for any RP2040 board as long as the pico-sdk is used

	ATTN: writing to the program flash requires to stop whatever runs in or reads from flash!
	Callbacks suspend_system() and resume_system() should be implemented by the application!
	see common/pico/SuspendSystem.h.

	Note: Writing to the Qspi flash is unbuffered!
	TODO: use a sector buffer for writing.

	@template param ssw:
	This is the number of bits per sector used in readSector()/writeSector().
	  - 8 / 256 bytes: this is the actual write page size of the program flash.
	  - 9 / 512 bytes: this sector size is required by the FatFS FileSystem.
	  - 12/4096 bytes: this is the erase sector size of the program flash.
*/
template<int ssw>
class QspiFlashDevice : BlockDevice
{
public:
	// create BlockDevice at with 'size' at 'offset' in program flash.
	QspiFlashDevice(uint32 offset, uint32 size, Flags = readable | writable) throws;

	// create BlockDevice at with 'size' at end of program flash.
	// if FLASH_PREFERENCES is defined then the size is reduced to preserve the preferences.
	QspiFlashDevice(uint32 size, Flags = readable | writable) throws;

	// BlockDevice interface:
	virtual void   readSectors(LBA, void* data, SIZE count) throws override;
	virtual void   writeSectors(LBA, const void* data, SIZE count) throws override;
	virtual void   readData(ADDR, void* data, SIZE size) throws override;
	virtual void   writeData(ADDR, const void* data, SIZE size) throws override;
	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) throws override;

private:
	uint32 first_sector;
};


// 3 versions are implemented:
extern template class QspiFlashDevice<8>;
extern template class QspiFlashDevice<9>;
extern template class QspiFlashDevice<12>;

} // namespace kio::Devices
