// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FileSystem.h"
#include "ff15/source/ff.h"

namespace kio::Devices
{

class FatFS final : public FileSystem
{
public:
	virtual uint64		 getFree() override;
	virtual uint64		 getSize() override;
	virtual DirectoryPtr openDir(cstr path) override;
	virtual FilePtr		 openFile(cstr path, FileOpenMode flags = READ) override;

	virtual FileType getFileType(cstr path) noexcept override;
	virtual void	 makeDir(cstr path) throws override;
	virtual void	 remove(cstr path) throws override;
	virtual void	 rename(cstr path, cstr name) throws override;
	virtual void	 setFmode(cstr path, FileMode mode, uint8 mask) throws override;
	virtual void	 setMtime(cstr path, uint32 mtime) throws override;

private:
	FATFS fatfs;

	friend class FileSystem;
	friend class FatDir;
	friend class FatFile;
	friend class RCPtr<FatFS>;
	friend FileSystemPtr mount(cstr, BlockDevicePtr);
	friend FileSystemPtr mount(cstr);
	friend void			 makeFS(BlockDevicePtr, cstr);

	static void mkfs(BlockDevice*, int volume_idx, cstr type = "FAT");
	FatFS(cstr name, BlockDevicePtr) throws;
	virtual ~FatFS() override;
};

using FatFSPtr = RCPtr<FatFS>;

} // namespace kio::Devices


extern cstr tostr(FRESULT) noexcept;


#if 0

FatFs provides various filesystem functions for the applications as shown below.

    File Access
        f_open		-> FatDir.open() -> FatFile ctor
        f_close		-> FatFile.close() and dtor
        f_read		-> FatFile.read() etc.
        f_write		-> FatFile.write() etc. 
        f_lseek		-> FatFile.setFpos() 
        f_truncate	-> FatFile.truncate() 
        f_sync		-> FatFile.ioctl() 
        f_forward	-  Forward data to the stream
        f_expand	-  Allocate a contiguous block to the file
        f_gets		-> Not Used
        f_putc		-> Not Used
        f_puts		-> Not Used
        f_printf	-> Not Used
        f_tell		-> FatFile.getFpos()
        f_eof		-> Not used. File.eof() basically does exactly the same.
        f_size		-> FatFile.getSize()
        f_error		-  Test for an error
    Directory Access
        f_opendir	-> FatFS.opendir()
        f_closedir	-  FatDir.close() and dtor
        f_readdir	-  FatDir.next()
        f_findfirst	-  Not Used - FatDir.rewind() and find(pattern)
        f_findnext	-  Not Used - FatDir.find(pattern)
    File and Directory Management
        f_stat		-> FatDir.getInfo()
        f_unlink	-  FatDir.remove()
        f_rename	-> FatDir.rename()
        f_chmod		-> FatDir.setFmode()
        f_utime		-> FatDir.setMtime()
        f_mkdir		-> FatDir.makeDir()
        f_chdir		-> Not Used - FatFS.setcwd()
        f_chdrive	-> Not Used - FatFS.setcwd()
        f_getcwd	-> Not Used - FatFS.getcwd()
    Volume Management and System Configuration
        f_mount		-> FatFS ctor and dtor, FatFS.mount() 
        f_mkfs		-> FatFS.mkfs()
        f_fdisk		-> Not Used
        f_getfree	-> FatFS.getFree()
        f_getlabel	-> Not Used
        f_setlabel	-> Not Used
        f_setcp		-> Not Used. using fixed cp850 Latin-1 instead.

#endif


/*






















*/
