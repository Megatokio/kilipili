// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
#include "devices_types.h"

namespace kio::Devices
{

/*
	Interface class `BlockDevice`
*/

class BlockDevice : public RCObject
{
public:
	NO_COPY_MOVE(BlockDevice);

	SIZE  sector_count;
	uint8 ss_read;	// log2 of physical read sector size, 0 = 1 byte
	uint8 ss_write; // log2 of physical write sector size, 0 = 1 byte
	uint8 ss_erase; // log2 of physical erase block size, 0 = not needed or not overwritable
	Flags flags;

	virtual ~BlockDevice() noexcept override = default;

	// addressable read/write
	// sector size = 1 << ss_write.
	// read/write all data or throw
	// call writeSectors(nullptr) to erase sectors

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) throws;
	virtual void   readSectors(LBA, void* data, SIZE count) throws;
	virtual void   writeSectors(LBA, const void* data, SIZE count) throws;
	virtual void   readData(ADDR address, void* data, SIZE size) throws;
	virtual void   writeData(ADDR address, const void* data, SIZE size) throws;

	// convenience:

	void sync() throws { ioctl(IoCtl::CTRL_SYNC); }
	SIZE sectorSize() const noexcept { return 1 << ss_write; }
	SIZE eraseBlockSize() const noexcept { return 1 << ss_erase; }
	LBA	 sectorCount() const noexcept { return sector_count; }
	ADDR totalSize() const noexcept;
	bool isReadable() const noexcept { return flags & readable; }
	bool isWritable() const noexcept { return flags & writable; }
	bool isOverwritable() const noexcept { return flags & overwritable; }

protected:
	BlockDevice(SIZE sectors, uint ss_read, uint ss_write, uint ss_erase, Flags f) noexcept :
		sector_count(sectors),
		ss_read(uint8(ss_read)),
		ss_write(uint8(ss_write)),
		ss_erase(uint8(ss_erase)),
		flags(f)
	{}
	void clamp(ADDR, SIZE) const throws;
	void clamp_blocks(LBA, SIZE) const throws;
};


using BlockDevicePtr = RCPtr<BlockDevice>;

inline void BlockDevice::readSectors(LBA, void*, SIZE) throws { throw NOT_READABLE; }
inline void BlockDevice::writeSectors(LBA, const void*, SIZE) throws { throw NOT_WRITABLE; }


} // namespace kio::Devices


/*






























*/
