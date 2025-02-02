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
#include "cdefs.h"
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

static int index_of(cstr name) noexcept
{
	for (int i = 0; i < FF_VOLUMES; i++)
	{
		FileSystem* fs = file_systems[i];
		if (fs && lceq(name, fs->name)) return i;
	}
	return -1;
}

int index_of(FileSystem* fs) noexcept
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

FileSystemPtr mount(cstr name) throws
{
	// mount the FileSystem on the well-known BlockDevice with the given name.
	// returns the already mounted FS if it is mounted.

	// if called with a fullpath:
	if (cptr dp = strchr(name, ':')) name = substr(name, dp);

	trace("FS::mount(name)");
	debugstr("FS::mount: \"%s\"\n", name);
	assert(name && *name);

	int idx = index_of(name);
	if (idx >= 0) return file_systems[idx];

	if (lceq(name, "rsrc")) { return new RsrcFS(name); }
#ifdef PICO_DEFAULT_SPI
	if (lceq(name, "sdcard")) return new FatFS(name, SDCard::defaultInstance());
#endif
#if defined FLASH_BLOCKDEVICE && FLASH_BLOCKDEVICE
	if (lceq(name, "flash"))
	{
		uint32 size = FLASH_BLOCKDEVICE;
		if constexpr (prefs_size) size = Preferences().read<uint32>(tag_flashdisk_size, size);
		return new FatFS(name, new QspiFlashDevice<9>(size));
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

constexpr static cptr find_dp(cstr p) noexcept
{
	while (is_alphanumeric(*p)) p++;
	return *p == ':' ? p : nullptr;
}

constexpr bool is_canonical_fullpath(cstr path, FileSystem* fs = nullptr) noexcept
{
	// check whether path is a canonical full path
	// and whether it refers to the FileSystem fs, if supplied.

	cptr dp = find_dp(path);
	if (!dp) return false;									 // no dev
	if (size_t(dp - path) >= sizeof(fs->name)) return false; // ill. dev name
	if (dp[1] != '/') return false;							 // no absolute path

	if (fs) // compare fs name:
		for (cptr p = path, q = fs->name;; p++, q++)
			if (to_lower(*p) != to_lower(*q)) return *p != ':' || *q != 0;

	if (dp[2] == 0) return true; // odd man out: "dev:/" ends with '/'

	for (cptr p = dp + 1; *p; p++) // check whether canonical:
	{
		if (p[0] != '/') continue;
		if (p[1] == '/' || p[1] == 0) return false;
		if (p[1] != '.') continue;
		if (p[2] == 0 || p[2] == '/') return false;
		if (p[2] != '.') continue;
		if (p[3] == 0 || p[3] == '/') return false;
	}
	return true;
}

static_assert(is_canonical_fullpath("foo:/"));
static_assert(!is_canonical_fullpath("foo:"));
static_assert(is_canonical_fullpath("foo:/bar"));
static_assert(!is_canonical_fullpath("foo:/bar/"));
static_assert(is_canonical_fullpath("foo:/*"));
static_assert(!is_canonical_fullpath("foo:/*/"));
static_assert(!is_canonical_fullpath("foo:/bar//baz"));
static_assert(!is_canonical_fullpath("foo:/bar/../baz"));
static_assert(!is_canonical_fullpath("foo:/bar/./baz"));
static_assert(is_canonical_fullpath("foo:/bar/.boo/baz"));
static_assert(is_canonical_fullpath("foo:/bar/.../baz"));

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

	TempMemSave _;
	path = makeFullPath(path);
	return mount(path)->openDir(path);
}

FilePtr openFile(cstr path, FileOpenMode flags) throws
{
	trace("openFile");

	TempMemSave _;
	path = makeFullPath(path);
	return mount(path)->openFile(path, flags);
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

	TempMemSave _;
	path = makeFullPath(path);
	cwd	 = mount(path);
	cwd->setWorkDir(path);
}

FileType getFileType(cstr path) noexcept
{
	trace("getFileType");
	assert(path);

	TempMemSave _;
	path = makeFullPath(path);
	return mount(path)->getFileType(path);
}

void makeDir(cstr path) throws
{
	trace("makeDir");
	assert(path);

	TempMemSave _;
	path = makeFullPath(path);
	mount(path)->makeDir(path);
}

static inline bool is_rootdir(cstr path) { return strchr(path, ':')[2] == 0; }

static void remove_dir(FileSystem* fs, cstr path, cstr pattern = "") throws
{
	// remove 'path/pattern'
	// remove dir 'path' if last component <=> pattern == ""
	// remove file 'path' if last component else throw NOT_A_DIR or ignore spurious dirpath pattern match

	trace(__func__);
	assert(fs);
	assert(path);
	assert(pattern);
	debugstr("remove_dir \"%s\" w/pattern = \"%s\"\n", path, pattern);

	TempMemSave _;
	if (!isaDirectory(path)) throw DIRECTORY_NOT_FOUND;

	cptr sep		= strchr(pattern, '/');
	cstr subpattern = sep ? sep + 1 : "";
	if (sep) pattern = substr(pattern, sep);

	if (*pattern == 0 || (strchr(pattern, '*') || strchr(pattern, '?')))
	{
		// remove path						-> remove path
		// remove path/filename				-> remove file
		// remove path/dirname				-> remove dir
		// remove path/dirname/subpattern	-> remove subpattern

		DirectoryPtr dir = openDir(path);
		while (FileInfo finfo = dir->next(pattern))
		{
			cstr filepath = catstr(path, &"/"[is_rootdir(path)], finfo.fname);
			if (finfo.ftype == FileType::DirectoryFile) remove_dir(fs, filepath, subpattern);
			else if (!sep) fs->remove(filepath);
			// else ignore spurious dirname match
		}
		if (*pattern == 0) fs->remove(path);
	}
	else // no wildcard
	{
		// remove dir/filename				-> remove file
		// remove dir/dirname				-> remove dir
		// remove dir/dirname/subpattern	-> remove subpattern

		cstr filepath = catstr(path, &"/"[is_rootdir(path)], pattern);
		if (isaDirectory(filepath)) remove_dir(fs, filepath, subpattern);
		else if (sep) throw REGULAR_FILE;
		fs->remove(filepath);
	}
}

void remove(cstr path) throws
{
	trace("remove");
	assert(path);
	debugstr("remove \"%s\"\n", path);

	TempMemSave _;
	path			 = makeFullPath(path);
	FileSystemPtr fs = mount(path);

	cptr p1 = strchr(path, '*');
	cptr p2 = strchr(path, '?');

	if (p1 || p2) // rm dir/*
	{
		p1 = !p1 ? p2 : !p2 ? p1 : min(p1, p2);	   // p1 -> first wildcard
		p2 = rfind(path, p1, '/');				   // p2 -> last '/' before wildcard
		assert(p2 && *p2 == '/');				   //
		path = substr(path, p2 + (p2[-1] == ':')); // rootdir => keep final "/"
		remove_dir(fs, path, p2 + 1);
	}
	else if (isaDirectory(path)) remove_dir(fs, path); // rm dir
	else fs->remove(path);							   // rm file
}

static void copy_file(cstr q, cstr z) throws
{
	trace(__func__);
	assert(q);
	assert(z);
	debugstr("copy_file \"%s\" -> \"%s\"\n", q, z);

	FilePtr qf	 = openFile(q);
	FilePtr zf	 = openFile(z, Devices::WRITE);
	SIZE	size = qf->getSize();
	char	bu[512];
	while (size)
	{
		uint n = min(size, 512u);
		qf->read(bu, n);
		zf->write(bu, n);
		size -= n;
	}
	zf->close();
}

static void copy_dir(cstr indir, cstr outdir, cstr pattern = "*") throws
{
	trace(__func__);
	assert(indir);
	assert(outdir);
	assert(pattern);
	debugstr("copy_dir \"%s\" -> \"%s\" w/pattern = \"%s\"\n", indir, outdir, pattern);

	TempMemSave _;
	if (!isaDirectory(indir)) throw DIRECTORY_NOT_FOUND;
	makeDir(outdir);

	cptr p		  = strchr(pattern, '/');
	cstr filename = p ? substr(pattern, p) : pattern; // potential filename before "/"
	pattern		  = p ? p + 1 : "*";				  // remaining pattern after "/"
	bool wild	  = strchr(filename, '*') || strchr(filename, '?');

	if (!wild)
	{
		cstr infile	 = catstr(indir, &"/"[is_rootdir(indir)], filename);   // dir or file
		cstr outfile = catstr(outdir, &"/"[is_rootdir(outdir)], filename); // dir or file

		if (p || isaDirectory(infile)) return copy_dir(infile, outfile, pattern);
		else return copy_file(infile, outfile);
	}

	DirectoryPtr dir = openDir(indir);
	while (FileInfo finfo = dir->next(filename))
	{
		assert(finfo.ftype == FileType::RegularFile || finfo.ftype == FileType::DirectoryFile);

		TempMemSave _;
		cstr		infile	= catstr(indir, &"/"[is_rootdir(indir)], finfo.fname);
		cstr		outfile = catstr(outdir, &"/"[is_rootdir(outdir)], finfo.fname);

		if (finfo.ftype == FileType::DirectoryFile) copy_dir(infile, outfile, pattern);
		else if (!p) copy_file(infile, outfile);
	}
}

void copy(cstr q, cstr z) throws
{
	trace(__func__);
	assert(q);
	assert(z);
	debugstr("copy \"%s\" -> \"%s\"\n", q, z);

	//	cp dir/path/file  dir/path/old_file		overwrite old_file
	//	cp dir/path/file  dir/path/new_file		create new_file
	//	cp dir/path/file  dir/path/zdir			create new file in zdir

	//	cp dir/path/qdir  dir/path/zdir			copy qdir into zdir
	//	cp dir/path/*/..  dir/path/zdir			copy pattern into zdir

	TempMemSave _;
	q = makeFullPath(q);
	z = makeFullPath(z);

	FileSystemPtr _q = mount(q); // retain FS
	FileSystemPtr _z = mount(z); // retain FS

	if (!isaDirectory(z))		// cp file -> old_file|new_file
		return copy_file(q, z); // may overwrite old file

	cptr p1 = strchr(q, '*');
	cptr p2 = strchr(q, '?');

	if (p1 || p2) // cp dir/* -> dir
	{
		p1 = !p1 ? p2 : !p2 ? p1 : min(p1, p2); // p1 -> first wildcard
		p2 = rfind(q, p1, '/');					// p2 -> last '/' before wildcard
		assert(p2 && *p2 == '/');				//
		q = substr(q, p2 + (p2[-1] == ':'));	// rootdir => keep final "/"
		return copy_dir(q, z, p2 + 1);
	}

	z = catstr(z, &"/"[is_rootdir(z)], filename_from_path(q));
	if (isaDirectory(q)) return copy_dir(q, z); // cp dir -> dir
	else return copy_file(q, z);				// cp file -> dir
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
