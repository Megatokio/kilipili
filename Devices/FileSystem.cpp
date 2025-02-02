// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FileSystem.h"
#include "FatFS.h"
#include "File.h"
#include "Preferences.h"
#include "QspiFlashDevice.h"
#include "RsrcFS.h"
#include "SDCard.h"
#include "Trace.h"
#include "cstrings.h"
#include "ff15/source/ffconf.h"

#if defined FLASH_PREFERENCES && FLASH_PREFERENCES
static constexpr uint prefs_size = FLASH_PREFERENCES;
#else
static constexpr uint prefs_size = 0;
#endif

namespace kio::Devices
{

static FileSystem* file_systems[FF_VOLUMES] = {}; // all NULL

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
	debugstr("FS::makeFS(bdev,type)\n");

	type = lowerstr(type);
	if (startswith(type, "fat"))
	{
		int idx = index_of(nullptr); // FatFS needs a slot, even if not mounted
		if (idx < 0) throw NO_MOUNTPOINT_FREE;
		FatFS::mkfs(bdev, idx, type);
	}
	else throw UNKNOWN_FILESYSTEM;
}

void makeFS(cstr devicename, cstr type) throws
{
	trace(__func__);
	debugstr("FS::makeFS(%s,%s)\n", devicename, type);
	assert(devicename && *devicename && !strchr(devicename, ':'));

	if (index_of(devicename) >= 0) throw DEVICE_IN_USE;

#if defined FLASH_BLOCKDEVICE && FLASH_BLOCKDEVICE
	if (lceq(devicename, "flash"))
	{
		makeFS(new QspiFlashDevice<9>(FLASH_BLOCKDEVICE), type);
		if constexpr (prefs_size) Preferences().write(tag_flashdisk_size, uint32(FLASH_BLOCKDEVICE));
		return;
	}
#endif
#ifdef PICO_DEFAULT_SPI
	if (lceq(devicename, "sdcard")) { makeFS(SDCard::defaultInstance(), type); }
#endif
	if (lceq(devicename, "rsrc")) throw NOT_WRITABLE;
	else throw UNKNOWN_DEVICE;
}

FileSystemPtr mount(cstr devicename, BlockDevicePtr bdev) throws
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

FileSystemPtr mount(cstr devicename) throws
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
#if defined FLASH_BLOCKDEVICE && FLASH_BLOCKDEVICE
	if (lceq(devicename, "flash"))
	{
		uint32 size = FLASH_BLOCKDEVICE;
		if constexpr (prefs_size) size = Preferences().read<uint32>(tag_flashdisk_size, size);
		return new FatFS(devicename, new QspiFlashDevice<9>(size));
	}
#endif
	throw UNKNOWN_DEVICE;
}

void unmount(FileSystem* fs)
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

static cptr find_dp(cstr p) noexcept
{
	while (is_alphanumeric(*p)) p++;
	return *p == ':' ? p : nullptr;
}

bool is_canonical_fullpath(cstr path, FileSystem* fs = nullptr) noexcept
{
	// check whether path is a canonical full path
	// and whether it refers to the FileSystem fs, if supplied.

	cptr dp = find_dp(path);
	if (!dp) return false;							 // no dev
	if (dp >= path + sizeof(fs->name)) return false; // ill. dev name
	if (dp[1] != '/') return false;					 // no absolute path

	if (fs) // compare fs name:
		for (cptr p = path, q = fs->name;; p++, q++)
			if (to_lower(*p) != to_lower(*q)) return *p != ':' || *q != 0;

	if (dp[2] == 0) return true; // odd man out: "dev:/" ends with '/'

	for (cptr p = dp + 1;; p++) // check whether canonical:
	{
		if (*p == 0) return true;
		if (*p == '/' && (p[1] == '/' || p[1] == '.' || p[1] == 0)) return false;
	}
}

static cstr make_fullpath(cstr path, FileSystem* fs)
{
	// make canonical full absolute path
	// the path or device are not tested and may be void

	// prepend device and workdir
	// eliminate "//", "/.", "/.." and trailing '/'

	// fs != nullptr <=> called from this fs
	//	 use fs->name instead of device from path, if present (but should be same)
	//	 use fs->workdir for relative path
	// fs == nullptr <=> called from a global scope
	//	 use cwd->name if no device in path
	//	 use cwd->workdir for relative path

	// in:
	//   "dev:/abs_path/to/dir/or/file"
	//   "dev:rel_path/to/dir/or/file"
	//   "dev:/"	= root dir of device
	//   "dev:" 	= workdir dir of device
	//   "/abs_path/to/dir/or/file"
	//   "rel_path/to/dir/or/file"
	//   "/"		= root dir of current device
	//   "" 		= workdir dir of current device

	// out:
	//	"dev:/abs_path/to/dir/or/file"
	//	"dev:/"		= exception: root dir path ends on '/'

	trace("FS::makeFullPath");
	assert(path);

	if (is_canonical_fullpath(path, fs)) return path;

	if (!fs) fs = cwd;
	cptr dp = find_dp(path);
	if (!dp && !fs) throw NO_WORKING_DEVICE;
	ptr z;

	// prepend device and cwd
	// append '/' to simpify tests:
	if (dp)
	{
		if (dp[1] == '/') z = catstr(path, "/");
		else
		{
			cstr dev = substr(path, dp);
			int	 i	 = index_of(dev);
			if (i < 0) z = catstr(dev, ":/", dp + 1, "/");
			else if (cstr wd = file_systems[i]->workdir) z = catstr(wd, "/", dp + 1, "/");
			else z = catstr(file_systems[i]->name, ":/", dp + 1, "/");
		}
	}
	else // no dp:
	{
		if (path[0] == '/') z = catstr(fs->name, ":", path, "/");
		else if (fs->workdir) z = catstr(fs->workdir, "/", path, "/");
		else z = catstr(fs->name, ":/", path, "/");
	}

	cptr q = z;
	path   = z;

	// eliminate "//", "/.", "/.."
	while (*q)
	{
		if (*q == '/')
		{
			if (q[1] == '/') // "//"
			{
				q++;
				continue;
			}
			if (q[1] == '.' && q[2] == '/') // "/."
			{
				q += 2;
				continue;
			}
			if (q[1] == '.' && q[2] == '.' && q[3] == '/') // "/.."
			{
				q += 3;
				if (z[-1] == ':') continue; // "dev:/../"
				while (*--z != '/') {}		// "dev:/dir/../"
				continue;
			}
		}

		*z++ = *q++;
	}

	assert(z[-1] == '/');

	if (z[-2] != ':') z--;
	*z = 0;
	return path;
}

cstr makeFullPath(cstr path)
{
	// make canonical full absolute path
	// the path or device are not tested and may be void

	return make_fullpath(path, nullptr);
}

DirectoryPtr openDir(cstr path) throws
{
	trace("openDir");

	cptr dp = find_dp(path);
	if (dp) return mount(substr(path, dp))->openDir(path);
	else if (cwd) return cwd->openDir(path);
	else throw NO_WORKING_DEVICE;
}

FilePtr openFile(cstr path, FileOpenMode flags) throws
{
	trace("openFile");

	cptr dp = find_dp(path);
	if (dp) return mount(substr(path, dp))->openFile(path, flags);
	else if (cwd) return cwd->openFile(path);
	else throw NO_WORKING_DEVICE;
}

FileSystemPtr getDevice(cstr name)
{
	int i = index_of(name);
	return i >= 0 ? file_systems[i] : nullptr;
}

FileSystemPtr getWorkDevice()
{
	return cwd; //
}

void setWorkDevice(FileSystemPtr fs)
{
	cwd = std::move(fs); //
}

cstr getWorkDir()
{
	return cwd ? cwd->getWorkDir() : nullptr; //
}

void setWorkDir(cstr path)
{
	// "A:", "A:/foo", "A:foo", "/foo", "foo"

	trace("setWorkDir");
	assert(path);

	cptr dp = find_dp(path);
	if (dp) cwd = mount(substr(path, dp));
	if (cwd) cwd->setWorkDir(path);
	else throw NO_WORKING_DEVICE;
}

FileType getFileType(cstr path) noexcept
{
	trace("getFileType");
	assert(path);

	cptr dp = find_dp(path);
	if (!dp && !cwd) return FileType::NoFile;
	FileSystemPtr fs = dp ? mount(substr(path, dp)) : cwd;
	return fs->getFileType(path);
}

void remove(cstr path) throws
{
	trace("remove");
	assert(path);

	cptr dp = find_dp(path);
	if (!dp && !cwd) throw NO_WORKING_DEVICE;
	FileSystemPtr fs = dp ? mount(substr(path, dp)) : cwd;
	fs->remove(path);
}

void makeDir(cstr path) throws
{
	trace("makeDir");
	assert(path);

	cptr dp = find_dp(path);
	if (!dp && !cwd) throw NO_WORKING_DEVICE;
	FileSystemPtr fs = dp ? mount(substr(path, dp)) : cwd;
	fs->makeDir(path);
}


// ===========================================================

FileSystem::FileSystem(cstr devname) throws // ctor
{
	trace("FS::ctor");
	assert(devname);

	if (strlen(devname) >= sizeof(name)) throw NAME_TOO_LONG;
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

cstr FileSystem::makeFullPath(cstr path)
{
	return make_fullpath(path, this); //
}

cstr FileSystem::getWorkDir() const noexcept
{
	return workdir ? workdir : catstr(name, ":/"); //
}

void FileSystem::setWorkDir(cstr path)
{
	trace("FS::setWorkDir");

	path = make_fullpath(path, this);
	if (!isaDirectory(path)) throw DIRECTORY_NOT_FOUND;

	delete[] workdir;
	workdir = nullptr;
	workdir = newcopy(path);
}

} // namespace kio::Devices

/*








































*/
