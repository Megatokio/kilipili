// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FatFile.h"
#include "Logger.h"
#include "kilipili_cdefs.h"

namespace kio::Devices
{

static const cstr ff_errors[] = {
	/*  FR_OK,					*/ "Success",
	/*  FR_DISK_ERR,			*/ "A hard error occurred in the low level disk I/O layer",
	/*  FR_INT_ERR,				*/ "Assertion failed",
	/*  FR_NOT_READY,			*/ "The physical drive cannot work",
	/*  FR_NO_FILE,				*/ "Could not find the file",
	/*  FR_NO_PATH,				*/ "Could not find the path",
	/*  FR_INVALID_NAME,		*/ "The path name format is invalid",
	/*  FR_DENIED,				*/ "Access denied due to prohibited access or directory full",
	/*  FR_EXIST,				*/ "Access denied due to prohibited access",
	/*  FR_INVALID_OBJECT,		*/ "The file/directory object is invalid",
	/*  FR_WRITE_PROTECTED,		*/ "The physical drive is write protected",
	/*  FR_INVALID_DRIVE,		*/ "The logical drive number is invalid",
	/*  FR_NOT_ENABLED,			*/ "The volume has no work area",
	/*  FR_NO_FILESYSTEM,		*/ "There is no valid FAT volume",
	/*  FR_MKFS_ABORTED,		*/ "The f_mkfs() aborted due to any problem",
	/*  FR_TIMEOUT,				*/ TIMEOUT, //"Could not get a grant to access the volume within defined period",
	/*  FR_LOCKED,				*/ "The operation is rejected according to the file sharing policy",
	/*  FR_NOT_ENOUGH_CORE,		*/ "LFN working buffer could not be allocated",
	/*  FR_TOO_MANY_OPEN_FILES, */ "Number of open files > FF_FS_LOCK",
	/*  FR_INVALID_PARAMETER	*/ INVALID_ARGUMENT, //"Given parameter is invalid",
};

cstr tostr(FRESULT err) noexcept
{
	if (err < NELEM(ff_errors)) return ff_errors[err];
	else return "FatFS unknown error";
}

static Flags fileflagsfrommode(uint mode)
{
	constexpr uint m_readable = FatFile::Mode::READ;
	constexpr uint m_writable = FatFile::Mode::WRITE;

	constexpr uint f_readable = Flags::readable;
	constexpr uint f_writable = Flags::writable;

	uint flags = ((mode & m_readable) ? f_readable : 0) | ((mode & m_writable) ? f_writable : 0);
	return Flags(flags);
}

FatFile::FatFile(cstr fpath, uint8 mode) : File(fileflagsfrommode(mode))
{
	FRESULT err = f_open(&fatfile, fpath, mode);
	if (err) throw tostr(err);
}

uint32 FatFile::ioctl(IoCtl cmd, void*, void*)
{
	switch (cmd.cmd)
	{
	case IoCtl::CTRL_SYNC:
		if (FRESULT err = f_sync(&fatfile)) throw tostr(err);
		return 0;
	default: throw INVALID_ARGUMENT;
	}
}

void FatFile::truncate()
{
	FRESULT err = f_truncate(&fatfile);
	if (err) throw tostr(err);
}

SIZE FatFile::read(char* data, SIZE size, bool partial)
{
	SIZE	count = 0;
	FRESULT err	  = f_read(&fatfile, data, size, &count);
	if (err) throw tostr(err);
	if (count < size && !partial) throw END_OF_FILE;
	return count;
}

SIZE FatFile::write(const char* data, SIZE size, bool partial)
{
	SIZE	count = 0;
	FRESULT err	  = f_write(&fatfile, data, size, &count);
	if (err) throw tostr(err);
	if (count < size && !partial) throw END_OF_FILE;
	return count;
}

int FatFile::getc(uint __unused timeout_us)
{
	if (fatfile.fptr < fatfile.obj.objsize) return getc();
	else return -1;
}

char FatFile::getc()
{
	SIZE	count = 0;
	FRESULT err	  = f_read(&fatfile, &last_char, 1, &count);
	if (unlikely(err)) throw tostr(err);
	if (unlikely(count == 0)) throw END_OF_FILE;
	return last_char;
}

SIZE FatFile::putc(char c)
{
	SIZE	count = 0;
	FRESULT err	  = f_write(&fatfile, &c, 1, &count);
	if (err) throw tostr(err);
	if (count < 1) throw END_OF_FILE;
	return count;
}

ADDR FatFile::getSize() const noexcept
{
	return f_size(&fatfile); //
}

ADDR FatFile::getFpos() const noexcept { return f_tell(&fatfile); }

void FatFile::setFpos(ADDR addr)
{
	FRESULT err = f_lseek(&fatfile, addr);
	if (err) throw tostr(err);
}

void FatFile::close(bool __unused force)
{
	// the docs don't tell what to if close fails.
	// we assume that the file handle has become invalid
	// and we can dispose of it.

	FRESULT err = f_close(&fatfile);
	if (err) throw tostr(err);
}

FatFile::~FatFile()
{
	if (fatfile.obj.fs != nullptr) // else the file is closed
	{
		FRESULT err = f_close(&fatfile);
		if (err) logline(tostr(err));
	}
}

} // namespace kio::Devices

/*











































*/
