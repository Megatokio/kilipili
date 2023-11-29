// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FileSystem.h"
#include "FatFS.h"
#include "File.h"
#include "SDCard.h"
#include "cstrings.h"
#include "ff15/source/ffconf.h"
#include "pico/stdio.h"
#include <pico/config.h>


// volume names as required by FatFS:
constexpr char NoDevice[]			 = "";
cstr		   VolumeStr[FF_VOLUMES] = {NoDevice};


namespace kio::Devices
{

//using FSName = char[8];
//static FSName	   fs_names[FF_VOLUMES]		= {""};
FileSystem* file_systems[FF_VOLUMES] = {nullptr};

static constexpr char UNKNOWN_FILESYSTEM[]	= "unknown file system";
static constexpr char UNKNOWN_DEVICE[]		= "unknown device";
static constexpr char DEVICE_IN_USE[]		= "device in use";
static constexpr char FILESYSTEM_IN_USE[]	= "file system in use";
static constexpr char NO_MOUNTPOINT_FREE[]	= "no mountpoint free";
static constexpr char PATH_WITHOUT_DEVICE[] = "invalid path without device";
static constexpr char NAME_TOO_LONG[]		= "name too long";


// ====================================================

int index_of(cstr name)
{
	for (int i = 0; i < FF_VOLUMES; i++)
		if (lceq(name, VolumeStr[i])) return i;
	return -1;
}

int index_of(FileSystem* fs)
{
	for (int i = 0; i < FF_VOLUMES; i++)
		if (fs == file_systems[i]) return i;
	return -1;
}

BlockDevice* newBlockDeviceForName(cstr name)
{
	if (lceq(name, "sdcard"))
	{
		static constexpr uint8 rx  = PICO_DEFAULT_SPI_RX_PIN;
		static constexpr uint8 cs  = PICO_DEFAULT_SPI_CSN_PIN;
		static constexpr uint8 clk = PICO_DEFAULT_SPI_SCK_PIN;
		static constexpr uint8 tx  = PICO_DEFAULT_SPI_TX_PIN;

		return new SDCard(rx, cs, clk, tx);
	}
	throw UNKNOWN_DEVICE;
}

FileSystem::FileSystem(cstr devname, BlockDevice* blkdev) throws : // ctor
	blkdev(blkdev)
{
	if (strlen(devname) >= sizeof(name)) throw NAME_TOO_LONG;
	memcpy(name, devname, sizeof(name));
}

FileSystem::~FileSystem() noexcept
{
	delete[] workdir;

	int idx = index_of(this);
	if (idx >= 0)
	{
		VolumeStr[idx]	  = NoDevice;
		file_systems[idx] = nullptr;
	}
}

void FileSystem::makeFS(BlockDevicePtr bdev, cstr type) throws // static
{
	type = lowerstr(type);
	if (startswith(type, "fat"))
	{
		int idx = index_of(NoDevice); // FatFS needs a slot, even if not mounted
		if (idx < 0) throw NO_MOUNTPOINT_FREE;
		FatFS::mkfs(bdev, idx, type);
	}
	else throw UNKNOWN_FILESYSTEM;
}

void FileSystem::makeFS(cstr devname, cstr type) throws // static
{
	int idx = index_of(devname);
	if (idx >= 0) throw FILESYSTEM_IN_USE;

	makeFS(newBlockDeviceForName(devname), type);
}

FileSystemPtr FileSystem::mount(cstr devname, BlockDevicePtr bdev) throws // static
{
	int idx = index_of(devname);
	if (idx >= 0) throw DEVICE_IN_USE;
	idx = index_of(NoDevice);
	if (idx < 0) throw NO_MOUNTPOINT_FREE;

	// try to mount with all FileSystems we know:
	// (not that many, right now :-)
	{
		FileSystemPtr fs  = new FatFS(devname, bdev, idx);
		file_systems[idx] = fs;
		VolumeStr[idx]	  = fs->name;
		bool success	  = fs->mount();
		if (success) return fs;
	}

	file_systems[idx] = nullptr;
	VolumeStr[idx]	  = NoDevice;
	throw UNKNOWN_FILESYSTEM;
}

FileSystemPtr FileSystem::mount(cstr devname) throws // static
{
	int idx = index_of(devname);
	if (idx >= 0) return file_systems[idx];

	BlockDevicePtr bdev = newBlockDeviceForName(devname);
	return mount(devname, bdev);
}

void FileSystem::setWorkDir(cstr path)
{
	path = makeAbsolutePath(path);
	delete[] workdir;
	workdir = nullptr;

	DirectoryPtr p = openDir(path); // does it mount?
	workdir		   = newcopy(path);
}

cstr FileSystem::makeAbsolutePath(cstr path)
{
	if (path[0] == '/') return path;
	if (workdir) return catstr(workdir, "/", path);
	else return catstr("/", path);
}

DirectoryPtr openDir(cstr path) throws
{
	ptr dp = strchr(path, ':');
	if (!dp) throw PATH_WITHOUT_DEVICE;

	cstr devname = substr(path, dp);
	return FileSystem::mount(devname)->openDir(dp + 1);
}

FilePtr openFile(cstr path, FileOpenMode flags) throws
{
	ptr dp = strchr(path, ':');
	if (!dp) throw PATH_WITHOUT_DEVICE;

	cstr devname = substr(path, dp);
	return FileSystem::mount(devname)->openFile(dp + 1, flags);
}


} // namespace kio::Devices

/*








































*/
