// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FatDir.h"
#include "FatFS.h"
#include "FatFile.h"
#include "Logger.h"
#include "Trace.h"
#include "cdefs.h"
#include "cstrings.h"
#include <pico/stdio.h>
#include <utility>

namespace kio ::Devices
{

template<>
FileInfo::FileInfo(const FILINFO& info)
{
	trace(__func__);

	fname		 = newcopy(info.fname);
	fsize		 = SIZE(info.fsize);
	ftype		 = info.fattrib & AM_DIR ? DirectoryFile : RegularFile;
	fmode		 = FileMode(info.fattrib & 0x0f);
	mtime.year	 = uint8(info.fdate >> 9) + (1980 - 1970);
	mtime.month	 = ((info.fdate >> 5) & 0x0f) - 1;
	mtime.day	 = (info.fdate & 0x1f) - 1;
	mtime.hour	 = info.ftime >> 11;
	mtime.minute = (info.ftime >> 5) & 0x3f;
	mtime.second = (info.ftime & 0x1f) * 2;
}

FatDir::FatDir(RCPtr<FileSystem>fs, cstr path) throws : //
	Directory(std::move(fs),path)
{
	trace(__func__);
	//assert(dynamic_cast<FatFS*>(this->fs.ptr()));

	FRESULT err = f_opendir(&fatdir, catstr(this->fs->name, ":", path));
	if (err) throw tostr(err);
}

FatDir::~FatDir() noexcept
{
	trace(__func__);

	FRESULT err = f_closedir(&fatdir);
	if (err) logline("close FatDir: %s", tostr(err));
}

void FatDir::rewind() throws
{
	trace(__func__);

	FRESULT err = f_readdir(&fatdir, nullptr);
	if (err) throw tostr(err);
}

FileInfo FatDir::next(cstr pattern) throws
{
	trace(__func__);

	for (;;)
	{
		FILINFO filinfo;
		FRESULT err = f_readdir(&fatdir, &filinfo);
		if (err) throw tostr(err);
		if (filinfo.fname[0] == 0) return FileInfo(); // end of dir
		if (fnmatch(pattern, filinfo.fname, true)) return FileInfo(filinfo);
	}
}

FilePtr FatDir::openFile(cstr path, FileOpenMode mode)
{
	trace(__func__);

	path = makeAbsolutePath(path);
	return new FatFile(static_cast<FatFS*>(fs.ptr()), path, mode);
}

DirectoryPtr FatDir::openDir(cstr path)
{
	trace(__func__);

	path = makeAbsolutePath(path);
	return new FatDir(fs, path);
}

void FatDir::remove(cstr path)
{
	trace(__func__);

	path		= makeAbsolutePath(path);
	FRESULT err = f_unlink(path);
	if (err) throw tostr(err);
}

void FatDir::makeDir(cstr path)
{
	trace(__func__);

	path		= makeAbsolutePath(path);
	FRESULT err = f_mkdir(path);
	if (err) throw tostr(err);
}

void FatDir::rename(cstr path, cstr name)
{
	trace(__func__);

	path		= makeAbsolutePath(path);
	FRESULT err = f_rename(path, name);
	if (err) throw tostr(err);
}

void FatDir::setFmode(cstr path, FileMode fmode, uint8 mask)
{
	trace(__func__);

	if constexpr (!FF_USE_CHMOD) throw "option disabled";

	path		= makeAbsolutePath(path);
	FRESULT err = f_chmod(path, fmode, mask);
	if (err) throw tostr(err);
}

void FatDir::setMtime(cstr path, uint32 mtime)
{
	trace(__func__);

	if constexpr (!FF_USE_CHMOD) throw "option disabled";

	FILINFO				   info;
	static constexpr uint8 dpm[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	uint days = mtime / (24 * 60 * 60);

	mtime -= days * (24 * 60 * 60);
	uint s = mtime % 60;
	mtime /= 60;
	uint m = mtime % 60;
	mtime /= 60;
	uint h	   = mtime;
	info.ftime = uint16(h * 2048 | m * 32 | s / 2);

	days -= 365 * 10 + 2; // base(1970) -> base(1980)

	uint y = days / (4 * 365 + 1);
	days -= y * (4 * 365 + 1);

	if (days >= 366) days -= 1;
	y += days / 365;
	days = days % 365;

	uint mo = 0;
	if (y % 4 != 0 && days >= 31 + 28) days++;
	while (days >= dpm[mo])
	{
		days -= dpm[mo];
		mo += 1;
	}

	info.fdate = uint16(y * 512 | (mo + 1) * 32 | (days + 1));

	FRESULT err = f_utime(path, &info);
	if (err) throw tostr(err);
}


} // namespace kio::Devices


/*









































*/
