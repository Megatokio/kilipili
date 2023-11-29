// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FatFS.h"
#include "FatDir.h"
#include "FatFile.h"
#include "Logger.h"
#include "basic_math.h"
#include "cstrings.h"
#include "devices_types.h"
#include "ff15/source/diskio.h"
#include "ff15/source/ffconf.h"
#include "kilipili_cdefs.h"
#include "pico/stdio.h"


static const cstr ff_errors[] = {
	/*  FR_OK,					*/ "Success",
	/*  FR_DISK_ERR,			*/ "A hard error occurred in the low level disk I/O layer",
	/*  FR_INT_ERR,				*/ "Assertion failed",
	/*  FR_NOT_READY,			*/ "The physical drive cannot work",
	/*  FR_NO_FILE,				*/ "Could not find the file",
	/*  FR_NO_PATH,				*/ "Could not find the path",
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

FatFS::FatFS(cstr name, BlockDevice* blkdev, int idx) throws : // ctor
	FileSystem(name, blkdev)
{
	printf("FatFS::FatFS\n");

	assert(uint(idx) <= FF_VOLUMES);
}

FatFS::~FatFS() // dtor
{
	printf("FatFS::~FatFS\n");

	FRESULT err = f_mount(&fatfs, nullptr, 0); // unmount, unregister buffers
	if (err) logline("unmount error: %s", tostr(err));
}

ADDR FatFS::getFree()
{
	DWORD	num_clusters;
	FATFS*	fatfsptr = nullptr;
	FRESULT err		 = f_getfree(name, &num_clusters, &fatfsptr);
	if (err) throw tostr(err);
	assert(fatfsptr == &fatfs);
	return ADDR(num_clusters) * uint(fatfs.csize << 9);
}

ADDR FatFS::getSize()
{
	ADDR total_size = (ADDR(fatfs.n_fatent - 2) * uint(fatfs.csize << 9));
	return total_size;
}

bool FatFS::mount()
{
	printf("FatFS::mount\n");

	FRESULT err = f_mount(&fatfs, name, 1 /*mount now*/);
	if (err && err != FR_NO_FILESYSTEM) throw tostr(err);
	return !err; // ok = true
}

DirectoryPtr FatFS::openDir(cstr path)
{
	path = makeAbsolutePath(path);
	return new FatDir(this, path);
}

FilePtr FatFS::openFile(cstr path, FileOpenMode flags)
{
	path = makeAbsolutePath(path);
	return new FatFile(this, path, flags);
}

void FatFS::mkfs(BlockDevice* blkdev, int idx, cstr /*type*/)
{
	const uint	ssx	   = max(9u, blkdev->ss_erase);
	const uint	align  = 1 << (ssx - 9);
	const uint8 fmtopt = blkdev->flags & partition ? FM_ANY | FM_SFD : FM_ANY;
	const uint	n_mix  = uint(blkdev->getSize() >> 16) >> (ssx - 7) << (ssx - 5);
	const uint	n_root = minmax(align >> 5, n_mix, 512u);

	MKFS_PARM options = {
		.fmt	 = fmtopt, // Format option (FM_FAT, FM_FAT32, FM_EXFAT and FM_SFD)
		.n_fat	 = 1,	   // Number of FATs for FAT/FAT32: 1 or 2
		.align	 = align,  // Data area alignment (sector)
		.n_root	 = n_root, // Number of root directory entries
		.au_size = 0	   // Cluster size (byte): 0=default
	};

	char  name[2] = {char('0' + idx), 0};
	uint  bu_size = 64 kB;
	void* buffer;
	while (!(buffer = malloc(bu_size)) && (bu_size /= 2) >= FF_MAX_SS) {}
	if (!buffer) throw OUT_OF_MEMORY;
	FRESULT err = f_mkfs(name, &options, buffer, bu_size);
	free(buffer);
	if (err) throw tostr(err);
}

} // namespace kio::Devices


// ######################################################################


using namespace kio;
using namespace kio::Devices;


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE id)
{
	printf("%s\n", __func__);

	// required callback for FatFS:

	// id = Physical drive number to identify the drive

	// STA_NOINIT		0x01	/* Drive not initialized */
	// STA_NODISK		0x02	/* No medium in the drive */
	// STA_PROTECT		0x04	/* Write protected */

	assert(id < FF_VOLUMES && file_systems[id] != nullptr && file_systems[id]->blkdev);
	BlockDevice* blkdev = file_systems[id]->blkdev;

	if (blkdev->isWritable()) return 0;
	if (blkdev->isReadable()) return STA_PROTECT;
	else return STA_NODISK;
}

DSTATUS disk_initialize(BYTE id)
{
	// required callback for FatFS:

	// id = Physical drive number to identify the drive

	// STA_NOINIT		0x01	/* Drive not initialized */
	// STA_NODISK		0x02	/* No medium in the drive */
	// STA_PROTECT		0x04	/* Write protected */

	printf("***disk_initialize***\n");

	assert(id < FF_VOLUMES && file_systems[id] != nullptr && file_systems[id]->blkdev);
	BlockDevice* blkdev = file_systems[id]->blkdev;

	try
	{
		blkdev->ioctl(IoCtl::CTRL_CONNECT);
		return disk_status(id);
	}
	catch (Error e)
	{
		printf("fatfs.disk_initialize: %s\n", e);
		return STA_NODISK;
	}
	catch (...)
	{
		printf("fatfs.disk_initialize: unknown error\n");
		return STA_NODISK;
	}
}

DRESULT disk_read(BYTE id, BYTE* buff, LBA_t sector, UINT count)
{
	printf("%s\n", __func__);

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

	assert(id < FF_VOLUMES && file_systems[id] != nullptr && file_systems[id]->blkdev);
	BlockDevice* blkdev = file_systems[id]->blkdev;

	try
	{
		blkdev->readSectors(sector, ptr(buff), count);
		return RES_OK;
	}
	catch (Error e)
	{
		printf("fatfs.disk_read: %s\n", e);
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		printf("fatfs.disk_read: unknown error\n");
		return RES_ERROR;
	}
}

DRESULT disk_write(BYTE id, const BYTE* buff, LBA_t sector, UINT count)
{
	printf("%s\n", __func__);

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

	assert(id < FF_VOLUMES && file_systems[id] != nullptr && file_systems[id]->blkdev);
	BlockDevice* blkdev = file_systems[id]->blkdev;

	try
	{
		if (!blkdev->isWritable()) return RES_WRPRT;

		blkdev->writeSectors(sector, ptr(buff), count);
		return RES_OK;
	}
	catch (cstr e)
	{
		printf("fatfs.disk_write: %s\n", e);

		if (e == END_OF_FILE) return RES_PARERR;
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		printf("fatfs.disk_write: unknown error\n");
		return RES_ERROR;
	}
}

DRESULT disk_ioctl(BYTE id, BYTE cmd, void* buff)
{
	printf("%s\n", __func__);

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

	assert(id < FF_VOLUMES && file_systems[id] != nullptr && file_systems[id]->blkdev);
	BlockDevice* blkdev = file_systems[id]->blkdev;

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

		if (buff == nullptr) blkdev->ioctl(IoCtl::Cmd(cmd));
		throw "buff != null: TODO";
	}
	catch (Error e)
	{
		printf("fatfs.ioctl: %s\n", e);

		if (e == INVALID_ARGUMENT) return RES_PARERR;
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		printf("fatfs.ioctl: unknown error\n");
		return RES_ERROR;
	}
}

/*



































*/
