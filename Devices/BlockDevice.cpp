// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BlockDevice.h"
#include "kilipili_common.h"
#include <memory>


namespace kio::Devices
{

uint32 BlockDevice::ioctl(IoCtl cmd, void*, void*)
{
	// default implementation

	switch (cmd.cmd)
	{
	case IoCtl::CTRL_SYNC: return 0;
	case IoCtl::GET_SECTOR_SIZE: return 1u << ss_write;
	case IoCtl::GET_BLOCK_SIZE: return 1u << ss_erase;
	case IoCtl::CTRL_TRIM: return 0;

	case IoCtl::FLUSH_IN: return 0;
	case IoCtl::CTRL_RESET: return 0;
	case IoCtl::CTRL_CONNECT: return 0;
	case IoCtl::CTRL_DISCONNECT: return 0;
	default: throw INVALID_ARGUMENT;
	}
}

void BlockDevice::readPartialSector(LBA lba, char* z, SIZE offs, SIZE count)
{
	using Buffer = std::unique_ptr<char[]>;

	if (count == 0) return;
	const SIZE sector_size = 1 << ss_write;
	assert(offs + count <= sector_size);

	Buffer buffer {new char[sector_size]};
	readSectors(lba, buffer.get(), 1);
	memcpy(z, &buffer[offs], count);
}

void BlockDevice::writePartialSector(LBA lba, const char* q, SIZE offs, SIZE count)
{
	using Buffer = std::unique_ptr<char[]>;

	if (count == 0) return;
	const SIZE sector_size = 1 << ss_write;
	assert(offs + count <= sector_size);

	Buffer buffer {new char[sector_size]};
	readSectors(lba, buffer.get(), 1);
	memcpy(&buffer[offs], q, count);
	writeSectors(lba, buffer.get(), 1);
}


} // namespace kio::Devices
