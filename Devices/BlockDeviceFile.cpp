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
	fsize(bdev->getSize())
{}


SIZE BlockDeviceFile::read(void* _data, SIZE count, bool partial)
{
	ptr data = ptr(_data);

	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
	}
	
	int	 ss	   = bdev->ss_write;
	SIZE ssize = 1 << ss;
	SIZE smask = ssize - 1;
	SIZE rem   = count;

	if (fpos & smask)
	{
		SIZE cnt = min(rem, ssize - (SIZE(fpos) & smask));
		bdev->readPartialSector(LBA(fpos >> ss), data, fpos & smask, cnt);
		fpos += cnt;
		data += cnt;
		rem -= cnt;
	}

	if (rem > ssize)
	{
		bdev->readSectors(LBA(fpos >> ss), data, rem >> ss);
		fpos += rem - (rem & smask);
		data += rem - (rem & smask);
		rem &= smask;
	}

	if (rem)
	{
		bdev->readPartialSector(LBA(fpos >> ss), data, 0, rem);
		fpos += rem;
	}

	return count;
}

SIZE BlockDeviceFile::write(const void* _data, SIZE count, bool partial)
{
	cptr data = cptr(_data);

	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
	}

	int	 ss	   = bdev->ss_write;
	SIZE ssize = 1 << ss;
	SIZE smask = ssize - 1;
	SIZE rem   = count;

	if (fpos & smask)
	{
		SIZE cnt = min(rem, ssize - (SIZE(fpos) & smask));
		bdev->writePartialSector(LBA(fpos >> ss), data, fpos & smask, cnt);
		fpos += cnt;
		data += cnt;
		rem -= cnt;
	}

	if (rem > ssize)
	{
		bdev->writeSectors(LBA(fpos >> ss), data, rem >> ss);
		fpos += rem - (rem & smask);
		data += rem - (rem & smask);
		rem &= smask;
	}

	if (rem)
	{
		bdev->writePartialSector(LBA(fpos >> ss), data, 0, rem);
		fpos += rem;
	}

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
