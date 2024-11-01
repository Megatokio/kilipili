// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFS.h"
#include "RsrcFile.h"
#include "cstrings.h"
#include <cstring>

namespace kio ::Devices
{

inline cuptr skip(cuptr p)
{
	while (*p++) {}
	return p;
}

inline uint32 peek32(cuint8* p)
{
	uint32 value;
	memcpy(&value, p, 4);
	return value;
}

inline uint32 peek24(cuint8* p)
{
	uint32 value = 0;
	memcpy(&value, p, 3);
	return value;
}

inline uint32 get_fsize(cuint8* p)
{
	p = skip(p);
	return peek32(p[3] ? p + 4 : p);
}

inline cuptr next_entry(cuptr p)
{
	if unlikely (!p) return nullptr;
	p = skip(p);
	return p + peek24(p) + 4;
}

inline const uint8* next_direntry(cuptr p)
{
	p = next_entry(p);
	return p && *p ? p : nullptr;
}

static const uint8* next_direntry(cuptr p, cstr pattern)
{
	while (p && !fnmatch(pattern, cptr(p), true)) { p = next_direntry(p); }
	return p;
}


// ***********************************************************************
// Resource File System

RsrcFS* RsrcFS::getInstance() noexcept
{
	static RsrcFS rsfs;
	return &rsfs;
}

RsrcFS::RsrcFS() noexcept : FileSystem("rsrc")
{
	rc = 1; // never destroy!
}

ADDR RsrcFS::getSize()
{
	if (!resource_file_data) return 0;
	cuptr p = resource_file_data;
	while (*p) { p = next_entry(p); }
	return uint32(p - resource_file_data);
}

DirectoryPtr RsrcFS::openDir(cstr path)
{
	assert(path);
	path		 = makeAbsolutePath(path);
	cstr pattern = eq(path, "/") ? "*" : catstr(path + 1, "/*");
	if (next_direntry(resource_file_data, pattern)) return new RsrcDir(this, path);
	else throw DIRECTORY_NOT_FOUND;
}

FilePtr RsrcFS::openFile(cstr path, FileOpenMode mode)
{
	/*	uncompressed[] =
		  char[] filename   0-terminated string
		  uint24 size       data size )
		  uint8  flag = 0
		  char[size] data   uncompressed file data

		compressed[] =
		  char[] filename   0-terminated string
		  uint24 csize+4    compressed size (incl. usize)
		  uint8  flags != 0 windowsize<<4 + lookaheadsize
		  uint32 usize      uncompressed size
		  char[csize] data  compressed file data
	*/
	assert(path);
	path = makeAbsolutePath(path);
	if (mode & ~READ) throw NOT_WRITABLE;
	cuint8* p = next_direntry(resource_file_data, path + 1);
	if (!p) throw FILE_NOT_FOUND;

	p			 = skip(p);
	uint32 csize = peek24(p);
	uint8  flags = p[3];
	if (flags == 0) return new RsrcFile(p + 4, csize);
	uint32 usize = peek32(p + 4);
	return new CompressedRomFile(p + 8, usize, csize - 4, flags);
}


// ***********************************************************************
// Resource Directory

RsrcDir::RsrcDir(RCPtr<RsrcFS> fs, cstr path) : //
	Directory(path),
	fs(std::move(fs)),
	dpos(resource_file_data)
{}

void RsrcDir::rewind() throws
{
	dpos = resource_file_data;
	subdirs.purge();
}

bool RsrcDir::is_in_subdirs(cstr path, cptr sep)
{
	uint len = uint(sep - path) + 1;

	for (uint i = 0; i < subdirs.count(); i++)
	{
		if (memcmp(path, subdirs[i], len) == 0) return true;
	}
	return false;
}

FileInfo RsrcDir::next(cstr pattern) throws
{
	assert(path[0] == '/');
	uint					  dpathlen = strlen(path) - 1;
	static constexpr DateTime notime; // all-zero

	// subdirs are returned when next() encounters the first file which establishes the subdir.
	// in order to only return a subdir once the returned subdir is registered in Array subdirs.

	while (dpos)
	{
		cuptr dpos = this->dpos;
		this->dpos = next_direntry(dpos);

		if (dpathlen) // we are a subdir
		{
			if (dpos[dpathlen] != '/') continue;
			if (memcmp(path + 1, dpos, dpathlen)) continue;
			dpos += 1 + dpathlen; // this is a nice trick! :-)
		}

		cstr fname = cptr(dpos);

		if (cptr sep = strchr(fname, '/')) // this is a subdir
		{
			if (is_in_subdirs(fname, sep)) continue; // already returned
			subdirs.append(fname);
			fname = substr(fname, sep);
			if (!fnmatch(pattern, fname, true)) continue;
			else return FileInfo(fname, 0 /*size*/, notime, DirectoryFile, WriteProtected);
		}

		if (!fnmatch(pattern, fname, true)) continue;
		else return FileInfo(fname, get_fsize(dpos), notime, RegularFile, WriteProtected);
	}

	return FileInfo(nullptr, 0, notime, NoFile, WriteProtected);
}

FilePtr RsrcDir::openFile(cstr fpath, FileOpenMode fmode) throws
{
	fpath = makeAbsolutePath(fpath);
	return fs->openFile(fpath, fmode);
}

DirectoryPtr RsrcDir::openDir(cstr dpath) throws
{
	dpath = makeAbsolutePath(dpath);
	return fs->openDir(dpath);
}


} // namespace kio::Devices

/*





























*/
