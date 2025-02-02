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
	assert(kio::find(path, ":/"));

	FRESULT err = f_opendir(&fatdir, path);
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


} // namespace kio::Devices


/*









































*/
