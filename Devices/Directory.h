// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "devices_types.h"

namespace kio::Devices
{


/*
	A Directory represents a directory file on the FileSystem.
	It provides methods
	- to create, delete and open files and nested directories
	- iterate over files in directory
	- get and manipulate meta data
*/
class Directory : public RCObject
{
public:
	virtual ~Directory() noexcept override { delete[] path; }

	/* ————————————————————————————————————————
		Iterate over directory entries:
	*/
	virtual void	 rewind() throws					 = 0; // rewind dir
	virtual FileInfo next(cstr pattern = nullptr) throws = 0; // next file in dir
	FileInfo		 find(cstr pattern = nullptr) throws;	  // rewind & return first

	/* ————————————————————————————————————————
		Create, delete and open files and directories:
	*/
	virtual FilePtr		 openFile(cstr path, FileOpenMode mode = READ) throws = 0;
	virtual DirectoryPtr openDir(cstr path) throws							  = 0;
	virtual void		 remove(cstr path) throws							  = 0;
	virtual void		 makeDir(cstr path) throws							  = 0;

	/* ————————————————————————————————————————
		Get and manipulate meta data:
	*/
	virtual void rename(cstr path, cstr name) throws				   = 0;
	virtual void setFmode(cstr path, FileMode mode, uint8 mask) throws = 0;
	virtual void setMtime(cstr path, uint32 mtime) throws			   = 0;

	cstr getPath() const noexcept { return path; }

protected:
	cstr path = nullptr;
	Directory(cstr path);
	cstr makeAbsolutePath(cstr path);
};


// ===================== Implementations ======================


inline FileInfo Directory::find(cstr pattern) throws
{
	rewind();
	return next(pattern);
}

} // namespace kio::Devices
