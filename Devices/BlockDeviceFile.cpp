// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BlockDeviceFile.h"
#include "cdefs.h"

namespace kio::Devices
{

BlockDeviceFile::BlockDeviceFile(RCPtr<BlockDevice> bdev) noexcept :
	File(bdev->flags),
	bdev(std::move(bdev)),
	fsize(bdev->totalSize())
{}


SIZE BlockDeviceFile::read(void* _data, SIZE count, bool partial)
{
	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
	}

	bdev->readData(fpos, _data, count);
	fpos += count;
	return count;
}

SIZE BlockDeviceFile::write(const void* _data, SIZE count, bool partial)
{
	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
	}

	bdev->writeData(fpos, _data, count);
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

int BlockDeviceFile::getc(uint /*timeout_us*/)
{
	if (unlikely(fpos == fsize)) return -1;

	read(&last_char, 1);
	fpos += 1;
	return uchar(last_char);
}

char BlockDeviceFile::getc()
{
	read(&last_char, 1);
	fpos += 1;
	return last_char;
}

SIZE BlockDeviceFile::putc(char c)
{
	write(&c, 1);
	fpos += 1;
	return 1;
}

void BlockDeviceFile::close(bool)
{
	bdev->sync(); //
}

} // namespace kio::Devices

/*


  
  


























*/
