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

ADDR RsrcFS::getSize()
{
	// get size of the file system

	trace(__func__);

	if (!resource_file_data) return 0;
	cuptr p = resource_file_data;
	while (*p) { p = next_entry(p); }
	return ADDR(p - resource_file_data);
}

DirectoryPtr RsrcFS::openDir(cstr path)
{
	trace(__func__);
	assert(path);

	path		 = makeAbsolutePath(path);
	cstr pattern = eq(path, "/") ? "*" : catstr(path + 1, "/*");
	if (next_direntry(resource_file_data, pattern)) return new RsrcDir(this, path);
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

	path = makeAbsolutePath(path);
	if (mode & ~READ) throw NOT_WRITABLE;
	cuint8* p = next_direntry(resource_file_data, path + 1);
	if (!p) throw FILE_NOT_FOUND;

	p = skip(p);
	if (is_compressed(p)) return new HeatShrinkDecoder(new RsrcFile(p, csize(p) + 8), false); // compressed
	else return new RsrcFile(p + 4, usize(p));												  // uncompressed
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
	trace(__func__);
	dpos = resource_file_data;
	subdirs.purge();
}


bool is_in_ram(const void* p) { return uint32(p) >= SRAM_STRIPED_BASE && uint32(p) < SRAM_STRIPED_END; }
bool is_in_flash(const void* p) { return uint32(p) >= XIP_BASE && uint32(p) < XIP_BASE + PICO_FLASH_SIZE_BYTES; }

void dump_memory(cstr title, cptr p, uint sz)
{
	printf("%s\n", title);
	for (uint i = 0; i < sz; i += 32)
	{
		printf("0x%08x: ", uint32(p));
		for (uint j = i; j < i + 32; j++) printf("%02x ", p[j]);
		for (uint j = i; j < i + 32; j++) printf("%c", is_printable(p[j]) ? p[j] : '_');
		printf("\n");
	}
}

bool RsrcDir::is_in_subdirs(cstr path, cptr sep)
{
	trace(__func__);
	assert(path);
	assert(sep);
	assert(is_in_ram(path) || is_in_flash(path));

	uint len = uint(sep - path) + 1;
	assert(len <= 256);
	assert(is_in_ram(subdirs.getData()) || (subdirs.count() == 0 && subdirs.getData() == nullptr));

	for (uint i = 0; i < subdirs.count(); i++)
	{
		if (!is_in_flash(subdirs[i]))
			dump_memory(usingstr("subdirs.data[%u]", subdirs.count()), cptr(subdirs.getData()), 256);

		assert(is_in_flash(subdirs[i]));
		if (memcmp(path, subdirs[i], len) == 0) return true;
	}
	return false;
}

FileInfo RsrcDir::next(cstr pattern) throws
{
	trace(__func__);
	assert(path && path[0] == '/');

	uint					  dpathlen = strlen(path) - 1;
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
	trace(__func__);

	fpath = makeAbsolutePath(fpath);
	return fs->openFile(fpath, fmode);
}

DirectoryPtr RsrcDir::openDir(cstr dpath) throws
{
	trace(__func__);

	dpath = makeAbsolutePath(dpath);
	return fs->openDir(dpath);
}


} // namespace kio::Devices

/*





























*/
