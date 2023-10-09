// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BlockDeviceFile.h"
#include "kilipili_cdefs.h"
#include <memory>

namespace kio::Devices
{

BlockDeviceFile::BlockDeviceFile(BlockDevice& bdev) noexcept :
	File(bdev.flags),
	BlockDevice(bdev),
	bdev(bdev),
	fsize(bdev.getSize())
{}


SIZE BlockDeviceFile::read(char* data, SIZE count, bool partial)
{
	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
	}

	int	 ss	   = ss_write;
	SIZE ssize = 1 << ss;
	SIZE smask = ssize - 1;
	SIZE rem   = count;

	if (fpos & smask)
	{
		SIZE cnt = min(rem, ssize - (SIZE(fpos) & smask));
		readPartialSector(LBA(fpos >> ss), data, fpos & smask, cnt);
		fpos += cnt;
		data += cnt;
		rem -= cnt;
	}

	if (rem > ssize)
	{
		bdev.readSectors(LBA(fpos >> ss), data, rem >> ss);
		fpos += rem - (rem & smask);
		data += rem - (rem & smask);
		rem &= smask;
	}

	if (rem)
	{
		readPartialSector(LBA(fpos >> ss), data, 0, rem);
		fpos += rem;
	}

	return count;
}

SIZE BlockDeviceFile::write(const char* data, SIZE count, bool partial)
{
	if (count > fsize - fpos)
	{
		count = SIZE(fsize - fpos);
		if (!partial) throw END_OF_FILE;
	}

	int	 ss	   = ss_write;
	SIZE ssize = 1 << ss;
	SIZE smask = ssize - 1;
	SIZE rem   = count;

	if (fpos & smask)
	{
		SIZE cnt = min(rem, ssize - (SIZE(fpos) & smask));
		writePartialSector(LBA(fpos >> ss), data, fpos & smask, cnt);
		fpos += cnt;
		data += cnt;
		rem -= cnt;
	}

	if (rem > ssize)
	{
		bdev.writeSectors(LBA(fpos >> ss), data, rem >> ss);
		fpos += rem - (rem & smask);
		data += rem - (rem & smask);
		rem &= smask;
	}

	if (rem)
	{
		writePartialSector(LBA(fpos >> ss), data, 0, rem);
		fpos += rem;
	}

	return count;
}

uint32 BlockDeviceFile::ioctl(IoCtl ctl, void* arg1, void* arg2)
{
	// filter and handle those which are not for the block device:
	(void)0; // currently none

	// send the rest to the block device:
	return bdev.ioctl(ctl, arg1, arg2);
}

void BlockDeviceFile::readSectors(LBA lba, char* data, SIZE count)
{
	bdev.readSectors(lba, data, count); //
}

void BlockDeviceFile::writeSectors(LBA lba, const char* data, SIZE count)
{
	bdev.writeSectors(lba, data, count); //
}

void BlockDeviceFile::readPartialSector(LBA lba, char* data, SIZE offs, SIZE count)
{
	bdev.readPartialSector(lba, data, offs, count);
}

void BlockDeviceFile::writePartialSector(LBA lba, const char* data, SIZE offs, SIZE count)
{
	bdev.writePartialSector(lba, data, offs, count);
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
	bdev.sync(); //
}

} // namespace kio::Devices

/*


  
  


























*/
