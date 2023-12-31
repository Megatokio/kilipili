// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "devices_types.h"
#include "cdefs.h"

namespace kio::Devices
{

/* Interface class `BlockDevice`
*/
class BlockDevice : public RCObject
{
public:
	uint8 ss_read;	// log2 of physical read sector size, 0 = 1 byte
	uint8 ss_write; // log2 of physical write sector size, 0 = 1 byte
	uint8 ss_erase; // log2 of physical erase block size, 0 = not needed or not overwritable
	Flags flags;

	BlockDevice(uint ss_read, uint ss_write, uint ss_erase, Flags f = readwrite) noexcept :
		ss_read(uint8(ss_read)),
		ss_write(uint8(ss_write)),
		ss_erase(uint8(ss_erase)),
		flags(f)
	{}
	virtual ~BlockDevice() noexcept override   = default;
	BlockDevice(const BlockDevice&) noexcept   = default;
	BlockDevice(BlockDevice&&) noexcept		   = default;
	BlockDevice& operator=(const BlockDevice&) = delete;
	BlockDevice& operator=(BlockDevice&&)	   = delete;

	// addressable read/write
	// sector size = 1 << ss_write.
	// read/write all data or throw
	// call writeSectors(nullptr) to format sectors

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) throws;
	virtual void   readSectors(LBA, char* data, SIZE count) throws;
	virtual void   writeSectors(LBA, const char* data, SIZE count) throws;
	virtual void   readPartialSector(LBA, char* data, SIZE offs, SIZE count) throws;
	virtual void   writePartialSector(LBA, const char* data, SIZE offs, SIZE count) throws;

	// convenience:

	void sync() throws { ioctl(IoCtl::CTRL_SYNC); }
	SIZE getSectorSize() const noexcept { return 1 << ss_write; }
	SIZE getEraseBlockSize() const noexcept { return 1 << ss_erase; }
	LBA	 getSectorCount() { return ioctl(IoCtl::GET_SECTOR_COUNT); }
	ADDR getSize() { return ADDR(getSectorCount()) << ss_write; }
	bool isReadable() const noexcept { return flags & readable; }
	bool isWritable() const noexcept { return flags & writable; }
	bool isOverwritable() const noexcept { return flags & overwritable; }
};


using BlockDevicePtr = RCPtr<BlockDevice>;

inline void BlockDevice::readSectors(LBA, char*, SIZE) { throw NOT_READABLE; }
inline void BlockDevice::writeSectors(LBA, const char*, SIZE) { throw NOT_WRITABLE; }


} // namespace kio::Devices


/*






























*/
