// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "StdDir.h"
#include "common/cdefs.h"
#include "common/cstrings.h"
#include "common/trace.h"
#include <fcntl.h>
#include <sys/stat.h>


namespace kilipili
{

static gid_t grp = getegid();
static uid_t usr = geteuid();

static uint emode(mode_t mode, gid_t file_grp, uid_t file_usr) noexcept
{
	if (grp == file_grp) mode |= (mode >> 3);
	if (usr == file_usr) mode |= (mode >> 6);
	if (usr == 0) mode |= (mode >> 6); // assumes root = 0
	return mode & 7;
}

static void operator|=(FileMode& z, FileMode f) { z = FileMode(z | f); }

FileInfo StdDir::makeFileInfo(const dirent* dirent)
{
	// currently only default handling of symlinks:
	//   name = name of the symlink
	//   ftype, fsize, fmode, mtime of the target
	//   IsaSymlink is set in FileMode

	// if the StdDir was created by StdFS (this->fs != nullptr) then FileInfo.name is converted to ucs1

	struct stat st;
	int			err = ::stat(catstr(dirpath_utf8, "/", dirent->d_name), &st);
	if (err) throw strerror(errno);

	uint fsize = 0;
	fsize	   = st.st_size == uint(st.st_size) ? uint(st.st_size) : 0xffffffff;

	FileMode fmode = Writable;
	uint	 mode  = emode(st.st_mode, st.st_gid, st.st_uid);
	if ((mode & 2) == 0) fmode = WriteProtected;

	FileType ftype = SpecialFile;
	if ((st.st_mode & S_IFMT) == S_IFDIR) ftype = DirectoryFile;
	if ((st.st_mode & S_IFMT) == S_IFREG) ftype = RegularFile;
	if (dirent->d_type == DT_LNK) fmode |= IsaSymlink;

	tm d;
	localtime_r(&st.st_mtim.tv_sec, &d);
	DateTime mtime;
	mtime.year	 = uint8(d.tm_year - 70); // base(1900) -> base(1970)
	mtime.month	 = uint8(d.tm_mon);		  // Jan=0 -> Jan=0
	mtime.day	 = uint8(d.tm_mday - 1);  // primo=1 -> primo=0
	mtime.hour	 = uint8(d.tm_hour);
	mtime.minute = uint8(d.tm_min);
	mtime.second = uint8(d.tm_sec);

	cstr name = dupstr(dirent->d_name);
	if (fs) name = fromutf8str(name);
	return FileInfo(name, fsize, mtime, ftype, fmode);
}

StdDir::StdDir(cstr path_utf8) throws :									// "/path/to/dir"
	StdDir(nullptr, path_utf8, catstr("disk:", fromutf8str(path_utf8))) // not neccessarily a full path
{}

StdDir::StdDir(RCPtr<FileSystem> fs, cstr path_utf8, cstr path_latin1) throws :
	Directory(fs, path_latin1),
	dirpath_utf8(newcopy(path_utf8))
{
	trace(__func__);

	dir = ::opendir(path_utf8);
	if (!dir) throw strerror(errno);
}

StdDir::~StdDir() noexcept
{
	if (dir) closedir(dir);
	delete[] dirpath_utf8;
}

void StdDir::rewind() throws
{
	assert(dir);

	::rewinddir(dir);
}

FileInfo StdDir::next(cstr pattern) throws
{
	// if the StdDir was created directly (fs=nullptr) then pattern and FileInfo.name are in utf8,
	// else pattern and FileInfo.name are in local ucs1 character set (currently latin-1 only)

	assert(dir);

	if (fs && pattern) pattern = toutf8str(pattern);

	for (;;)
	{
		errno		   = 0;				 // required
		dirent* dirent = ::readdir(dir); // NOT THREAD SAFE!
		if (!dirent)
		{
			if (errno) throw strerror(errno);
			else return FileInfo(); // eof
		}

		cstr name = dirent->d_name;

		if (name[0] == '.' && (name[1] == '.' || name[1] == 0)) continue;
		if (!pattern || fnmatch(pattern, name, true)) return makeFileInfo(dirent);
	}
}

} // namespace kilipili


/*


























*/
