// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FatFile.h"
#include "FatFS.h"
#include "Logger.h"
#include "cstrings.h"
#include "cdefs.h"

namespace kio::Devices
{

static Flags fileflagsfrommode(FileOpenMode mode)
{
	constexpr uint m_readable = FileOpenMode::READ;
	constexpr uint m_writable = FileOpenMode::WRITE;

	constexpr uint f_readable = Flags::readable;
	constexpr uint f_writable = Flags::writable;

	uint flags = ((mode & m_readable) ? f_readable : 0) | ((mode & m_writable) ? f_writable : 0);
	return Flags(flags);
}

FatFile::FatFile(FatFSPtr device, cstr path, FileOpenMode mode) : //
	File(fileflagsfrommode(mode)),
	device(device)
{
	FRESULT err = f_open(&fatfile, catstr(device->name, ":", path), mode);
	if (err) throw tostr(err);
}

uint32 FatFile::ioctl(IoCtl cmd, void* arg1, void* arg2)
{
	switch (cmd.cmd)
	{
	case IoCtl::CTRL_SYNC:
		if (FRESULT err = f_sync(&fatfile)) throw tostr(err);
		return 0;
	default: return File::ioctl(cmd, arg1, arg2);
	}
}

void FatFile::truncate()
{
	FRESULT err = f_truncate(&fatfile);
	if (err) throw tostr(err);
}

SIZE FatFile::read(void* data, SIZE size, bool partial)
{
	SIZE	count = 0;
	FRESULT err	  = f_read(&fatfile, data, size, &count);
	if unlikely (err) throw tostr(err);
	if unlikely (count < size && !partial) throw END_OF_FILE;
	return count;
}

SIZE FatFile::write(const void* data, SIZE size, bool partial)
{
	SIZE	count = 0;
	FRESULT err	  = f_write(&fatfile, data, size, &count);
	if unlikely (err) throw tostr(err);
	if unlikely (count < size && !partial) throw END_OF_FILE;
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
	if unlikely (err) throw tostr(err);
	if unlikely (count == 0) throw END_OF_FILE;
	return last_char;
}

SIZE FatFile::putc(char c)
{
	SIZE	count = 0;
	FRESULT err	  = f_write(&fatfile, &c, 1, &count);
	if unlikely (err) throw tostr(err);
	if unlikely (count < 1) throw END_OF_FILE;
	return count;
}

ADDR FatFile::getSize() const noexcept
{
	return f_size(&fatfile); //
}

ADDR FatFile::getFpos() const noexcept
{
	return f_tell(&fatfile); //
}

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
