// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "FileSystem.h"
#include "ff14b/ff.h"

namespace kio::Devices
{

extern cstr tostr(FRESULT) noexcept;


class FatFS : public FileSystem
{
	FATFS fatfs;

public:
	FatFS(BlockDevice& blkdev, cstr name);
	virtual ~FatFS() override;

	virtual void mkfs() override;
	virtual void mount() override;
	virtual ADDR getFree() override;
	virtual ADDR getSize() override;
};


} // namespace kio::Devices


#if 0

FatFs provides various filesystem functions for the applications as shown below.

    File Access
        f_open		-> FatFS.open() -> FatFile ctor
        f_close		-> FatFile.close and dtor
        f_read		-> FatFile.read() etc.
        f_write		-> FatFile.write() etc. 
        f_lseek		-> FatFile.set_fpos() 
        f_truncate	-> FatFile.truncate() 
        f_sync		-> FatFile.ioctl() 
        f_forward	-  Forward data to the stream
        f_expand	-  Allocate a contiguous block to the file
        f_gets		-> Not Used
        f_putc		-> Not Used
        f_puts		-> Not Used
        f_printf	-> Not Used
        f_tell		-> FatFile.get_fpos()
        f_eof		-> Not used. File.eof() basically does exactly the same.
        f_size		-> FatFile.get_size()
        f_error		-  Test for an error
    Directory Access
        f_opendir	-> FatFS.opendir()
        f_closedir	-  Close an open directory
        f_readdir	-  Read a directory item
        f_findfirst	-  Open a directory and read the first item matched
        f_findnext	-  Read a next item matched
    File and Directory Management
        f_stat		-> FatFS.getinfo()
        f_unlink	-  FatFS.remove()
        f_rename	-> FatFS.rename()
        f_chmod		-> FatFS.setmode()
        f_utime		-> FatFS.setmtime()
        f_mkdir		-> FatFS.mkdir()
        f_chdir		-> FatFS.setcwd()
        f_chdrive	-> FatFS.setcwd()
        f_getcwd	-> FatFS.getcwd()
    Volume Management and System Configuration
        f_mount		-> FatFS ctor and dtor, FatFS.mount() 
        f_mkfs		-> FatFS.mkfs()
        f_fdisk		-> Not Used
        f_getfree	-> FatFS.get_free()
        f_getlabel	-> Not Used
        f_setlabel	-> Not Used
        f_setcp		-> Not Used. using fixed cp850 Latin-1 instead.

#endif


/*






















*/
