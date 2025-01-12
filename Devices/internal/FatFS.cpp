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

#undef debugstr
#define debugstr(...) void(0)

// volume names as required by FatFS:
constexpr char NoDevice[]			 = "";
cstr		   VolumeStr[FF_VOLUMES] = {"", "", "", ""};

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


FatFS::FatFS(cstr name, BlockDevicePtr bdev, int idx) throws : // ctor
	FileSystem(name),
	volume_idx(idx)
{
	trace(__func__);

	debugstr("FatFS::FatFS\n");

	assert(uint(idx) <= FF_VOLUMES);
	blkdevs[idx] = bdev;
}

FatFS::~FatFS() // dtor
{
	trace(__func__);

	debugstr("FatFS::~FatFS\n");

	FRESULT err			  = f_mount(nullptr, name, 0); // unmount, unregister buffers
	VolumeStr[volume_idx] = NoDevice;
	blkdevs[volume_idx]	  = nullptr;
	if (err) logline("unmount error: %s", tostr(err));
}

ADDR FatFS::getFree()
{
	trace(__func__);

	DWORD	num_clusters;
	FATFS*	fatfsptr = nullptr;
	FRESULT err		 = f_getfree(name, &num_clusters, &fatfsptr);
	if (err) throw tostr(err);
	assert(fatfsptr == &fatfs);
	if constexpr (sizeof(ADDR) >= sizeof(FSIZE_t)) return ADDR(num_clusters) * uint(fatfs.csize << 9);
	uint64 free = uint64(num_clusters) * uint(fatfs.csize << 9);
	if (free == ADDR(free)) return ADDR(free);
	debugstr("FatFS: free size exceeds 4GB\n");
	return 0xffffffffu & ~(uint(fatfs.csize << 9) - 1);
}

ADDR FatFS::getSize()
{
	trace(__func__);

	if constexpr (sizeof(ADDR) >= sizeof(FSIZE_t)) return ADDR(fatfs.n_fatent - 2) * uint(fatfs.csize << 9);
	uint64 total_size = ADDR(fatfs.n_fatent - 2) * uint(fatfs.csize << 9);
	if (total_size == ADDR(total_size)) return ADDR(total_size);
	debugstr("FatFS: total size exceeds 4GB\n");
	return 0xffffffffu & ~(uint(fatfs.csize << 9) - 1);
}

bool FatFS::mount()
{
	trace(__func__);

	debugstr("FatFS::mount\n");

	VolumeStr[volume_idx] = name;
	FRESULT err			  = f_mount(&fatfs, catstr(name, ":"), 1 /*mount now*/);
	if (err) VolumeStr[volume_idx] = NoDevice;
	if (err && err != FR_NO_FILESYSTEM) throw tostr(err);
	return !err; // ok = true
}

DirectoryPtr FatFS::openDir(cstr path)
{
	trace(__func__);

	path = makeAbsolutePath(path);
	return new FatDir(this, path);
}

FilePtr FatFS::openFile(cstr path, FileOpenMode flags)
{
	trace(__func__);

	path = makeAbsolutePath(path);
	return new FatFile(this, path, flags);
}

void FatFS::mkfs(BlockDevice* blkdev, int idx, cstr /*type*/)
{
	trace(__func__);

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

DSTATUS disk_status(BYTE id)
{
	trace(__func__);

	//debugstr("%s\n", __func__);

	// required callback for FatFS:

	// id = Physical drive number to identify the drive

	// STA_NOINIT		0x01	/* Drive not initialized */
	// STA_NODISK		0x02	/* No medium in the drive */
	// STA_PROTECT		0x04	/* Write protected */

	assert(id < FF_VOLUMES && blkdevs[id] != nullptr);
	BlockDevice* blkdev = blkdevs[id];

	if (blkdev->isWritable()) return 0;
	if (blkdev->isReadable()) return STA_PROTECT;
	else return STA_NODISK;
}

DSTATUS disk_initialize(BYTE id)
{
	trace(__func__);

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
	trace(__func__);

	debugstr("%s\n", __func__);

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
	trace(__func__);

	debugstr("%s\n", __func__);

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
	trace(__func__);

	debugstr("%s\n", __func__);

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
