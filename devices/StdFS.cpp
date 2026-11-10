// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "StdFS.h"
#include "StdDir.h"
#include "StdFile.h"
#include "common/cstrings.h"
#include "common/sdbm_hash.h"
#include "cstrings.h"
#include "trace.h"
#include <cstring>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>


namespace kilipili
{

static cstr hash(cstr name_ucs1)
{
	// make a fs name which fits into 7+1 bytes

	assert(name_ucs1);
	uint len = strlen(name_ucs1);
	if (len < 8) return name_ucs1;

	// either hash and risk of collission or a seed and ever changing names ...
	uint hash = sdbm_hash(name_ucs1);
	return catstr(leftstr(name_ucs1, 4), leftstr(numstr(hash, 36), 3));
}

cstr StdFS::make_utf8_path(cstr path_ucs1)
{
	// convert character encoding and fs naming conventions

	path_ucs1 = makeFullPath(path_ucs1); // "disk:/foo/path"
	cptr dp	  = strchr(path_ucs1, ':');
	assert(dp);

	if (fs_path_ucs1) // "usb-EF3:/foo/path"  ->  "media/username/usb-stick128mb/foo/path"
	{
		return toutf8str(catstr(fs_path_ucs1, dp + 1));
	}
	else // "disk:/foo/path"  ->  "/foo/path"
	{
		return toutf8str(dp + 1);
	}
}

Array<cstr> StdFS::getListOfDevices()
{
	Array<cstr> list;
	list << "disk";
	if (DIR* dir = opendir("/mnt"))
	{
		while (dirent* dirent = readdir(dir))
		{
			if (dirent->d_name[0] != '.' && dirent->d_type == DT_DIR) //
				list << dupstr(fromutf8str(dirent->d_name));
		}
		closedir(dir);
	}
	if (DIR* dir = opendir("/media"))
	{
		// the rules in "/media" may vary.

		while (dirent* media_dirent = readdir(dir))
		{
			if (media_dirent->d_name[0] == '.') continue;
			if (media_dirent->d_type != DT_DIR) continue;

			if (DIR* dir = opendir(catstr("/media/", media_dirent->d_name)))
			{
				// list << dupstr(fromutf8str(media_dirent->d_name)); // probably the user name

				while (dirent* dirent = readdir(dir))
				{
					if (dirent->d_name[0] != '.' && dirent->d_type == DT_DIR) //
						list << dupstr(fromutf8str(dirent->d_name));
				}
				closedir(dir);
			}
		}
		closedir(dir);
	}
	return list;
}

StdFS::StdFS(cstr name_ucs1) throws : FileSystem(hash(name_ucs1))
{
	trace("StdFS::StdFS");

	if (lceq(name_ucs1, "disk")) {} // fs_path_ucs1 = nullptr
	else
	{
		cstr name_utf8 = toutf8str(name_ucs1);
		if (DIR* dir = opendir("/mnt"))
		{
			while (dirent* dirent = readdir(dir))
			{
				if (fs_path_ucs1) break;
				if (dirent->d_name[0] == '.') continue;
				if (dirent->d_type != DT_DIR) continue;
				if (lceq(name_utf8, dirent->d_name)) //
					fs_path_ucs1 = catstr("/mnt/", fromutf8str(dirent->d_name));
			}
			closedir(dir);
		}
		if (DIR* dir = opendir("/media"))
		{
			while (dirent* media_dirent = readdir(dir))
			{
				if (fs_path_ucs1) break;
				if (media_dirent->d_name[0] == '.') continue;
				if (media_dirent->d_type != DT_DIR) continue;

				if (lceq(name_utf8, media_dirent->d_name)) //
					fs_path_ucs1 = catstr("/media/", fromutf8str(media_dirent->d_name));

				else if (DIR* dir = opendir(catstr("/media/", media_dirent->d_name)))
				{
					while (dirent* dirent = readdir(dir))
					{
						if (fs_path_ucs1) break;
						if (dirent->d_name[0] == '.') continue;
						if (dirent->d_type != DT_DIR) continue;
						if (lceq(name_utf8, dirent->d_name)) //
							fs_path_ucs1 = catstr("/media/", media_dirent->d_name, "/", fromutf8str(dirent->d_name));
					}
					closedir(dir);
				}
			}
			closedir(dir);
		}
		if (fs_path_ucs1 == nullptr) throw UNKNOWN_FILESYSTEM; // on behalf of mountFS()
		else fs_path_ucs1 = newcopy(fs_path_ucs1);
	}
}

StdFS::~StdFS() noexcept
{
	delete[] fs_path_ucs1;
	sync();
}

uint64 StdFS::getFree() { return 100 MB; } // TODO
uint64 StdFS::getSize() { return 200 MB; } // TODO

DirectoryPtr StdFS::openDir(cstr path_ucs1)
{
	trace("StdFS::openDir");

	cstr path_utf8 = make_utf8_path(path_ucs1);
	return new StdDir(this, path_utf8, path_ucs1);
}

FilePtr StdFS::openFile(cstr path_ucs1, FileOpenMode flags)
{
	trace("StdFS::openFile");

	cstr path_utf8 = make_utf8_path(path_ucs1);
	return new StdFile(path_utf8, flags);
}

FileType StdFS::getFileType(cstr path_ucs1) noexcept
{
	trace("StdFS::getFileType");

	cstr		path_utf8 = make_utf8_path(path_ucs1);
	struct stat st;
	int			err = ::stat(path_utf8, &st);
	if (err) return NoFile;

	if ((st.st_mode & S_IFMT) == S_IFDIR) return DirectoryFile;
	if ((st.st_mode & S_IFMT) == S_IFREG) return RegularFile;
	return SpecialFile;
}

ADDR StdFS::getFileSize(cstr path_ucs1)
{
	trace("StdFS::getFileSize");

	cstr		path_utf8 = make_utf8_path(path_ucs1);
	struct stat st;
	int			err = ::stat(path_utf8, &st);
	if (err) throw strerror(errno);

	return st.st_size == ADDR(st.st_size) ? ADDR(st.st_size) : 0xffffffff;
}

void StdFS::makeDir(cstr path_ucs1) throws
{
	trace("StdFS::makeDir");

	cstr path_utf8 = make_utf8_path(path_ucs1);
	int	 err	   = ::mkdir(path_utf8, 0770);
	if (err)
	{
		if (errno == EEXIST) return;
		else throw strerror(errno);
	}
}

void StdFS::remove(cstr path_ucs1) throws
{
	trace("StdFS::remove");

	cstr path_utf8 = make_utf8_path(path_ucs1);
	int	 err	   = ::remove(path_utf8);
	if (err) throw strerror(errno);
}

void StdFS::rename(cstr path_ucs1, cstr name_ucs1) throws
{
	trace("StdFS::rename");

	cstr oldpath_utf8 = make_utf8_path(path_ucs1);
	cstr newpath_utf8 = catstr(directory_from_path(oldpath_utf8), toutf8str(name_ucs1));
	int	 err		  = ::rename(oldpath_utf8, newpath_utf8);
	if (err) throw strerror(errno);
}

void StdFS::setFmode(cstr path_ucs1, FileMode mode, uint8 mask) throws
{
	trace("StdFS::setFmode");

	cstr path_utf8 = make_utf8_path(path_ucs1);
	(void)path_utf8;

	// the only bit we might want to change up to now is the "write protect" bit
	debugstr("StdFS::setFmode(): not implemented (TODO)\n");
}

void StdFS::setMtime(cstr path_ucs1, uint32 mtime) throws
{
	trace("StdFS::setMtime");

	cstr	path_utf8 = make_utf8_path(path_ucs1);
	timeval tv[2]	  = {{mtime, 0}, {mtime, 0}};
	int		err		  = ::utimes(path_utf8, tv);
	if (err) throw strerror(errno);
}


} // namespace kilipili


/*

































*/
