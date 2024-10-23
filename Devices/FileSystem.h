// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BlockDevice.h"

namespace kio::Devices
{

/*	note:
	Devices::openDir(path) and openFile(path):
		path must start with "someDevice:"
		the path without the device is used for a call to the FileSystem function.
	FileSystem::openDir(path) and openFile(path):
		absolute path "/" starts at root directory.
		relative path starts at the current workdir.
	Directory::openDir(path) and openFile(path):
		absolute path "/" starts at root directory.
		relative path starts in this directory.
		relative path may start with "./" and "../".		
*/

class FileSystem : public RCObject
{
public:
	/* —————————————————————————————————————————————
		Mount supplied device and register it as device `name`.
		Normally not needed but may be used to check FS beforehand
		or to keep it in memory to prevent repeated unmount-mount cycles.
	*/
	static FileSystemPtr mount(cstr name, BlockDevicePtr device) throws;

	/* —————————————————————————————————————————————
		Mount the well-known named device.
		well-known are currently:
		- "sdcard"  the default SDCard
		- "rsrc"    the resource file system
	*/
	static FileSystemPtr mount(cstr devicename) throws;

	/* —————————————————————————————————————————————
		Create filesystem on the supplied device.
		Type "FAT" creates the default variant of FAT for the disk size.
		Other filesystems are TODO.
		The device should not be mounted.
	*/
	static void makeFS(BlockDevicePtr, cstr type = "FAT") throws;

	/* —————————————————————————————————————————————
		Get total size and free space.
		If exact calculation of free space is expensive return a lower limit.
	*/
	virtual ADDR getFree() = 0;
	virtual ADDR getSize() = 0;

	/* —————————————————————————————————————————————
		Get and open directory.
		Get and open regular file.
	*/
	virtual DirectoryPtr openDir(cstr path)								= 0;
	virtual FilePtr		 openFile(cstr path, FileOpenMode flags = READ) = 0;

	/* ——————————————————————————————————————————————
		Manage a working directory.
	*/
	void setWorkDir(cstr path);
	cstr getWorkDir() const noexcept { return workdir; }
	cstr makeAbsolutePath(cstr path); // utility for subclasses


	char name[8] = "";
	cstr workdir = nullptr; // allocated

protected:
	FileSystem(cstr name) throws;
	virtual ~FileSystem() noexcept override;

private:
	virtual bool mount() throws = 0;
};


// =================================================== //
//			global methods without object			   //
// =================================================== //


/* ————————————————————————————————————————————————————
	Open a file or directory.
	The path must start with a device name followed by a colon.
	After that the name may be absolute or relative.
	Relative paths are appended to the current workdir of the device.
*/
extern FilePtr		openFile(cstr path, FileOpenMode flags = READ) throws;
extern DirectoryPtr openDir(cstr path) throws;

} // namespace kio::Devices

/*































*/
