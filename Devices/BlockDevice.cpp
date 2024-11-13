// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BlockDevice.h"
#include <memory>
#include <string.h>


namespace kio::Devices
{

void BlockDevice::clamp_blocks(LBA block, SIZE count) const throws
{
	if (block > sector_count || count > sector_count - block) throw END_OF_FILE;
}

void BlockDevice::clamp(ADDR pos, SIZE size) const throws
{
	if (pos > totalSize() || size > totalSize() - pos) throw END_OF_FILE;
}

uint32 BlockDevice::ioctl(IoCtl cmd, void*, void*)
{
	// default implementation

	switch (cmd.cmd)
	{
	case IoCtl::CTRL_SYNC: return 0;
	case IoCtl::GET_SECTOR_SIZE: return 1u << ss_write;
	case IoCtl::GET_BLOCK_SIZE: return 1u << ss_erase;
	case IoCtl::GET_SECTOR_COUNT: return sector_count;
	case IoCtl::CTRL_TRIM: return 0;

	case IoCtl::FLUSH_IN: return 0;
	case IoCtl::CTRL_RESET: return 0;
	case IoCtl::CTRL_CONNECT: return 0;
	case IoCtl::CTRL_DISCONNECT: return 0;
	default: throw INVALID_ARGUMENT;
	}
}

void BlockDevice::readData(ADDR address, void* _data, SIZE count) throws // bad_alloc
{
	ptr	 data		 = reinterpret_cast<ptr>(_data);
	SIZE sector_size = 1 << ss_write;
	LBA	 block		 = LBA(address >> ss_write);

	if (SIZE offset = address & (sector_size - 1))
	{
		std::unique_ptr<char[]> buffer {new char[sector_size]};

		SIZE n = std::min(count, sector_size - offset);

		readSectors(block, buffer.get(), 1);
		memcpy(data, &buffer[offset], n);

		count -= n;
		data += n;
		block += 1;
	}

	if (SIZE n = count >> ss_write)
	{
		readSectors(block, data, n);
		block += n;
		count -= n << ss_write;
		data += n << ss_write;
	}

	if (count)
	{
		std::unique_ptr<char[]> buffer {new char[sector_size]};
		readSectors(block, buffer.get(), 1);
		memcpy(data, &buffer[0], count);
	}
}

void BlockDevice::writeData(ADDR address, const void* _data, SIZE count) throws // bad_alloc
{
	cptr data		 = reinterpret_cast<cptr>(_data);
	SIZE sector_size = 1 << ss_write;
	LBA	 block		 = LBA(address >> ss_write);

	if (SIZE offset = address & (sector_size - 1))
	{
		std::unique_ptr<char[]> buffer {new char[sector_size]};

		SIZE n = std::min(count, sector_size - offset);

		readSectors(block, buffer.get(), 1);
		memcpy(&buffer[offset], data, n);
		writeSectors(block, buffer.get(), 1);

		count -= n;
		data += n;
		block += 1;
	}

	if (SIZE n = count >> ss_write)
	{
		writeSectors(block, data, n);
		block += n;
		count -= n << ss_write;
		data += n << ss_write;
	}

	if (count)
	{
		std::unique_ptr<char[]> buffer {new char[sector_size]};

		readSectors(block, buffer.get(), 1);
		memcpy(&buffer[0], data, count);
		writeSectors(block, buffer.get(), 1);
	}
}

ADDR BlockDevice::totalSize() const noexcept
{
	if constexpr (sizeof(ADDR) == sizeof(uint64)) return ADDR(sector_count) << ss_write;
	ADDR size = sector_count << ss_write;
	if (size >> ss_write == sector_count) return size;
	debugstr("BlockDevice: size exceeds 4GB\n");
	return 0xffffffffu << ss_write;
}

} // namespace kio::Devices

/*



































*/
