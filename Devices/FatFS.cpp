// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FatFS.h"
#include "Logger.h"
#include "devices_types.h"
#include "ff14b/diskio.h"
#include "ff14b/ffconf.h"
#include "utilities.h"


extern cstr VolumeStr[FF_VOLUMES];

constexpr cstr				no_drive				 = "";
cstr						VolumeStr[FF_VOLUMES]	 = {no_drive};
static kio::Devices::FatFS* file_systems[FF_VOLUMES] = {nullptr};


namespace kio::Devices
{


FatFS::FatFS(BlockDevice& blkdev, cstr name) : FileSystem(blkdev, name)
{
	assert(name && *name);

	for (uint i = 0; i < NELEM(VolumeStr); i++)
	{
		if (lceq(name, VolumeStr[i])) throw "volume name already exists";
	}

	for (uint i = 0; i < NELEM(VolumeStr); i++)
	{
		if (file_systems[i] == nullptr)
		{
			VolumeStr[i]	= name;
			file_systems[i] = this;
			f_mount(&fatfs, name, 0 /*don't mount*/);
			return;
		}
	}
	throw "FatFS: no more volumeIDs";
}

FatFS::~FatFS()
{
	if (fatfs.fs_type != 0) f_mount(&fatfs, nullptr, 0); // unmount

	uint i = this->fatfs.pdrv;
	assert(lceq(name, VolumeStr[i]));

	file_systems[i] = nullptr;
	VolumeStr[i]	= no_drive;
}

void FatFS::mkfs()
{
	const uint	ssx	   = max(9u, blkdev.ss_erase);
	const uint	align  = 1 << (ssx - 9);
	const uint8 fmtopt = blkdev.flags & partition ? FM_ANY | FM_SFD : FM_ANY;
	const uint	n_mix  = uint(blkdev.getSize() >> 16) >> (ssx - 7) << (ssx - 5);
	const uint	n_root = minmax(align >> 5, n_mix, 512u);

	MKFS_PARM options = {
		.fmt	 = fmtopt, // Format option (FM_FAT, FM_FAT32, FM_EXFAT and FM_SFD)
		.n_fat	 = 1,	   // Number of FATs for FAT/FAT32: 1 or 2
		.align	 = align,  // Data area alignment (sector)
		.n_root	 = n_root, // Number of root directory entries
		.au_size = 0	   // Cluster size (byte): 0=default
	};

	uint  bu_size = 64 kB;
	void* buffer;
	while (!(buffer = malloc(bu_size)) && (bu_size /= 2) >= FF_MAX_SS) {}
	FRESULT err = f_mkfs(name, &options, buffer, bu_size);
	free(buffer);
	if (err) throw tostr(err);
}

void FatFS::mount()
{
	FRESULT err = f_mount(&fatfs, name, 1 /*mount now*/);
	if (err) throw tostr(err);
}

ADDR FatFS::getFree()
{
	DWORD	nclst;
	FATFS*	fatfsptr = nullptr;
	FRESULT err		 = f_getfree(name, &nclst, &fatfsptr);
	if (err) throw tostr(err);
	assert(fatfsptr == &fatfs);
	return ADDR(nclst) * uint(fatfs.csize << 9);
}

ADDR FatFS::getSize()
{
	ADDR tot_sect = (ADDR(fatfs.n_fatent - 2) * uint(fatfs.csize << 9));
	return tot_sect;
}

// ######################################################################

__unused static cstr tostr(DRESULT err) noexcept
{
	constexpr cstr dresults[] = {
		"Success",				 // RES_OK = 0
		"Disk R/W Error",		 // RES_ERROR
		"Disk Write Protected",	 // RES_WRPRT
		"Disk Not Ready",		 // RES_NOTRDY
		"Disk Invalid Parameter" // RES_PARERR
	};

	if (err < NELEM(dresults)) return dresults[err];
	else return "FatFS disk unknown error";
}

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE id)
{
	// required callback for FatFS:

	// id = Physical drive number to identify the drive

	// STA_NOINIT		0x01	/* Drive not initialized */
	// STA_NODISK		0x02	/* No medium in the drive */
	// STA_PROTECT		0x04	/* Write protected */

	auto* volume = id < FF_VOLUMES ? &file_systems[id]->blkdev : nullptr;
	if (!volume) return STA_NOINIT;

	if (volume->is_writable()) return 0;
	if (volume->is_readable()) return STA_PROTECT;
	else return STA_NODISK;
}

DSTATUS disk_initialize(BYTE id)
{
	// required callback for FatFS:

	// id = Physical drive number to identify the drive

	// STA_NOINIT		0x01	/* Drive not initialized */
	// STA_NODISK		0x02	/* No medium in the drive */
	// STA_PROTECT		0x04	/* Write protected */

	auto* device = id < FF_VOLUMES ? &file_systems[id]->blkdev : nullptr;
	if (!device) return STA_NOINIT;

	// device->connect(); TODO: e.g. sdcard
	return disk_status(id);
}

DRESULT disk_read(BYTE id, BYTE* buff, LBA_t sector, UINT count)
{
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

	auto* device = id < FF_VOLUMES ? &file_systems[id]->blkdev : nullptr;
	if (!device) return RES_PARERR;

	try
	{
		device->readSectors(sector, ptr(buff), count);
		return RES_OK;
	}
	catch (cstr e)
	{
		logline("fatfs.disk_read: %s", e);
		return RES_ERROR;
	}
	catch (...)
	{
		logline("fatfs.disk_read: unknown error");
		return RES_ERROR;
	}
}

DRESULT disk_write(BYTE id, const BYTE* buff, LBA_t sector, UINT count)
{
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

	auto* device = id < FF_VOLUMES ? &file_systems[id]->blkdev : nullptr;
	if (!device) return RES_PARERR;

	try
	{
		if (!device->is_writable()) return RES_WRPRT;

		device->writeSectors(sector, ptr(buff), count);
		return RES_OK;
	}
	catch (cstr e)
	{
		logline("fatfs.disk_write: %s", e);

		if (e == END_OF_FILE) return RES_PARERR;
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		logline("fatfs.disk_write: unknown error");
		return RES_ERROR;
	}
}

DRESULT disk_ioctl(BYTE id, BYTE cmd, void* buff)
{
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

	BlockDevice* device = id < FF_VOLUMES ? &file_systems[id]->blkdev : nullptr;
	if (!device) return RES_PARERR;

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

		if (buff == nullptr) device->ioctl(IoCtl(IoCtl::Cmd(cmd)), buff);
		throw "buff != null: TODO";
	}
	catch (cstr e)
	{
		logline("fatfs.ioctl: %s", e);

		if (e == INVALID_ARGUMENT) return RES_PARERR;
		if (e == TIMEOUT) return RES_NOTRDY;
		return RES_ERROR;
	}
	catch (...)
	{
		logline("fatfs.ioctl: unknown error");
		return RES_ERROR;
	}
}

} // namespace kio::Devices

/*



































*/
