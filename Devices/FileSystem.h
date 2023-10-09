// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BlockDevice.h"

namespace kio::Devices
{

class FatFile;
class FatDir;


enum FileType : uint8 {
	NoFile		= 0,
	RegularFile = 1,
	Directory	= 2,
};

enum FileMode : uint8 {
	WriteProtected = 0x1, // Fat: Read-only
	Hidden		   = 0x2, // Fat: Hidden
	SystemFile	   = 0x4, // Fat: System
	Modified	   = 0x8, // Fat: Archive
};

struct FileInfo
{
	cstr	 fname; // tempmem or stat() argument!
	SIZE	 fsize;
	uint32	 mtime; // seconds since 1970
	FileType ftype;
	FileMode fmode;
	uint8	 padding[2] = {0};
};


class FileSystem
{
public:
	BlockDevice& blkdev;
	cstr		 name; // static

public:
	FileSystem(BlockDevice& blkdev, cstr name) noexcept : blkdev(blkdev), name(name) {}
	virtual ~FileSystem() noexcept;

	virtual void mkfs()	   = 0;
	virtual void mount()   = 0;
	virtual ADDR getFree() = 0;
	virtual ADDR getSize() = 0;

	virtual void setcwd(cstr path); // chdir+chdrive
	virtual cstr getcwd();

	virtual bool getinfo(cstr path, FileInfo* = nullptr); // check exist, get info
	virtual void rename(cstr path, cstr name);
	virtual void setmtime(cstr path, uint mtime);
	virtual void setmode(cstr path, FileMode);

	virtual FatFile* open(cstr path, uint8 mode);
	virtual FatDir*	 opendir(cstr path);

	virtual void mkdir(cstr path);
	virtual void remove(cstr path);
};

} // namespace kio::Devices
