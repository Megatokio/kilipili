// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Directory.h"
#include "ff15/source/ff.h"

namespace kio ::Devices
{

class FatDir;
using FatDirPtr = RCPtr<FatDir>;
class FatFS;
using FatFSPtr = RCPtr<FatFS>;


class FatDir : public Directory
{
public:
	virtual ~FatDir() noexcept override;

	virtual FileInfo	 next(cstr pattern = nullptr) throws override;
	virtual void		 rewind() throws override;
	virtual FilePtr		 openFile(cstr path, FileOpenMode mode) override;
	virtual DirectoryPtr openDir(cstr path) override;
	virtual void		 remove(cstr path) override;
	virtual void		 makeDir(cstr path) override;
	virtual void		 rename(cstr path, cstr name) override;
	virtual void		 setFmode(cstr path, FileMode fmode, uint8 mask) override;
	virtual void		 setMtime(cstr path, uint32 mtime) override;

	cstr path = nullptr;

private:
	FatFSPtr device; // keep alive
	DIR		 fatdir;

	FatDir(FatFSPtr device, cstr path) throws;
	friend class FatFS;
	cstr makeAbsolutePath(cstr path);
	//void copy_fileinfo(FileInfo&, const FILINFO&);
};

} // namespace kio::Devices
