// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "QspiFlashDevice.h"
#include "Flash.h"


namespace kio::Devices
{

#if defined FLASH_PREFERENCES && FLASH_PREFERENCES
static constexpr uint prefs_size = FLASH_PREFERENCES;
#else
static constexpr uint prefs_size = 0;
#endif


template<int ssw>
QspiFlashDevice<ssw>::QspiFlashDevice(uint32 addr, uint32 size, Flags flags) throws :
	BlockDevice(size >> ssw, 0, ssw, Flash::sse, flags),
	first_sector(addr >> ssw)
{
	assert(addr % (1 << Flash::sse) == 0); // must be aligned to erase sector size
	assert(size % (1 << Flash::sse) == 0); // must be aligned to erase sector size

	if (addr < Flash::binary_size()) throw("program overwrote start of flash disk");
	if (addr + size > Flash::flash_size()) throw("the flash disk extends beyond flash end");
	if (addr + size > Flash::flash_size() - prefs_size) throw("the flash disk overwrites the preferences");
}

template<int ssw>
QspiFlashDevice<ssw>::QspiFlashDevice(uint32 size, Flags flags) throws :
	QspiFlashDevice(Flash::flash_size() - size, size - prefs_size, flags)
{}

template<int ssw>
void QspiFlashDevice<ssw>::readSectors(LBA block, void* data, SIZE count) throws
{
	clamp_blocks(block, count);
	Flash::readData((first_sector + block) << ssw, data, count << ssw);
}

template<int ssw>
void QspiFlashDevice<ssw>::readData(ADDR addr, void* data, SIZE size) throws
{
	clamp(addr, size);
	Flash::readData((first_sector << ssw) + addr, data, size);
}

template<int ssw>
void QspiFlashDevice<ssw>::writeSectors(LBA block, const void* data, SIZE count) throws
{
	if (!isWritable()) throw NOT_WRITABLE;
	clamp_blocks(block, count);
	if (data) Flash::writeData((first_sector + block) << ssw, data, count << ssw);
	else Flash::eraseData((first_sector + block) << ssw, count << ssw);
}

template<int ssw>
void QspiFlashDevice<ssw>::writeData(ADDR addr, const void* data, SIZE size) throws
{
	if (!isWritable()) throw NOT_WRITABLE;
	clamp(addr, size);
	if (data) Flash::writeData((first_sector << ssw) + addr, data, size);
	else Flash::eraseData((first_sector << ssw) + addr, size);
}

template<int ssw>
uint32 QspiFlashDevice<ssw>::ioctl(IoCtl cmd, void* arg1, void* arg2) throws
{
	switch (cmd.cmd)
	{
		//case IoCtl::CTRL_SYNC: return 0;
		//case IoCtl::GET_SECTOR_SIZE: return 1u << ss_write;
		//case IoCtl::GET_BLOCK_SIZE: return 1u << ss_erase;
		//case IoCtl::FLUSH_IN: return 0;
		//case IoCtl::CTRL_RESET: return 0;
		//case IoCtl::CTRL_CONNECT: return 0;
		//case IoCtl::CTRL_DISCONNECT: return 0;

	case IoCtl::CTRL_TRIM:
	{
		// blkdev->ioctl(IoCtl(IoCtl::CTRL_TRIM, IoCtl::LBA, IoCtl::SIZE), &lba, &count);
		if (cmd.arg1 != cmd.LBA || cmd.arg2 != cmd.SIZE || !arg1 || !arg2) throw INVALID_ARGUMENT;
		LBA	 lba   = *reinterpret_cast<LBA*>(arg1);
		SIZE count = *reinterpret_cast<SIZE*>(arg2);
		writeSectors(lba, nullptr, count);
		return 0;
	}
	default: //
		return BlockDevice::ioctl(cmd, arg1, arg2);
	}
}


template class QspiFlashDevice<8>;
template class QspiFlashDevice<9>;
template class QspiFlashDevice<12>;


} // namespace kio::Devices

/*





















*/
