// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FileSystem.h"

/*
	class `StdFS` represents a file system on the host computer.
	It is only used in the emulator. Desktop tools which want to access files and directories
	in the host file system create StdFile and StdDir directly.

	class `StdFS` can be used for the entire file system, when created with name "disk"
	or for mounted devices in "/mnt" and "/media" using their name.
*/

namespace kilipili
{

class StdFS : public FileSystem
{
public:
	virtual ~StdFS() noexcept override;

	virtual uint64 getFree() override; // TODO
	virtual uint64 getSize() override; // TODO

	virtual DirectoryPtr openDir(cstr path) override;
	virtual FilePtr		 openFile(cstr path, FileOpenMode flags = READ) override;

	virtual FileType getFileType(cstr path) noexcept override;
	virtual void	 makeDir(cstr path) throws override;
	virtual void	 remove(cstr path) throws override;
	virtual void	 rename(cstr path, cstr name) throws override;
	virtual void	 setFmode(cstr path, FileMode mode, uint8 mask) throws override;
	virtual void	 setMtime(cstr path, uint32 mtime) throws override;
	virtual ADDR	 getFileSize(cstr path) override;

protected:
	friend FileSystemPtr mountFS(cstr name_ucs1);
	StdFS(cstr name) throws;

private:
	cstr make_utf8_path(cstr path_ucs1);
	cstr fs_path_ucs1 = nullptr; // if mounted FS in mnt/ or media/
};


} // namespace kilipili


/*



















*/
