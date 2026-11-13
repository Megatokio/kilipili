// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FileSystem.h"
#include "devices_types.h"

namespace kilipili
{


/*
	A Directory represents a directory file on the FileSystem.
	It provides methods to iterate over files in the directory.
	For convenience it also stores the full path to the directory.
*/
class Directory : public RCObject
{
public:
	Id("Directory");

	virtual ~Directory() noexcept override { delete[] dirpath; }

	virtual void	 rewind() throws					 = 0; // rewind dir
	virtual FileInfo next(cstr pattern = nullptr) throws = 0; // next file in dir

	cstr		getFullPath() const noexcept { return dirpath; } // "dev:/path/to/dir"
	FileSystem* getFS() const noexcept { return fs; }

protected:
	Directory(RCPtr<FileSystem>, cstr full_path);

	RCPtr<FileSystem> fs;
	cstr			  dirpath = nullptr; // full path "dev:/path/to/dir"
};

} // namespace kilipili
