// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BlockDevice.h"

extern const unsigned char resource_file_data[] __weak_symbol;

namespace kio::Devices
{

// =================================================== //
//			global methods without object			   //
// =================================================== //


/*	Create filesystem on the supplied BlockDevice.
	Type "FAT" creates the default variant of FAT for the disk size.
	Other file systems are TODO.
	The BlockDevice should not be in use by a FileSystem.
*/
extern void makeFS(BlockDevicePtr, cstr type = "FAT") throws;
extern void makeFS(cstr devname, cstr type = "FAT") throws;

/* 	Mount the well-known device with name `devicename`.
	Currently well-known are:
	- "sdcard"  the default SDCard
	- "rsrc"    the resource file system
	- "flash"   the internal program flash
	Normally not needed because the FS functions do this automatically.
	But you may store the returned FileSystemPtr to keep the FileSystem alive.
	setWorkDevice() keeps the FileSystem alive too.
*/
extern FileSystemPtr mount(cstr devicename_or_fullpath_with_colon);
extern FileSystemPtr mount(cstr name, BlockDevicePtr device) throws;

// needed by FatFS:
extern int index_of(FileSystem*) noexcept;

/*	Unmount a FileSystem.
	Normally not needed because the FS does not keep the FileSystems alive
	and they are automatically unmounted on exit of each function.
	Only the current work dir is kept alive.
	Therefore the only thing `unmount()` does is to check if it is the cwd.
	To really unmount a FS you must also release all FileSystemPtr's you may have yourself.
	You may check the retain count `FileSystem::rc`.
*/
extern void unmount(FileSystem*);
extern void unmountAll();

/*	Set the current working directory.
	The cwd keeps the FileSystem alive.
*/
extern void			 setWorkDevice(FileSystemPtr);
extern FileSystemPtr getWorkDevice();		// get device or nullptr
extern void			 setWorkDir(cstr path); //
extern cstr			 getWorkDir();			// get cwd path or nullptr

// make canonical full path from any path.
// prepend device and cwd, clean-up path.
extern cstr makeFullPath(cstr path);

// query file type.
extern FileType getFileType(cstr path) noexcept;
inline bool		isaDirectory(cstr path) noexcept { return getFileType(path) == FileType::DirectoryFile; }
inline bool		isaFile(cstr path) noexcept { return getFileType(path) == FileType::RegularFile; }
inline bool		exists(cstr path) noexcept { return getFileType(path) != FileType::NoFile; }

/* 	Open a file or directory.
	FilePtr and DirectoryPtr keep their FileSystem alive.

	-  dev:/abs_path/to/dir/or/file	= full absolute path
	-  dev:rel_path/to/dir/or/file	= relative path starting at cwd of device dev
	-  /abs_path/to/dir/or/file		= absolute path starting at root dir of cwd
	-  rel_path/to/dir/or/file		= relative path starting at cwd
	-  dev:/						= root dir of device dev
	-  dev: 						= cwd of device dev
	-  /							= root dir of cwd
	-  ""							= cwd
*/
extern FilePtr		openFile(cstr path, FileOpenMode flags = READ) throws;
extern DirectoryPtr openDir(cstr path) throws;
extern void			makeDir(cstr path) throws;
extern void			remove(cstr path_and_pattern) throws;
extern void			copy(cstr source_path_and_pattern, cstr dest_path) throws;


/* ____________________________________________________________
	Base class for File Systems
*/
class FileSystem : public RCObject
{
public:
	Id("FileSystem");

	virtual ~FileSystem() noexcept override;

	virtual uint64 getFree() = 0; // If calculation is expensive return a lower estimate
	virtual uint64 getSize() = 0;

	virtual DirectoryPtr openDir(cstr path)								= 0;
	virtual FilePtr		 openFile(cstr path, FileOpenMode flags = READ) = 0;

	virtual FileType getFileType(cstr path) noexcept					   = 0;
	virtual void	 makeDir(cstr path) throws							   = 0;
	virtual void	 remove(cstr path) throws							   = 0;
	virtual void	 rename(cstr path, cstr name) throws				   = 0;
	virtual void	 setFmode(cstr path, FileMode mode, uint8 mask) throws = 0;
	virtual void	 setMtime(cstr path, uint32 mtime) throws			   = 0;

	void setWorkDir(cstr path);
	cstr getWorkDir() const noexcept;
	cstr getName() const noexcept { return name; }

	char name[8] = "";
	cstr workdir = nullptr; // full path, may be nullptr for rootdir!

protected:
	cstr makeFullPath(cstr path);
	FileSystem(cstr name) throws;
};


} // namespace kio::Devices

/*































*/
