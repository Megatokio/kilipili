// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BlockDevice.h"
#include "common/Array.h"

extern const unsigned char resource_file_data[] __weak_symbol;

namespace kilipili
{

// =================================================== //
//			global methods without object			   //
// =================================================== //

/*	get a list of all known devices
	typical list is: {"rsrc","flash","sdcard"}
	on the emulator it might look more like:
	{"rsrc","flash","disk","HD1","HD4","usb-128gb"}
	removable media are listed even if they are not inserted.
*/
extern Array<cstr> getListOfDevices();

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
	- "sdcard2" the second SDCard (if present)
	- "rsrc"    the resource file system
	- "flash"   the internal program flash
	- "disk"	the host file system (on the emulator)
	Normally not needed because the FS functions do this automatically.
	But you may store the returned FileSystemPtr to keep the FileSystem mounted.
	setWorkDevice() keeps the FileSystem mounted too.
*/
extern FileSystemPtr getFS(cstr full_or_partial_path) throws;
extern FileSystemPtr mountFS(cstr name) throws;
extern FileSystemPtr mountFS(cstr name, BlockDevicePtr device) throws;

// needed by FatFS:
extern int index_of(FileSystem*) noexcept;

/*	Unmount a FileSystem.
	Normally not needed because the FS does not keep the FileSystems mounted
	and they are automatically unmounted on exit of each function.
	Only the current work dir is kept mounted.
	Therefore the only thing `unmount()` does is to check if it is the cwd.
	To really unmount a FS you must also release all FileSystemPtr's you may have yourself.
	You may check the retain count `FileSystem::rc`.
*/
extern void unmountFS(FileSystem*);
extern void unmountAll();

/*	Set the current working directory.
	The cwd keeps the FileSystem mounted.
*/
extern void			 setWorkDevice(FileSystemPtr);
extern FileSystemPtr getWorkDevice();		// get device or nullptr
extern void			 setWorkDir(cstr path); //
extern cstr			 getWorkDir();			// get cwd path or nullptr

/*	resolve the directory part of a filepath (up to the last '/').
	only actual directory files are considered for matching a pattern.
	if mkdir=true then the path should better not contain wildcards.	
*/
str resolveDirPath(cstr path, bool mkdir = false);

/*	resolve the directory part of a filepath (up to the last '/')
	and split of the last part as the pattern (with or without wildcards)
*/
cstr resolveDirPathWithPattern(cstr path, cstr* fnpattern_out);

/*	resolve the directory part of a filepath (up to the last '/')
	and try to resolve the last path component as a filename.
	set ftype to the resulting FileType which may be NoFile.
*/
cstr resolveFilePath(cstr path, FileType* filetype_out);

/*	resolve the directory part of a filepath (all components up to the last '/')
	and resolve the last path component as a file of the requested FileType.
	the requested filetype may be NoFile: but this may also fail if a similarly named file exists.
*/
cstr resolveFilePath(cstr path, FileType requested_filetype = RegularFile);

/* 	Open, create or manipulate files and directories.
	note: FilePtr and DirectoryPtr keep their FileSystem mounted.
	note: these non-FS versions all resolve wildcards in the path and fname
*/
extern FilePtr		openFile(cstr path, FileOpenMode flags = READ) throws; // creates dirpath if flags=CREATE
extern DirectoryPtr openDir(cstr path) throws;
extern void			makeDir(cstr path) throws; // creates the entire path
extern void			removeFile(cstr path_and_pattern) throws;
extern void			copyFile(cstr source_path_and_pattern, cstr dest_path) throws;
extern ADDR			getFileSize(cstr path) throws;
extern FileType		getFileType(cstr path) noexcept;
inline bool			isaDirectory(cstr path) noexcept { return getFileType(path) == FileType::DirectoryFile; }
inline bool			isaFile(cstr path) noexcept { return getFileType(path) == FileType::RegularFile; }


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
	virtual ADDR	 getFileSize(cstr path)								   = 0;

	void setWorkDir(cstr path);
	cstr getWorkDir() const noexcept;
	cstr getName() const noexcept { return name; }

	char name[8] = "";
	cstr workdir = nullptr; // full path, may be nullptr for rootdir!

	/* resolve the directory part of a filepath (up to the last '/').
	   only actual directory files are considered for matching a pattern.
	   if mkdir=true then the path should better not contain wildcards.	
	*/
	str resolveDirPath(cstr path, bool mkdir = false);

	/* resolve the directory part of a filepath (up to the last '/')
	   and split of the last part as the pattern (with or without wildcards)
	*/
	str resolveDirPathWithPattern(cstr path, cstr* fnpattern_out);

	/* resolve the directory part of a filepath (up to the last '/')
	   and try to resolve the last path component as a filename.
	   set ftype to the resulting FileType which may be NoFile.
	*/
	str resolveFilePath(cstr path, FileType* filetype_out);

	/* resolve the directory part of a filepath (all components up to the last '/')
	   and resolve the last path component as a file of the requested FileType.
	   requesting NoFile is possible but not recommended: this may also fail if a similar filename exists.
	*/
	str resolveFilePath(cstr path, FileType requested_filetype = RegularFile);

protected:
	friend class FatDir;
	bool is_canonical_fullpath(cstr path, bool allow_trailing_slash = false) noexcept;
	bool is_resolved_fullpath(cstr path, bool allow_final_slash = false);
	cstr makeFullPath(cstr path, bool keep_trailing_slash = false);
	FileSystem(cstr name) throws;
};


} // namespace kilipili

/*































*/
