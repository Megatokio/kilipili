// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"


namespace kio::Devices
{

using SIZE = uint;	 // byte count for read/write/erase
using ADDR = uint64; // uint32/64: disk size, file size, file position. exFAT needs uint64
using LBA  = uint32; // up to 2TB


constexpr char NOT_WRITABLE[]	  = "not writable";
constexpr char NOT_READABLE[]	  = "not readable";
constexpr char NOT_ERASABLE[]	  = "not erasable";
constexpr char END_OF_FILE[]	  = "end of file";
constexpr char TIMEOUT[]		  = "timeout";
constexpr char INVALID_ARGUMENT[] = "invalid argument";


// IoCtl enums
// match FatFS ioctl() function codes
struct IoCtl
{
	enum Cmd {
		/* Generic command (Used by FatFs) */
		CTRL_SYNC		 = 0, // Complete pending write process (needed at FF_FS_READONLY == 0)
		GET_SECTOR_COUNT = 1, // Get media size (needed at FF_USE_MKFS == 1)
		GET_SECTOR_SIZE	 = 2, // 1 << ss_write // Get sector size (needed at FF_MAX_SS != FF_MIN_SS)
		GET_BLOCK_SIZE	 = 3, // 1 << ss_erase // Get erase block size (needed at FF_USE_MKFS == 1)
		CTRL_TRIM = 4, // Tell device that the data on the block of sectors is no longer used (used if FF_USE_TRIM == 1)

		/* Generic command (Not used by FatFs) */
		CTRL_POWER	= 5, // Get/Set power status
		CTRL_LOCK	= 6, // Lock/Unlock media removal
		CTRL_EJECT	= 7, // Eject media
		CTRL_FORMAT = 8, // Create physical format on the media

		/* MMC/SDC specific ioctl command */
		MMC_GET_TYPE   = 10, // Get card type
		MMC_GET_CSD	   = 11, // Get CSD
		MMC_GET_CID	   = 12, // Get CID
		MMC_GET_OCR	   = 13, // Get OCR
		MMC_GET_SDSTAT = 14, // Get SD status
		ISDIO_READ	   = 55, // Read data form SD iSDIO register
		ISDIO_WRITE	   = 56, // Write data to SD iSDIO register
		ISDIO_MRITE	   = 57, // Masked write data to SD iSDIO register

		/* ATA/CF specific ioctl command */
		ATA_GET_REV	  = 20, // Get F/W revision
		ATA_GET_MODEL = 21, // Get model name
		ATA_GET_SN	  = 22, // Get serial number

		// alias, others:
		FLUSH_OUT	  = CTRL_SYNC,	 // wait & send all pending outputs
		ERASE_SECTORS = CTRL_TRIM,	 // mark sectors as unused: start,count, and format if needed by device
		ERASE_DISK	  = CTRL_FORMAT, // create file system after formatting whole disk, if needed by device

		FLUSH_IN   = 9,	 // flush & discard pending buffered inputs
		CTRL_RESET = 80, // reset internal state, discard pending input and output, keep connected
		CTRL_CONNECT,	 // connect to hardware, load removable disk
		CTRL_DISCONNECT, // disconnect from hardware, unload removable disk
	};

	enum Arg {
		NONE = 0,
		SIZE = sizeof(kio::Devices::SIZE),
		ADDR = sizeof(kio::Devices::ADDR),
		LBA	 = sizeof(kio::Devices::LBA),
	};

	uint cmd  : 16;
	uint arg1 : 8;
	uint arg2 : 8;

	IoCtl(Cmd cmd, Arg a1 = NONE, Arg a2 = NONE) : cmd(cmd), arg1(a1), arg2(a2) {}
};


// Device Flags:
enum Flags : uint8 {
	readable	 = 1,	 //
	writable	 = 2,	 //
	overwritable = 4,	 // can overwrite old data without formatting. else eventually 0xff->anyvalue->0x00.
	partition	 = 0x20, // hint for mkfs: needs partitioning like a HD
	readwrite	 = readable | writable | overwritable,
};
inline Flags operator|(Flags a, Flags b) { return Flags(uint(a) | uint(b)); }


} // namespace kio::Devices
