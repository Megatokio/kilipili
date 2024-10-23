// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FileSystem.h"
#include "FatFS.h"
#include "File.h"
#include "RsrcFS.h"
#include "SDCard.h"
#include "Trace.h"
#include "cstrings.h"
#include "ff15/source/ffconf.h"

namespace kio::Devices
{

FileSystem* file_systems[FF_VOLUMES] = {nullptr, nullptr, nullptr, nullptr};

static constexpr char UNKNOWN_FILESYSTEM[]	= "unknown file system";
static constexpr char UNKNOWN_DEVICE[]		= "unknown device";
static constexpr char DEVICE_IN_USE[]		= "device in use";
static constexpr char NO_MOUNTPOINT_FREE[]	= "no mountpoint free";
static constexpr char PATH_WITHOUT_DEVICE[] = "invalid path without device";
static constexpr char NAME_TOO_LONG[]		= "name too long";


// ====================================================

static int index_of(cstr name)
{
	for (int i = 0; i < FF_VOLUMES; i++)
	{
		FileSystem* fs = file_systems[i];
		if (fs && lceq(name, fs->name)) return i;
	}
	return -1;
}

static int index_of(FileSystem* fs)
{
	for (int i = 0; i < FF_VOLUMES; i++)
		if (fs == file_systems[i]) return i;
	return -1;
}

static int index_of(std::nullptr_t)
{
	for (int i = 0; i < FF_VOLUMES; i++)
		if (file_systems[i] == nullptr) return i;
	return -1;
}

FileSystem::FileSystem(cstr devname) throws // ctor
{
	trace(__func__);

	if (strlen(devname) >= sizeof(name)) throw NAME_TOO_LONG;
	memcpy(name, devname, sizeof(name));
}

FileSystem::~FileSystem() noexcept
{
	trace(__func__);

	delete[] workdir;

	int idx = index_of(this);
	if (idx >= 0) file_systems[idx] = nullptr;
}

void FileSystem::makeFS(BlockDevicePtr bdev, cstr type) throws // static
{
	trace(__func__);

	type = lowerstr(type);
	if (startswith(type, "fat"))
	{
		int idx = index_of(nullptr); // FatFS needs a slot, even if not mounted
		if (idx < 0) throw NO_MOUNTPOINT_FREE;
		FatFS::mkfs(bdev, idx, type);
	}
	else throw UNKNOWN_FILESYSTEM;
}

FileSystemPtr FileSystem::mount(cstr devicename, BlockDevicePtr bdev) throws // static
{
	trace(__func__);

	int idx = index_of(devicename);
	if (idx >= 0) throw DEVICE_IN_USE;

	idx = index_of(nullptr);
	if (idx < 0) throw NO_MOUNTPOINT_FREE;

	// try to mount with all FileSystems we know:
	// (not that many, right now :-)
	FileSystemPtr fs = new FatFS(devicename, bdev, idx);

	if (fs->mount()) return file_systems[idx] = fs;
	else throw UNKNOWN_FILESYSTEM;
}

FileSystemPtr FileSystem::mount(cstr devicename) throws // static
{
	trace(__func__);

	int idx = index_of(devicename);
	if (idx >= 0) return file_systems[idx];

	idx = index_of(nullptr);
	if (idx < 0) throw NO_MOUNTPOINT_FREE;


	FileSystem* fs = nullptr;
	if (lceq(devicename, "rsrc")) fs = RsrcFS::getInstance();
#ifdef PICO_DEFAULT_SPI
	else if (lceq(devicename, "sdcard")) { fs = new FatFS(devicename, SDCard::defaultInstance(), idx); }
#endif
	else throw UNKNOWN_DEVICE;

	if (fs->mount()) return file_systems[idx] = fs;
	else throw UNKNOWN_FILESYSTEM;
}

void FileSystem::setWorkDir(cstr path)
{
	trace(__func__);

	path = makeAbsolutePath(path);
	delete[] workdir;
	workdir = nullptr;

	DirectoryPtr p = openDir(path); // does it mount?
	workdir		   = newcopy(path);
}

cstr FileSystem::makeAbsolutePath(cstr path)
{
	trace(__func__);

	if (path[0] == '/') return path;
	if (workdir) return catstr(workdir, "/", path);
	else return catstr("/", path);
}

DirectoryPtr openDir(cstr path) throws
{
	trace(__func__);

	ptr dp = strchr(path, ':');
	if (!dp) throw PATH_WITHOUT_DEVICE;

	cstr devname = substr(path, dp);
	return FileSystem::mount(devname)->openDir(dp + 1);
}

FilePtr openFile(cstr path, FileOpenMode flags) throws
{
	trace(__func__);

	ptr dp = strchr(path, ':');
	if (!dp) throw PATH_WITHOUT_DEVICE;

	cstr devname = substr(path, dp);
	return FileSystem::mount(devname)->openFile(dp + 1, flags);
}


} // namespace kio::Devices

/*








































*/
