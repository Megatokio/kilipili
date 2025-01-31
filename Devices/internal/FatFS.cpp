// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FatFS.h"
#include "FatDir.h"
#include "FatFile.h"
#include "Logger.h"
#include "Trace.h"
#include "basic_math.h"
#include "cdefs.h"
#include "cstrings.h"
#include "devices_types.h"
#include "ff15/source/diskio.h"
#include "ff15/source/ffconf.h"
#include <pico/stdio.h>

// volume names as required by FatFS:
constexpr char NoDevice[]			 = "";
cstr		   VolumeStr[FF_VOLUMES] = {NoDevice, NoDevice, NoDevice, NoDevice};

static const cstr ff_errors[] = {
	/*  FR_OK,					*/ "Success",
	/*  FR_DISK_ERR,			*/ "A hard error occurred in the low level disk I/O layer",
	/*  FR_INT_ERR,				*/ "Assertion failed",
	/*  FR_NOT_READY,			*/ "The physical drive cannot work",
	/*  FR_NO_FILE,				*/ kio::Devices::FILE_NOT_FOUND,
	/*  FR_NO_PATH,				*/ kio::Devices::DIRECTORY_NOT_FOUND,
	/*  FR_INVALID_NAME,		*/ "The path name format is invalid",
	/*  FR_DENIED,				*/ "Access denied due to prohibited access or directory full",
	/*  FR_EXIST,				*/ "Access denied due to prohibited access",
	/*  FR_INVALID_OBJECT,		*/ "The file/directory object is invalid",
	/*  FR_WRITE_PROTECTED,		*/ "The physical drive is write protected",
	/*  FR_INVALID_DRIVE,		*/ "The logical drive number is invalid",
	/*  FR_NOT_ENABLED,			*/ "The volume has no work area",
	/*  FR_NO_FILESYSTEM,		*/ "There is no valid FAT volume",
	/*  FR_MKFS_ABORTED,		*/ "The f_mkfs() aborted due to any problem",
	/*  FR_TIMEOUT,				*/ kio::Devices::TIMEOUT,
	/*  FR_LOCKED,				*/ "The operation is rejected according to the file sharing policy",
	/*  FR_NOT_ENOUGH_CORE,		*/ "LFN working buffer could not be allocated",
	/*  FR_TOO_MANY_OPEN_FILES, */ "Number of open files > FF_FS_LOCK",
	/*  FR_INVALID_PARAMETER	*/ kio::Devices::INVALID_ARGUMENT,
};

cstr tostr(FRESULT err) noexcept
{
	if (err < NELEM(ff_errors)) return ff_errors[err];
	else return "FatFS unknown error";
}


namespace kio::Devices
{

// BlockDevices matching VolumeStr[]:
static RCPtr<BlockDevice> blkdevs[FF_VOLUMES];


FatFS::FatFS(cstr name, BlockDevicePtr bdev) throws : // ctor
	FileSystem(name)
{
	trace("FatFS::FatFS");
	debugstr("FatFS(%s)\n", name);

	int idx = index_of(this);
	assert(uint(idx) <= FF_VOLUMES);
	assert(bdev != nullptr);

	blkdevs[idx]   = bdev;
	VolumeStr[idx] = this->name;
	if (FRESULT err = f_mount(&fatfs, catstr(name, ":"), 1 /*mount now*/))
	{
		VolumeStr[idx] = NoDevice; // dtor not called
		blkdevs[idx]   = nullptr;  // if we throw!
		throw tostr(err);
	}
}

FatFS::~FatFS() // dtor
{
	trace("FatFS::~FatFS");
	debugstr("~FatFS\n");

	int idx = index_of(this);
	assert(uint(idx) <= FF_VOLUMES);

	if (VolumeStr[idx] == name)
	{
		FRESULT err = f_mount(nullptr, catstr(name, ":"), 0); // unmount, unregister buffers
		if (err) logline("unmount error: %s", tostr(err));
	}
	VolumeStr[idx] = NoDevice;
	blkdevs[idx]   = nullptr;
}

uint64 FatFS::getFree()
{
	trace("FatFS::getFree");

	DWORD	num_clusters;
	FATFS*	fatfsptr = nullptr;
	FRESULT err		 = f_getfree(catstr(name, ":"), &num_clusters, &fatfsptr);
	if (err) throw tostr(err);
	assert(fatfsptr == &fatfs);
	return uint64(num_clusters) * uint(fatfs.csize << 9);
}

uint64 FatFS::getSize() { return uint64(fatfs.n_fatent - 2) * uint(fatfs.csize << 9); }

DirectoryPtr FatFS::openDir(cstr path)
{
	trace("FatFS::openDir");

	path = makeAbsolutePath(path);
	return new FatDir(this, path);
}

FilePtr FatFS::openFile(cstr path, FileOpenMode flags)
{
	trace("FatFS::openFile");

	path = makeAbsolutePath(path);
	return new FatFile(this, path, flags);
}

void FatFS::mkfs(BlockDevice* blkdev, int idx, cstr /*type*/)
{
	trace("FatFS::mkfs");

	if (blkdevs[idx]) throw DEVICE_IN_USE;

	const uint	ssx	   = max(9u, blkdev->ss_erase);
	const uint	align  = 1 << (ssx - 9);
	const uint8 fmtopt = blkdev->flags & partition ? FM_ANY | FM_SFD : FM_ANY;
	const uint	n_mix  = uint(blkdev->totalSize() >> 16) >> (ssx - 7) << (ssx - 5);
	const uint	n_root = minmax(align >> 5, n_mix, 512u);

	MKFS_PARM options = {
		.fmt	 = fmtopt, // Format option (FM_FAT, FM_FAT32, FM_EXFAT and FM_SFD)
		.n_fat	 = 1,	   // Number of FATs for FAT/FAT32: 1 or 2
		.align	 = align,  // Data area alignment (sector)
		.n_root	 = n_root, // Number of root directory entries
		.au_size = 0	   // Cluster size (byte): 0=default
	};

	debugstr("mkfs fmt    = %i\n", options.fmt);
	debugstr("mkfs n_fat  = %i\n", options.n_fat);
	debugstr("mkfs align  = %u\n", options.align);
	debugstr("mkfs n_root = %u\n", options.n_root);
	debugstr("mkfs au_siz = %u\n", options.au_size);

	uint  bu_size = 64 kB;
	void* buffer;
	while (!(buffer = malloc(bu_size)) && (bu_size /= 2) >= FF_MAX_SS) {}
	if (!buffer) throw OUT_OF_MEMORY;

	char name[2]   = {char('0' + idx), 0};
	VolumeStr[idx] = name;
	blkdevs[idx]   = blkdev;
	FRESULT err	   = f_mkfs(catstr(name, ":"), &options, buffer, bu_size);
	VolumeStr[idx] = NoDevice;
	blkdevs[idx]   = nullptr;

	free(buffer);
	if (err) throw tostr(err);
}

} // namespace kio::Devices


// ######################################################################


using namespace kio;
using namespace kio::Devices;

DSTATUS disk_status(BYTE id)
{
	trace("FatFS::disk_status");
	//debugstr("%s\n", "FatFS::disk_status");

	// required callback for FatFS:

	// id = Physical drive number to identify the drive

	// STA_NOINIT		0x01	/* Drive not initialized */
	// STA_NODISK		0x02	/* No medium in the drive */
	// STA_PROTECT		0x04	/* Write protected */

	assert(id < FF_VOLUMES && blkdevs[id] != nullptr);
	BlockDevice* blkdev = blkdevs[id];

	if (blkdev->isWritable()) return 0;
	if (blkdev->isReadable()) return STA_PROTECT;
	else return blkdev->isRemovable() ? STA_NODISK | STA_NOINIT : STA_NOINIT;
}

DSTATUS disk_initialize(BYTE id)
{
	trace("FatFS::disk_initialize");
	//debugstr("FatFS::disk_initialize(%i)", id);

	// required callback for FatFS:

	// id = Physical drive number to identify the drive

	// STA_NOINIT		0x01	/* Drive not initialized */
	// STA_NODISK		0x02	/* No medium in the drive */
	// STA_PROTECT		0x04	/* Write protected */

	assert_lt(id, FF_VOLUMES);
	assert(blkdevs[id] != nullptr);
	BlockDevice* blkdev = blkdevs[id];

	try
	{
		blkdev->ioctl(IoCtl::CTRL_CONNECT);
	}
	catch (Error e)
	{
		logline("FatFS::disk_initialize: %s", e);
	}
	catch (...)
	{
		logline("FatFS::disk_initialize: unknown error");
	}
	return disk_status(id);
}

DRESULT disk_read(BYTE id, BYTE* buff, LBA_t sector, UINT count)
{
	trace("FatFS::disk_read");
	//debugstr("%s\n","FatFS::disk_read");

	// required callback for FatFS:

	// BYTE id,  		/* Physical drive number to identify the drive */
	// BYTE* buff,		/* Data buffer to store read data */
	// LBA_t sector,	/* Start sector in LBA */
	// UINT count		/* Number of sectors to read */

	// RES_OK = 0,		/* 0: Successful */
	// RES_ERROR,		/* 1: R/W Error */
	// RES_WRPRT,		/* 2: Write Protected */
	// RES_NOTRDY,		/* 3: Not Ready */
	// RES_PARERR		/* 4: Invalid Parameter */

	assert(id < FF_VOLUMES && blkdevs[id] != nullptr);
	BlockDevice* blkdev = blkdevs[id];

	try
	{
		blkdev->readSectors(sector, ptr(buff), count);
		return RES_OK;
	}
	catch (Error e)
	{
		logline("FatFS::disk_read: %s", e);
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		logline("FatFS::disk_read: unknown error");
		return RES_ERROR;
	}
}

DRESULT disk_write(BYTE id, const BYTE* buff, LBA_t sector, UINT count)
{
	trace("FatFS::disk_write");
	//debugstr("%s\n","FatFS::disk_write");

	// required callback for FatFS:

	// BYTE id,  		/* Physical drive nmuber to identify the drive */
	// BYTE* buff,		/* Data buffer to store read data */
	// LBA_t sector,	/* Start sector in LBA */
	// UINT count		/* Number of sectors to read */

	// RES_OK = 0,		/* 0: Successful */
	// RES_ERROR,		/* 1: R/W Error */
	// RES_WRPRT,		/* 2: Write Protected */
	// RES_NOTRDY,		/* 3: Not Ready */
	// RES_PARERR		/* 4: Invalid Parameter */

	assert(id < FF_VOLUMES && blkdevs[id] != nullptr);
	BlockDevice* blkdev = blkdevs[id];

	try
	{
		if (!blkdev->isWritable()) return RES_WRPRT;

		blkdev->writeSectors(sector, cptr(buff), count);
		return RES_OK;
	}
	catch (Error e)
	{
		logline("FatFS::disk_write: %s", e);

		if (e == END_OF_FILE) return RES_PARERR;
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		logline("FatFS::disk_write: unknown error");
		return RES_ERROR;
	}
}

DRESULT disk_ioctl(BYTE id, BYTE cmd, void* buff)
{
	trace("FatFS::disk_ioctl");
	//debugstr("%s\n","FatFS::disk_ioctl");

	// required callback for FatFS:

	// BYTE  id,	/* Physical drive nmuber (0..) */
	// BYTE  cmd,	/* Control code */
	// void* buff	/* Buffer to send/receive control data */

	//  /* Generic command (Used by FatFs) */
	//  #define CTRL_SYNC			0	/* Complete pending write process (needed at FF_FS_READONLY == 0) */
	//  #define GET_SECTOR_COUNT	1	/* Get media size (needed at FF_USE_MKFS == 1) */
	//  #define GET_SECTOR_SIZE		2	/* Get sector size (needed at FF_MAX_SS != FF_MIN_SS) */
	//  #define GET_BLOCK_SIZE		3	/* Get erase block size (needed at FF_USE_MKFS == 1) */
	//  #define CTRL_TRIM			4	/* Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1) */

	// RES_OK = 0,		/* 0: Successful */
	// RES_ERROR,		/* 1: R/W Error */
	// RES_WRPRT,		/* 2: Write Protected */
	// RES_NOTRDY,		/* 3: Not Ready */
	// RES_PARERR		/* 4: Invalid Parameter */

	assert(id < FF_VOLUMES && blkdevs[id] != nullptr);
	BlockDevice* blkdev = blkdevs[id];

	try
	{
		constexpr uint ctrl_sync		= CTRL_SYNC;
		constexpr uint get_sector_count = GET_SECTOR_COUNT;
		constexpr uint get_sector_size	= GET_SECTOR_SIZE;
		constexpr uint get_block_size	= GET_BLOCK_SIZE;
		constexpr uint ctrl_trim		= CTRL_TRIM;
#undef CTRL_SYNC
#undef GET_SECTOR_COUNT
#undef GET_SECTOR_SIZE
#undef GET_BLOCK_SIZE
#undef CTRL_TRIM
		static_assert(IoCtl::CTRL_SYNC == ctrl_sync);
		static_assert(IoCtl::GET_SECTOR_COUNT == get_sector_count);
		static_assert(IoCtl::GET_SECTOR_SIZE == get_sector_size);
		static_assert(IoCtl::GET_BLOCK_SIZE == get_block_size);
		static_assert(IoCtl::CTRL_TRIM == ctrl_trim);

		// FatFS uses 512 byte sectors, except if configured to handle multiple sizes,
		// but 512 bytes is the minimum.
		// for devices with larger sectors we expect FatFS::FF_MAX_SS to be set accordingly.
		// for devices with smaller sectors we package them to 512 byte units.

		switch (cmd)
		{
		case IoCtl::GET_SECTOR_SIZE:
		{
			assert(buff);
			//static_assert(sizeof(WORD) == sizeof(FATFS::ssize));
			*reinterpret_cast<WORD*>(buff) = WORD(std::max(blkdev->sectorSize(), 512u));
			return RES_OK;
		}
		case IoCtl::GET_SECTOR_COUNT:
		{
			assert(buff);
			Devices::LBA sectorcount = blkdev->sectorCount();
			if (blkdev->ss_write < 9) sectorcount >>= 9 - blkdev->ss_write;
			*reinterpret_cast<LBA_t*>(buff) = sectorcount;
			return RES_OK;
		}
		case IoCtl::GET_BLOCK_SIZE:
		{
			assert(buff);
			*reinterpret_cast<DWORD*>(buff) = std::max(blkdev->eraseBlockSize(), 512u);
			return RES_OK;
		}
		case IoCtl::CTRL_TRIM:
		{
			assert(buff);
			LBA_t*		  bu	= reinterpret_cast<LBA_t*>(buff);
			Devices::LBA  lba	= bu[0];
			Devices::SIZE count = bu[1] - bu[0] + 1;

			if (blkdev->ss_write < 9)
			{
				lba <<= 9 - blkdev->ss_write;
				count <<= 9 - blkdev->ss_write;
			}

			blkdev->ioctl(IoCtl(IoCtl::CTRL_TRIM, IoCtl::LBA, IoCtl::SIZE), &lba, &count);
			return RES_OK;
		}
		default:
		{
			if (buff) throw "buff != null: TODO";
			blkdev->ioctl(IoCtl::Cmd(cmd));
			return RES_OK;
		}
		}
	}
	catch (Error e)
	{
		logline("FatFS::ioctl: %s", e);

		if (e == INVALID_ARGUMENT) return RES_PARERR;
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		logline("FatFS::ioctl: unknown error");
		return RES_ERROR;
	}
}

/*



































*/
