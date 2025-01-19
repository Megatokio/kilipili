// Copyright (c) 2023 - 2025 kio@little-bat.de
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

// current working device
static FileSystemPtr cwd;


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

int index_of(FileSystem* fs)
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

void makeFS(BlockDevicePtr bdev, cstr type) throws // static
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

FileSystemPtr mount(cstr devicename, BlockDevicePtr bdev) throws // static
{
	// discover the FileSystem on the BlockDevice and mount it with the given name.
	// throws if a FS with that name is already mounted.

	trace("FS::mount(name,bdev)");
	debugstr("FS::mount: \"%s\", bdev\n", devicename);

	assert(devicename && *devicename);
	assert(bdev);

	// TODO: we could dynamic_cast the already mounted FS and check whether it's on the same bdev:
	if (index_of(devicename) >= 0) throw DEVICE_IN_USE;

	// check if the device is readable by reading some random bytes:
	char bu[8];
	bdev->readData(0, bu, 8); // throws

	// try to mount with all FileSystems we know:
	// (not that many, right now :-)
	for (uint i = 0;; i++)
	{
		try
		{
			if (i == 0) return new FatFS(devicename, bdev);
			if (i == 1) {} //etc.
			break;
		}
		catch (...)
		{}
	}
	throw UNKNOWN_FILESYSTEM;
}

FileSystemPtr mount(cstr devicename) throws // static
{
	// mount the FileSystem on the well-known BlockDevice with the given name.
	// returns the already mounted FS if it is mounted.

	trace("FS::mount(name)");
	debugstr("FS::mount: \"%s\"\n", devicename);

	assert(devicename && *devicename);

	int idx = index_of(devicename);
	if (idx >= 0) return file_systems[idx];

	if (lceq(devicename, "rsrc")) { return new RsrcFS(devicename); }
#ifdef PICO_DEFAULT_SPI
	if (lceq(devicename, "sdcard")) return new FatFS(devicename, SDCard::defaultInstance());
#endif
	throw UNKNOWN_DEVICE;
}

void unmount(FileSystemPtr fs)
{
	if (fs == cwd) cwd = nullptr;
}

void unmountAll()
{
	cwd = nullptr;

	if constexpr (debug)
		for (uint i = 0; i < NELEM(file_systems); i++)
		{
			if (FileSystem* fs = file_systems[i])
				printf("unmountAll: \"%s\" still mounted (rc=%i)\n", fs->name, fs->rc);
		}
}

static cstr makeCanonicalPath(cstr path)
{
	// eliminate "//", "/." and "/.."

	for (int i = 0; path[i] != 0; i++)
	{
		if (path[i] != '/') continue;
		else if (path[i + 1] == '/') path = catstr(substr(path, path + i), path + i + 1);
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

DirectoryPtr openDir(cstr path) throws
{
	trace("FS::openDir");

	ptr dp = strchr(path, ':');
	if (!dp)
	{
		if (cwd) return cwd->openDir(path);
		else throw NO_WORKING_DEVICE;
	}

	cstr devname = substr(path, dp);
	return mount(devname)->openDir(dp + 1);
}

FilePtr openFile(cstr path, FileOpenMode flags) throws
{
	trace("FS::openFile");

	ptr dp = strchr(path, ':');
	if (!dp)
	{
		if (cwd) return cwd->openFile(path);
		else throw NO_WORKING_DEVICE;
	}

	cstr devname = substr(path, dp);
	return mount(devname)->openFile(dp + 1, flags);
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
	trace("getWorkDir");

	// "A:/", "A:/foo"
	return cwd ? catstr(cwd->name, ":", cwd->getWorkDir()) : nullptr;
}

void setWorkDir(cstr path)
{
	trace("setWorkDir");

	// "A:", "A:/foo", "A:foo", "/foo", "foo"
	if (!path || !*path) return;

	if (ptr dp = strchr(path, ':'))
	{
		cwd	 = mount(substr(path, dp));
		path = dp + 1;
	}

	if (cwd) cwd->setWorkDir(path);
	else throw NO_WORKING_DEVICE;
}

cstr makeAbsolutePath(cstr path)
{
	ptr dp = strchr(path, ':');
	if (dp) return mount(substr(path, dp))->makeAbsolutePath(dp + 1);
	else if (cwd) return cwd->makeAbsolutePath(path);
	else throw NO_WORKING_DEVICE;
}


// ===========================================================

FileSystem::FileSystem(cstr devname) throws // ctor
{
	trace("FS::ctor");

	if (strlen(devname) + 1 >= sizeof(name)) throw NAME_TOO_LONG;
	strcpy(name, devname);

	int idx = index_of(devname);
	if (idx >= 0) throw DEVICE_IN_USE;

	idx = index_of(nullptr);
	if (idx < 0) throw NO_MOUNTPOINT_FREE;

	file_systems[idx] = this;
}

FileSystem::~FileSystem() noexcept
{
	trace("FS::dtor");

	delete[] workdir;

	int idx = index_of(this);
	if (idx >= 0) file_systems[idx] = nullptr;
}

cstr FileSystem::makeAbsolutePath(cstr path)
{
	trace("FS::makeAbsolutePath");

	if (path[0] == '/') return makeCanonicalPath(path);
	if (path[0] == 0) return getWorkDir();
	return makeCanonicalPath(catstr(getWorkDir(), "/", path));
}

void FileSystem::setWorkDir(cstr path)
{
	trace("FS::setWorkDir");

	path = makeAbsolutePath(path);
	(void)openDir(path); // does it mount?

	delete[] workdir;
	workdir = nullptr;
	workdir = newcopy(path);
}


} // namespace kio::Devices

/*








































*/
