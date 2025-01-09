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


/* 	Mount the well-known device with name `devicename`.
	Currently well-known are:
	- "sdcard"  the default SDCard
	- "rsrc"    the resource file system
	Store the returned FileSystemPtr to keep the FileSystem alive.
	setWorkDevice() keeps the FileSystem alive too.
	Same as FileSystem::mount(devicename).
*/
extern FileSystemPtr mount(cstr devicename);

/*	Unmount the device with name `devicename`.
	Basically this clears the current work device, if it is this device:
	the file system has no other locks on a FileSystem.
	FileSystemPtrs in the application, FilePtrs and DirectoryPtrs keep a FileSystem alive.
	To see how many locks there are on a FileSystem inspect the reference count FileSystem->rc. 
*/
extern void unmount(FileSystemPtr);

/* 	Open a file or directory.
	If the path starts with a device name followed by a colon then the path is searched on
	that device, else the current working device is used.
	If, after that, the path starts with '/' then it is an absolute path on that device,
	else the current working directory of that device is prepended.
	FilePtrs and DirectoryPtrs keep their FileSystem alive.
	File::close() should release the lock but better also reset the FilePtr to free the File object.
*/
extern FilePtr		openFile(cstr path, FileOpenMode flags = READ) throws;
extern DirectoryPtr openDir(cstr path) throws;

/*	Set a current work device.
	The working device keeps the FileSystem alive.
	unmount() or setWorkDevice(nullptr) clears the working device.
*/
extern void			 setWorkDevice(FileSystemPtr);
extern FileSystemPtr getWorkDevice();	   // get device or nullptr
extern FileSystemPtr getDevice(cstr name); // get device or nullptr

/*	Set the current work directory:
	If the path starts with a device name followed by a colon then the path is searched on
	that device, else the current working device is used.
	If, after that, the path starts with '/' then it is an absolute path on that device,
	else the current working directory of that device is prepended.
	Examples are: "A:", "A:/foo", "A:foo", "/foo", "foo"
	getWorkDir() returns the current work directory path incl. device or nullptr if there is none.
*/
extern void setWorkDir(cstr path);
extern cstr getWorkDir();

extern cstr makeAbsolutePath(cstr path);
extern void unmountAll();

/*	openDir(path) and openFile(path):
		path must start with "someDevice:"
		the path without the device is used for the FileSystem function.
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
	virtual ~FileSystem() noexcept override;

	/*	Mount supplied device and register it as device `name`.
		Normally not needed but may be used to check FS beforehand
		or to keep it in memory to prevent repeated unmount-mount cycles.
	*/
	static FileSystemPtr mount(cstr name, BlockDevicePtr device) throws;

	/*	Mount the well-known named device.
		well-known are currently:
		- "sdcard"  the default SDCard
		- "rsrc"    the resource file system
	*/
	static FileSystemPtr mount(cstr devicename) throws;

	/*	Create filesystem on the supplied BlockDevice.
		Type "FAT" creates the default variant of FAT for the disk size.
		Other filesystems are TODO.
		The BlockDevice should not be in use by a FileSystem.
	*/
	static void makeFS(BlockDevicePtr, cstr type = "FAT") throws;

	/*	Get total size and free space.
		If exact calculation of free space is expensive return a lower limit.
	*/
	virtual ADDR getFree() = 0;
	virtual ADDR getSize() = 0;

	/*	Get and open directory.
		Get and open regular file.
	*/
	virtual DirectoryPtr openDir(cstr path)								= 0;
	virtual FilePtr		 openFile(cstr path, FileOpenMode flags = READ) = 0;

	/* Manage a working directory:
	*/
	void setWorkDir(cstr path);
	cstr getWorkDir() const noexcept { return workdir ? workdir : "/"; }
	cstr makeAbsolutePath(cstr path); // utility for subclasses


	char name[8] = "";
	cstr workdir = nullptr; // allocated

protected:
	FileSystem(cstr name) throws;

private:
	virtual bool mount() throws = 0;
};


} // namespace kio::Devices

/*































*/
