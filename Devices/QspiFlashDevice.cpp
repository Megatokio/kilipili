// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "QspiFlashDevice.h"
#include "QspiFlash.h"


namespace kio::Devices
{
static constexpr int sse = QspiFlash::sse;

template<int ssw>
QspiFlashDevice<ssw>::QspiFlashDevice(uint32 start, uint32 size, Flags flags) noexcept :
	BlockDevice((size ? size : QspiFlash::flash_size() - start) >> ssw, 0, ssw, sse, flags),
	first_sector(start >> ssw)
{
	if (start % (1 << sse) != 0) panic("QspiFlashDevice@56");				 // must be aligned to erase sector size
	if (size % (1 << sse) != 0) panic("QspiFlashDevice@57");				 // must be aligned to erase sector size
	if (QspiFlash::flash_binary_size() > start) panic("QspiFlashDevice@58"); // The program overwrote the flash disk.
}

template<int ssw>
void QspiFlashDevice<ssw>::readSectors(LBA block, void* data, SIZE count) throws
{
	clamp_blocks(block, count);
	QspiFlash::readData((first_sector + block) << ssw, data, count << ssw);
}

template<int ssw>
void QspiFlashDevice<ssw>::readData(ADDR addr, void* data, SIZE size) throws
{
	clamp(addr, size);
	QspiFlash::readData((first_sector << ssw) + addr, data, size);
}

template<int ssw>
void QspiFlashDevice<ssw>::writeSectors(LBA block, const void* data, SIZE count) throws
{
	if (!isWritable()) throw NOT_WRITABLE;
	clamp_blocks(block, count);
	if (data) QspiFlash::writeData((first_sector + block) << ssw, data, count << ssw);
	else QspiFlash::eraseData((first_sector + block) << ssw, count << ssw);
}

template<int ssw>
void QspiFlashDevice<ssw>::writeData(ADDR addr, const void* data, SIZE size) throws
{
	if (!isWritable()) throw NOT_WRITABLE;
	clamp(addr, size);
	if (data) QspiFlash::writeData((first_sector << ssw) + addr, data, size);
	else QspiFlash::eraseData((first_sector << ssw) + addr, size);
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
