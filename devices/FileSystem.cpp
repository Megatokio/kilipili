// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FileSystem.h"
#include "FatFS.h"
#include "File.h"
#include "FlashBlockDevice.h"
#include "Preferences.h"
#include "RsrcFS.h"
#include "common/Dispatcher.h"
#include "common/Logger.h"
#include "common/cdefs.h"
#include "common/cstrings.h"
#include "common/trace.h"
#include "ff15/source/ffconf.h"
#ifdef PICO
  #include "SDCard.h"
#else
  #include "StdFS.h"
#endif


#if defined FLASH_PREFERENCES && FLASH_PREFERENCES
static constexpr uint prefs_size = FLASH_PREFERENCES;
#else
static constexpr uint prefs_size = 0;
#endif

namespace kilipili
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
		return;
	}

	throw UNKNOWN_FILESYSTEM;
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
		makeFS(new FlashBlockDevice<9>(FLASH_BLOCKDEVICE), type);
		if constexpr (prefs_size) Preferences().write(tag_flashdisk_size, uint32(FLASH_BLOCKDEVICE));
		return;
	}
#endif
#if defined SDCARD_SPI
	if (lceq(devicename, "sdcard")) { makeFS(SDCard::getInstance(0), type); }
#endif
#if defined SDCARD_SPI && defined SDCARD_SPI_CS2_PIN
	if (lceq(devicename, "sdcard2")) { makeFS(SDCard::getInstance(1), type); }
#endif
	if (lceq(devicename, "rsrc")) throw NOT_WRITABLE;
	else throw UNKNOWN_DEVICE;
}

FileSystemPtr mountFS(cstr devicename, BlockDevicePtr bdev) throws
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

FileSystemPtr getFS(cstr path) throws
{
	assert(path);

#ifndef PICO
	if (startswith(path, "//")) return mountFS("disk");
#endif
	if (cptr dp = strchr(path, ':')) return mountFS(substr(path, dp));
	if (cwd) return cwd;
	else throw NO_WORKING_DEVICE;
}

FileSystemPtr mountFS(cstr name) throws
{
	// mount the FileSystem on the well-known BlockDevice with the given name.
	// returns the already mounted FS if it is mounted.

	assert(name && *name);

	int idx = index_of(name);
	if (idx >= 0) return file_systems[idx];

	trace("FS::mount(name)");
	debugstr("FS::mount: \"%s\"\n", name);

	if (lceq(name, "rsrc")) { return new RsrcFS(name); }
#if defined SDCARD_SPI
	if (lceq(name, "sdcard")) return new FatFS(name, SDCard::getInstance(0));
#endif
#if defined SDCARD_SPI && defined SDCARD_SPI_CS2_PIN
	if (lceq(name, "sdcard2")) return new FatFS(name, SDCard::getInstance(1));
#endif
#if defined FLASH_BLOCKDEVICE && FLASH_BLOCKDEVICE
	if (lceq(name, "flash"))
	{
		uint32 size = FLASH_BLOCKDEVICE;
		if constexpr (prefs_size) size = Preferences().read<uint32>(tag_flashdisk_size, size);
		return new FatFS(name, new FlashBlockDevice<9>(size));
	}
#endif
#ifndef PICO
	// last chance: it's "disk" or a device in "/mnt" or "/media"
	return new StdFS(name);
#else
	throw UNKNOWN_DEVICE;
#endif
}

void unmountFS(FileSystem* fs)
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

Array<cstr> getListOfDevices()
{
	Array<cstr> list;
	list << "rsrc";
#if defined FLASH_BLOCKDEVICE && FLASH_BLOCKDEVICE
	list << "flash";
#endif
#if defined SDCARD_SPI
	list << "sdcard";
#endif
#if defined SDCARD_SPI && defined SDCARD_SPI_CS2_PIN
	list << "sdcard2";
#endif
#ifndef PICO
	list.append(StdFS::getListOfDevices());
#endif
	return list;
}

constexpr static __unused cptr find_wildcard(cptr s, cptr e) noexcept
{
	while (s < e && *s != '*' && *s != '?') s++;
	return s < e ? s : nullptr;
}

constexpr static cptr find_dp(cstr p) noexcept
{
	while (is_alphanumeric(*p)) p++;
	return *p == ':' ? p : nullptr;
}

constexpr bool is_canonical_fullpath(cstr path, bool allow_trailing_slash = false) noexcept
{
	// check whether path is a canonical full path
	// does not check whether path components actually exist

	// --> "device:/path/to/file"
	//	   "device:"

	cptr dp = find_dp(path);
	if (!dp) return false; // no dev

	if (dp[1] != '/') return dp[1] == 0; // "device:" is ok, else a '/' must follow

	for (cptr p = dp + 1; *p; p++) // check whether canonical:
	{
		if (p[0] != '/') continue;
		if (p[1] == 0) return allow_trailing_slash; // "…/"
		if (p[1] == '/') return false;				// "…//…"
		if (p[1] != '.') continue;					// fname
		if (p[2] == 0 || p[2] == '/') return false; // "…/." or "…/./…"
		if (p[2] != '.') continue;					// fname starting with '.'
		if (p[3] == 0 || p[3] == '/') return false; // "…/.." or "…/../…"
	}
	return true;
}

bool FileSystem::is_canonical_fullpath(cstr path, bool allow_trailing_slash) noexcept
{
	assert(path);
	assert(find_dp(path) && eq(substr(path, find_dp(path)), name));
	return kilipili::is_canonical_fullpath(path, allow_trailing_slash);
}

static_assert(!is_canonical_fullpath("foo:/"));
static_assert(is_canonical_fullpath("foo:/", true));
static_assert(is_canonical_fullpath("foo:"));
static_assert(is_canonical_fullpath("foo:/bar"));
static_assert(!is_canonical_fullpath("foo:/bar/"));
static_assert(is_canonical_fullpath("foo:/bar/", true));
static_assert(is_canonical_fullpath("foo:/*"));
static_assert(!is_canonical_fullpath("foo:/*/"));
static_assert(!is_canonical_fullpath("foo:/bar//baz"));
static_assert(!is_canonical_fullpath("foo:/bar/../baz"));
static_assert(!is_canonical_fullpath("foo:/bar/./baz"));
static_assert(is_canonical_fullpath("foo:/bar/.boo/baz"));
static_assert(is_canonical_fullpath("foo:/bar/.../baz"));

static cstr make_fullpath(cstr path, FileSystem* fs, bool keep_trailing_slash)
{
	// make canonical full absolute path
	// the path or device are not tested and may be void

	// use cwd if fs = nullptr
	// prepend device and workdir
	// eliminate "//", "/.", "/.." and trailing '/' (except if allowed)
	// if !PICO then replace "//" at start of path with "disk:/"

	// out:
	//	"dev:"  or  "dev:/path/to/file"
	//	"dev:/" or  "dev:/path/to/dir/"		if keep_trailing_slash

	trace("FS::make_fullpath");
	assert(path);

	if (debug && fs && find_dp(path)) // fs names must match!
	{
		cptr dp = path;
		cptr dn = fs->name;
		while (to_lower(*dp) == to_lower(*dn)) { dn++, dp++; }
		assert(*dp == ':' && *dn == 0);
	}

	if (is_canonical_fullpath(path, keep_trailing_slash)) return path;

	keep_trailing_slash = keep_trailing_slash && lastchar(path) == '/';

	// prepend device and cwd
	// append '/' to simpify tests:
	ptr z;
#ifndef PICO
	if (startswith(path, "//")) z = catstr("disk:", path + 1, "/"); // "disk:/…"
	else
#endif
	{
		if (!fs) fs = cwd;
		cptr dp = find_dp(path);
		if (!dp && !fs) throw NO_WORKING_DEVICE;

		if (dp) // "dev:" "dev:/" "dev:path" "dev:/path"
		{
			z = catstr(substr(path, dp + 1), "/", dp + 1, "/");
		}
		else // no dp: "", "/", "path…" or "/path…"
		{
			if (path[0] == '/') z = catstr(fs->name, ":", path, "/");
			else if (fs->workdir) z = catstr(fs->workdir, "/", path, "/");
			else z = catstr(fs->name, ":/", path, "/");
		}
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
	if (!keep_trailing_slash) z--;
	*z = 0;
	return path;
}

str resolveDirPath(cstr path, bool mkdir)
{
	return getFS(path)->resolveDirPath(path, mkdir); //
}
cstr resolveDirPathWithPattern(cstr path, cstr* fnpattern)
{
	return getFS(path)->resolveDirPathWithPattern(path, fnpattern);
}
cstr resolveFilePath(cstr path, FileType* filetype)
{
	return getFS(path)->resolveFilePath(path, filetype); //
}
cstr resolveFilePath(cstr path, FileType ftype)
{
	return getFS(path)->resolveFilePath(path, ftype); //
}

DirectoryPtr openDir(cstr path) throws
{
	trace("openDir");

	FileSystemPtr fs = getFS(path);
	path			 = fs->resolveFilePath(path, DirectoryFile);
	return fs->openDir(path);
}

FilePtr openFile(cstr path, FileOpenMode flags) throws
{
	trace("openFile");

	FileSystemPtr fs = getFS(path);
	FileType	  ftype;
	path = fs->resolveFilePath(path, &ftype);

	if (ftype == NoFile)
	{
		if (flags & EXIST) throw FILE_NOT_FOUND;
	}
	else if (ftype == RegularFile)
	{
		if (flags & NEW) throw FILE_EXISTS;
	}
	else throw NOT_A_REGULAR_FILE;

	return fs->openFile(path, flags);
}

ADDR getFileSize(cstr path) throws
{
	trace("getFileSize");

	FileSystemPtr fs = getFS(path);
	path			 = fs->resolveDirPath(path, false);
	return fs->getFileSize(path);
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

	cwd	 = getFS(path);
	path = cwd->resolveFilePath(path, DirectoryFile);
	cwd->setWorkDir(path);
}

FileType getFileType(cstr path) noexcept
{
	trace("getFileType");
	assert(path);

	try
	{
		FileSystemPtr fs = getFS(path);
		path			 = fs->resolveDirPath(path, false);
		return fs->getFileType(path);
	}
	catch (Error e)
	{
		logline("%s", e);
	}
	catch (...)
	{
		logline("unknown error [408]");
	}
	return NoFile;
}

void makeDir(cstr path) throws
{
	trace("makeDir");
	assert(path);

	FileSystemPtr fs = getFS(path);
	path			 = fs->resolveDirPath(path, true);
	if (lastchar(path) != '/') fs->makeDir(path);
}

static inline bool is_rootdir(cstr path) { return strchr(path, ':')[1] == 0; }

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

	TODO();

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

		TempMemSave	 _;
		DirectoryPtr dir = openDir(path);
		while (FileInfo finfo = dir->next(pattern))
		{
			cstr filepath = catstr(path, &"/"[is_rootdir(path)], finfo.fname);
			if (finfo.ftype == FileType::DirectoryFile) remove_dir(fs, filepath, subpattern);
			else if (!sep) fs->remove(filepath);
			// else ignore spurious dirname match
			_.purge();
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
		else if (sep) throw NOT_A_DIRECTORY;
		fs->remove(filepath);
	}
}

void removeFile(cstr path) throws
{
	trace("remove");
	assert(path);
	debugstr("remove \"%s\"\n", path);

	TODO();

	TempMemSave _;
	path			 = make_fullpath(path, nullptr, false);
	FileSystemPtr fs = getFS(path);

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
	logline("copy %s", filenamefrompath(q));

	TODO();

	FilePtr qf	 = openFile(q);
	FilePtr zf	 = openFile(z, WRITE);
	SIZE	size = qf->getSize();

	constexpr uint busize = 2 kB;
	if (std::unique_ptr<char> buffer {new (std::nothrow) char[busize]})
	{
		while (size)
		{
			uint n = min(size, busize);
			Dispatcher::run();
			qf->read(buffer.get(), n);
			Dispatcher::run();
			zf->write(buffer.get(), n);
			size -= n;
		}
	}
	else
	{
		while (size)
		{
			Dispatcher::run();
			{
				constexpr uint busize = 512;
				char		   buffer[busize]; // on stack

				uint n = min(size, busize);
				qf->read(buffer, n);
				zf->write(buffer, n);
				size -= n;
			}
		}
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

	TODO();

	TempMemSave _;
	if (!isaDirectory(indir)) throw DIRECTORY_NOT_FOUND;
	makeDir(outdir);
	Dispatcher::run();

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
	else
	{
		TempMemSave	 _;
		DirectoryPtr dir = openDir(indir);
		while (FileInfo finfo = dir->next(filename))
		{
			assert(finfo.ftype == FileType::RegularFile || finfo.ftype == FileType::DirectoryFile);

			cstr infile	 = catstr(indir, &"/"[is_rootdir(indir)], finfo.fname);
			cstr outfile = catstr(outdir, &"/"[is_rootdir(outdir)], finfo.fname);

			if (finfo.ftype == FileType::DirectoryFile) copy_dir(infile, outfile, pattern);
			else if (!p) copy_file(infile, outfile);
			_.purge();
		}
	}
}

void copyFile(cstr q, cstr z) throws
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

	TODO();

	TempMemSave _;
	q = make_fullpath(q, nullptr, false);
	z = make_fullpath(z, nullptr, false);
	Dispatcher::run();

	FileSystemPtr _q = getFS(q); // retain FS
	FileSystemPtr _z = getFS(z); // retain FS
	Dispatcher::run();

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
	strncpy(name, devname, sizeof(name));

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

bool FileSystem::is_resolved_fullpath(cstr path, bool allow_final_slash)
{
	if (!is_canonical_fullpath(path, allow_final_slash)) return false;
	if (find_wildcard(path)) return false;
	return true;
}

cstr FileSystem::makeFullPath(cstr path, bool keep_trailing_slash)
{
	return make_fullpath(path, this, keep_trailing_slash);
}

cstr FileSystem::getWorkDir() const noexcept
{
	return workdir ? workdir : catstr(name, ":"); //
}

void FileSystem::setWorkDir(cstr path)
{
	trace("FS::setWorkDir");
	assert(is_resolved_fullpath(path));

	FileType ft = getFileType(path);
	if (ft != DirectoryFile) throw ft ? NOT_A_DIRECTORY : DIRECTORY_NOT_FOUND;

	delete[] workdir;
	workdir = nullptr;
	workdir = newcopy(path);
}

str FileSystem::resolveDirPath(cstr path, bool mkdir)
{
	// resolve the directory part of a filepath,
	// that is: all components up to the last '/'
	// only actual directory files are considered for matching a pattern.
	// if mkdir=true then the path should better not contain wildcards.

	TempMemSave _;

	str fullpath = const_cast<str>(make_fullpath(path, this, true));
	if (lastchar(fullpath) == ':') return xdupstr(fullpath);
	if (fullpath == path) fullpath = dupstr(fullpath);

	// resolve the directory path:
	for (ptr sep, sep0 = strchr(fullpath, '/'); (sep = strchr(sep0 + 1, '/')); sep0 = sep, *sep = '/')
	{
		*sep		= 0;
		FileType ft = getFileType(fullpath);
		if (ft == DirectoryFile) continue;
		if (ft) throw NOT_A_DIRECTORY;

		str dname = sep0 + 1;
		if (mkdir && !find_wildcard(dname))
		{
			makeDir(path);
			continue;
		}
		// else wildcard => mkdir not allowed! we must find a match or fail!

		*sep0			 = 0;
		DirectoryPtr dir = openDir(fullpath);
		*sep0			 = '/';

		bool has_wc = find_wildcard(dname);
		int	 n_tc	= 0; // total count of matches
		int	 n_wc	= 0; // count of wrong case directory matches
		int	 n_cc	= 0; // count of case correct directory matches

		FileInfo f1;
		while (FileInfo f2 = dir->next(dname)) // search case folding
		{
			n_tc++;
			if (!f2.isaDir()) continue;
			int cc = has_wc ? fnmatch(dname, f2.fname, false) : eq(dname, f2.fname);
			if (cc || n_cc == 0) f1 = f2;
			n_wc += !cc;
			n_cc += cc;
			if (n_cc > 1 || (cc && !has_wc)) break;
		}
		if (n_wc + n_cc == 0) throw n_tc == 0 ? DIRECTORY_NOT_FOUND : NOT_A_DIRECTORY;
		if (n_cc > 1 || (n_cc == 0 && n_wc > 1)) throw AMBIGUOUS_DIRECTORYNAME;

		*sep	 = '/';
		sep0[1]	 = 0;
		fullpath = catstr(fullpath, f1.fname, sep);
		sep		 = strchr(fullpath, 0) - strlen(sep);
		assert(*sep == '/');
	}

	return xdupstr(fullpath);
}

str FileSystem::resolveDirPathWithPattern(cstr path, cstr* pattern)
{
	// resolve the directory part of a filepath,
	// that is: all components up to the last '/'
	// and split of the last part as the pattern (with or without wildcards)

	assert(path);
	assert(pattern);

	// if the filename is on one of "", ".", ".." or "*"
	// then the whole path is the dirpath and the pattern is null:
	cptr dp	   = strchr(path, ':');
	cstr fname = filenamefrompath(dp ? dp + 1 : path);
	if (fname[0] == 0 || eq(fname, "*") || eq(fname, ".") || eq(fname, ".."))
	{
		if (eq(fname, "*")) path = substr(path, fname);
		*pattern = nullptr;
		return resolveFilePath(path, DirectoryFile);
	}
	else
	{
		str rval = resolveDirPath(path, false);
		ptr p	 = strrchr(rval, '/');
		assert(p);
		*p		 = 0;
		*pattern = p + 1;
		return rval;
	}
}

str FileSystem::resolveFilePath(cstr path, FileType* ftype)
{
	// resolve the directory part of a filepath (up to the last '/')
	// and try to resolve the last path component as a filename.
	// set ftype to the resulting FileType which may be NoFile.

	assert(path);
	FileType dummy;
	if (!ftype) ftype = &dummy;

	str fullpath = resolveDirPath(path, false);
	ptr lastchar = strchr(fullpath, 0) - 1;
	if (*lastchar == '/')
	{
		*lastchar = 0;
		*ftype	  = DirectoryFile;
		return fullpath;
	}
	*ftype = getFileType(fullpath);
	if (*ftype) return fullpath;

	// the filename is a pattern, has wrong upper/lower case or doesn't exist:

	ptr sep0 = strrchr(fullpath, '/');

	if (sep0) *sep0 = 0;
	DirectoryPtr dir = openDir(fullpath);
	if (sep0) *sep0 = '/';

	str	 fname	= sep0 + 1;
	bool has_wc = find_wildcard(fname);
	int	 n_tc	= 0; // count of total filename matches
	int	 n_cc	= 0; // count of case correct filename matches

	FileInfo f1;
	while (FileInfo f2 = dir->next(fname)) // search case folding
	{
		int cc = has_wc ? fnmatch(fname, f2.fname, false) : eq(fname, f2.fname);
		if (cc || n_cc == 0) f1 = f2;
		n_tc += 1;
		n_cc += cc;
		if (n_cc > 1 || (cc && !has_wc)) break;
	}
	if (n_tc == 0) return fullpath; // does not exist
	if (n_cc > 1 || (n_cc == 0 && n_tc > 1)) throw AMBIGUOUS_FILENAME;

	*ftype	= f1.ftype;
	sep0[1] = 0;
	return catstr(fullpath, f1.fname);
}

str FileSystem::resolveFilePath(cstr path, FileType req_ft)
{
	// resolve the directory part of a filepath (all components up to the last '/')
	// and resolve the last path component as a file of the requested FileType.
	// the FileType may be NoFile: but this may also fail if a similarly named file exists.

	assert(path);

	FileType ft;

	// use resolveDirPath() which only considers directories for wildcard resolution:
	if (req_ft == DirectoryFile && *path && lastchar(path) != '/') path = catstr(path, "/");

	str rpath = resolveFilePath(path, &ft);
	if (ft == req_ft) return rpath;
	if (ft == NoFile) throw FILE_NOT_FOUND;
	throw req_ft == RegularFile ? NOT_A_REGULAR_FILE : WRONG_FILETYPE;
}


} // namespace kilipili

/*








































*/
