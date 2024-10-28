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

static FileSystem* file_systems[FF_VOLUMES] = {}; // all NULL

static constexpr char UNKNOWN_FILESYSTEM[] = "unknown file system";
static constexpr char UNKNOWN_DEVICE[]	   = "unknown device";
static constexpr char DEVICE_IN_USE[]	   = "device in use";
static constexpr char NO_MOUNTPOINT_FREE[] = "no mountpoint free";
static constexpr char NO_WORKING_DEVICE[]  = "no working device";
static constexpr char NAME_TOO_LONG[]	   = "name too long";


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

	assert(devicename);
	debugstr("FileSystem::mount: \"%s\"\n", devicename);

	cptr p = strchr(devicename, ':');
	if (p) devicename = substr(devicename, p);

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

static cstr makeCanonicalPath(cstr path)
{
	// eliminate "//", "/." and "/.."

	for (int i = 0; path[i] != 0; i++)
	{
		if (path[i] != '/') continue;
		else if (path[i + 1] == '/') path = i == 0 ? path + 1 : catstr(substr(path, path + i), path + i + 1);
		else if (path[i + 1] != '.') continue;
		else if (path[i + 2] == 0) return i == 0 ? "/" : substr(path, path + i);
		else if (path[i + 2] == '/') path = catstr(substr(path, path + i), path + i + 2);
		else if (path[i + 2] != '.') continue;
		if (path[i + 3] == 0)
		{
			if (i == 0) return "/";
			while (path[--i] != '/') {}
			return i == 0 ? "/" : substr(path, path + i);
		}
		else if (path[i + 3] != '/') continue;
		else if (i == 0) path = path + 3;
		else
		{
			int j = i;
			while (path[--i] != '/') {}
			path = catstr(substr(path, path + i), path + j + 3);
		}
		i--;
	}
	return path[0] != 0 ? path : "/";
}

cstr FileSystem::makeAbsolutePath(cstr path)
{
	trace(__func__);

	if (path[0] == '/') return makeCanonicalPath(path);
	if (path[0] == 0) return getWorkDir();
	else return makeCanonicalPath(catstr(getWorkDir(), "/", path));
}

// current working device
static FileSystemPtr cwd;

DirectoryPtr openDir(cstr path) throws
{
	trace(__func__);

	ptr dp = strchr(path, ':');
	if (!dp)
	{
		if (cwd) return cwd->openDir(path);
		else throw NO_WORKING_DEVICE;
	}

	cstr devname = substr(path, dp);
	return FileSystem::mount(devname)->openDir(dp + 1);
}

FilePtr openFile(cstr path, FileOpenMode flags) throws
{
	trace(__func__);

	ptr dp = strchr(path, ':');
	if (!dp)
	{
		if (cwd) return cwd->openFile(path);
		else throw NO_WORKING_DEVICE;
	}

	cstr devname = substr(path, dp);
	return FileSystem::mount(devname)->openFile(dp + 1, flags);
}


FileSystemPtr getWorkDevice()
{
	return cwd; //
}

void setWorkDevice(FileSystemPtr fs)
{
	cwd = std::move(fs); //
}

FileSystemPtr getDevice(cstr name)
{
	int i = index_of(name);
	return i >= 0 ? file_systems[i] : nullptr;
}

cstr getWorkDir()
{
	// "A:/", "A:/foo"
	return cwd ? catstr(cwd->name, ":", cwd->getWorkDir()) : nullptr;
}

void setWorkDir(cstr path)
{
	trace(__func__);

	// "A:", "A:/foo", "A:foo", "/foo", "foo"
	if (!path || !*path) return;

	if (ptr dp = strchr(path, ':'))
	{
		cwd	 = FileSystem::mount(substr(path, dp));
		path = dp + 1;
	}

	if (cwd) cwd->setWorkDir(path);
	else throw NO_WORKING_DEVICE;
}

cstr makeAbsolutePath(cstr path)
{
	ptr dp = strchr(path, ':');
	if (dp) return FileSystem::mount(substr(path, dp))->makeAbsolutePath(dp + 1);
	else if (cwd) return cwd->makeAbsolutePath(path);
	else throw NO_WORKING_DEVICE;
}

FileSystemPtr mount(cstr devicename)
{
	// "rsrc:", "sdcard:"
	return FileSystem::mount(devicename);
}

void unmount(FileSystemPtr fs)
{
	if (fs == cwd) cwd = nullptr;
}

extern void unmountAll()
{
	cwd = nullptr; //
}

} // namespace kio::Devices

/*








































*/
