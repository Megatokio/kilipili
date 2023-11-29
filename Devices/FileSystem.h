// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BlockDevice.h"
#include "Directory.h"

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
		Mount supplied device or the named device and register it as device `name`.
		Normally not needed but may be used to check FS beforehand
		or to keep it in memory to prevent repeated unmount-mount cycles.
	*/
	static FileSystemPtr mount(cstr name, BlockDevicePtr device) throws;
	static FileSystemPtr mount(cstr name) throws;

	/* —————————————————————————————————————————————
		Create filesystem on the supplied device or on the named device.
		Type "FAT" creates the default variant of FAT for the disk size.
		Other filesystems are TODO.
	*/
	static void makeFS(BlockDevicePtr, cstr type = "FAT") throws;
	static void makeFS(cstr devname, cstr type = "FAT") throws;

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


	char			   name[8] = "";
	RCPtr<BlockDevice> blkdev;
	cstr			   workdir = nullptr; // allocated

protected:
	FileSystem(cstr name, BlockDevice* blkdev) throws;
	virtual ~FileSystem() noexcept override;

private:
	virtual bool mount() throws = 0;
};


extern FileSystem* file_systems[];


// =================================================== //
//			global methods without object			   //
// =================================================== //


/* ————————————————————————————————————————————————————
	Open a file or directory.
	The path must start with a device name followed by a colon.
	After that the name may be absolute or relative.
	Relative paths are appended to the current workdir of the device.
*/
extern FilePtr		openFile(cstr path, FileOpenMode flags) throws;
extern DirectoryPtr openDir(cstr path) throws;

} // namespace kio::Devices

/*































*/
