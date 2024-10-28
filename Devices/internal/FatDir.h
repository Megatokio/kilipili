// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Directory.h"
#include "ff15/source/ff.h"

namespace kio ::Devices
{

class FatFS;
using FatFSPtr = RCPtr<FatFS>;


class FatDir : public Directory
{
public:
	virtual ~FatDir() noexcept override;

	virtual FileInfo	 next(cstr pattern = nullptr) throws override;
	virtual void		 rewind() throws override;
	virtual FilePtr		 openFile(cstr path, FileOpenMode mode = READ) override;
	virtual DirectoryPtr openDir(cstr path) override;
	virtual void		 remove(cstr path) override;
	virtual void		 makeDir(cstr path) override;
	virtual void		 rename(cstr path, cstr name) override;
	virtual void		 setFmode(cstr path, FileMode fmode, uint8 mask) override;
	virtual void		 setMtime(cstr path, uint32 mtime) override;

private:
	FatFSPtr device; // keep alive
	DIR		 fatdir;

	FatDir(FatFSPtr device, cstr path) throws;
	friend class FatFS;
};

using FatDirPtr = RCPtr<FatDir>;

} // namespace kio::Devices
