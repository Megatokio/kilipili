// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BlockDeviceFile.h"
#if defined MAKE_TOOLS && MAKE_TOOLS
  #define logline(...) (void)0
#else
  #include "Logger.h"
  #include "cdefs.h"
#endif


namespace kio::Devices
{

BlockDeviceFile::BlockDeviceFile(RCPtr<BlockDevice> bdev) noexcept :
	File(bdev->flags),
	bdev(std::move(bdev)),
	fsize(this->bdev->totalSize())
{
	if constexpr (sizeof(ADDR) < sizeof(uint64))
		if (bdev->sector_count >= 4 GB >> bdev->ss_write) logline("Warning: SDCard size >= 4GB");
}


SIZE BlockDeviceFile::read(void* data, SIZE count, bool partial)
{
	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
		if (eof_pending()) throw END_OF_FILE;
		if (count == 0) set_eof_pending();
	}

	bdev->readData(fpos, data, count);
	fpos += count;
	return count;
}

SIZE BlockDeviceFile::write(const void* data, SIZE count, bool partial)
{
	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
	}

	bdev->writeData(fpos, data, count);
	fpos += count;
	return count;
}

uint32 BlockDeviceFile::ioctl(IoCtl ctl, void* arg1, void* arg2)
{
	// filter and handle those which are not for the block device:
	(void)0; // currently none

	// send the rest to the block device:
	return bdev->ioctl(ctl, arg1, arg2);
}

void BlockDeviceFile::close()
{
	bdev->sync(); //
}

} // namespace kio::Devices

/*


  
  


























*/
