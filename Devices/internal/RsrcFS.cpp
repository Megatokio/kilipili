// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFS.h"
#include "HeatShrinkDecoder.h"
#include "RsrcFile.h"
#include "Trace.h"
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

inline bool is_compressed(cuptr p) { return p[3] & 0x80; }

inline uint32 usize(cuptr p) { return peek32(p) & 0x7fffffff; }

inline uint32 csize(cuptr p)
{
	assert(is_compressed(p));
	return peek24(p + 4);
}

inline uint32 get_fsize(cuint8* p)
{
	p = skip(p);
	return usize(p);
}

inline cuptr next_entry(cuptr p)
{
	if unlikely (!p) return nullptr;
	p = skip(p);
	if (is_compressed(p)) return p + csize(p) + 8; // compressed file
	else return p + usize(p) + 4;				   // uncompressed file
}

inline const uint8* next_direntry(cuptr p)
{
	trace(__func__);
	p = next_entry(p);
	return p && *p ? p : nullptr;
}

static const uint8* next_direntry(cuptr p, cstr pattern)
{
	trace(__func__);
	while (p && !fnmatch(pattern, cptr(p), true)) { p = next_direntry(p); }
	return p;
}


// ***********************************************************************
// Resource File System

uint64 RsrcFS::getSize()
{
	// get size of the file system

	trace(__func__);

	if (!resource_file_data) return 0;
	cuptr p = resource_file_data;
	while (*p) { p = next_entry(p); }
	return uint(p - resource_file_data);
}

DirectoryPtr RsrcFS::openDir(cstr path)
{
	trace(__func__);
	assert(path);

	cstr full_path = makeFullPath(path);

	path		 = strchr(full_path, ':') + 2;
	cstr pattern = *path == 0 ? "*" : catstr(path, "/*");
	if (next_direntry(resource_file_data, pattern)) return new RsrcDir(this, full_path);
	else throw DIRECTORY_NOT_FOUND;
}

FilePtr RsrcFS::openFile(cstr path, FileOpenMode mode)
{
	/*	uncompressed:
		  char[] filename   0-terminated string
		  uint32 size       sizeof data[]
		  char[] data       uncompressed file data
	
		compressed:
		  char[] filename   0-terminated string
		  uint32 size       uncompressed data size | 0x80000000
		  uint24 csize      sizeof cdata[]
		  uint8  flags		wbits<<4 + lbits
		  char[] data       compressed file data
	*/
	trace(__func__);
	assert(path);

	path = strchr(makeFullPath(path), ':') + 2;
	if (mode & ~READ) throw NOT_WRITABLE;
	cuint8* p = next_direntry(resource_file_data, path);
	if (!p) throw FILE_NOT_FOUND;

	p = skip(p);
	if (is_compressed(p)) return new HeatShrinkDecoder(new RsrcFile(p, csize(p) + 8), false); // compressed
	else return new RsrcFile(p + 4, usize(p));												  // uncompressed
}

ADDR RsrcFS::getFileSize(cstr path) throws
{
	trace(__func__);
	assert(path);

	path	  = strchr(makeFullPath(path), ':') + 2;
	cuint8* p = next_direntry(resource_file_data, path);
	if (!p) throw FILE_NOT_FOUND;
	return usize(skip(p));
}


FileType RsrcFS::getFileType(cstr path) noexcept
{
	trace(__func__);
	assert(path);

	path = strchr(makeFullPath(path), ':') + 2;
	if (*path == 0) return FileType::DirectoryFile; // root dir
	if (next_direntry(resource_file_data, path)) return FileType::RegularFile;
	if (next_direntry(resource_file_data, catstr(path, "/*"))) return FileType::DirectoryFile;
	else return FileType::NoFile;
}


// ***********************************************************************
// Resource Directory

RsrcDir::RsrcDir(RCPtr<RsrcFS> fs, cstr full_path) : //
	Directory(std::move(fs), full_path),
	dpos(resource_file_data)
{}

void RsrcDir::rewind() throws
{
	trace(__func__);
	dpos = resource_file_data;
	subdirs.purge();
}

bool RsrcDir::is_in_subdirs(cstr path, cptr sep)
{
	trace(__func__);
	assert(path);
	assert(sep);

	uint len = uint(sep - path) + 1;
	assert(len <= 256);

	for (uint i = 0; i < subdirs.count(); i++)
	{
		if (memcmp(path, subdirs[i], len) == 0) return true;
	}
	return false;
}

FileInfo RsrcDir::next(cstr pattern) throws
{
	trace(__func__);

	cstr path = strchr(dirpath, ':') + 2;
	assert(path);

	uint					  dpathlen = strlen(path);
	static constexpr DateTime notime; // all-zero

	// subdirs are returned when next() encounters the first file which establishes the subdir.
	// in order to only return a subdir once the returned subdir is registered in Array subdirs.

	while (this->dpos)
	{
		cuptr dpos = this->dpos;
		this->dpos = next_direntry(dpos);

		if (dpathlen) // we are a subdir
		{
			if (dpos[dpathlen] != '/') continue;
			if (memcmp(path, dpos, dpathlen)) continue;
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

} // namespace kio::Devices

/*





























*/
