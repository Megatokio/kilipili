// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FatFile.h"
#include "FatFS.h"
#include "Trace.h"
#include "cdefs.h"
#include "cstrings.h"

namespace kio::Devices
{

uint8 fatFS_mode_for_File_mode(FileOpenMode m)
{
	static_assert(FileOpenMode::READ == 1 + 16);
	static_assert(FileOpenMode::WRITE == 2 + 32);
	static_assert(FileOpenMode::APPEND == 2 + 4);

	static_assert(FA_READ == 1);
	static_assert(FA_WRITE == 2);

	// not WRITE => open for reading only
	if ((m & 2) == 0) return FA_READ + FA_OPEN_EXISTING;

	// WRITE or READWRITE:
	uint8 ff_mode = m & 3;								// READ and WRITE bits
	if (m & 4) m = m - TRUNCATE;						// sanitize
	if (m & 4) ff_mode |= FA_OPEN_APPEND;				// write, append
	if (m & NEW) ff_mode |= FA_CREATE_NEW;				// new
	else if (m & TRUNCATE) ff_mode |= FA_CREATE_ALWAYS; // exist|new, truncate
	else if (m & EXIST) ff_mode |= FA_OPEN_EXISTING;	// exist, !truncate
	else ff_mode |= FA_OPEN_ALWAYS;						// exist|new, !truncate

	return ff_mode;
}

FatFile::FatFile(FatFSPtr device, cstr path, FileOpenMode mode) : //
	File(mode),
	device(device)
{
	trace(__func__);

	FRESULT err = f_open(&fatfile, catstr(device->name, ":", path), fatFS_mode_for_File_mode(mode));
	if (err) throw tostr(err);
}

uint32 FatFile::ioctl(IoCtl cmd, void* arg1, void* arg2)
{
	trace(__func__);

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
	trace(__func__);

	FRESULT err = f_truncate(&fatfile);
	if (err) throw tostr(err);
}

SIZE FatFile::read(void* data, SIZE size, bool partial)
{
	trace(__func__);

	SIZE	count = 0;
	FRESULT err	  = f_read(&fatfile, data, size, &count);
	if unlikely (err) throw tostr(err);
	if unlikely (count < size)
	{
		if (!partial) throw END_OF_FILE;
		if (eof_pending()) throw END_OF_FILE;
		if (count == 0) set_eof_pending();
	}
	return count;
}

SIZE FatFile::write(const void* data, SIZE size, bool partial)
{
	trace(__func__);

	SIZE	count = 0;
	FRESULT err	  = f_write(&fatfile, data, size, &count);
	if unlikely (err) throw tostr(err);
	if unlikely (count < size && !partial) throw END_OF_FILE;
	return count;
}

int FatFile::getc(uint __unused timeout_us)
{
	trace(__func__);

	if (fatfile.fptr < fatfile.obj.objsize) return FatFile::getc();
	if (eof_pending()) throw END_OF_FILE;
	set_eof_pending();
	return -1;
}

char FatFile::getc()
{
	trace(__func__);

	SIZE	count = 0;
	FRESULT err	  = f_read(&fatfile, &last_char, 1, &count);
	if unlikely (err) throw tostr(err);
	if unlikely (count == 0) throw END_OF_FILE;
	return last_char;
}

SIZE FatFile::putc(char c)
{
	trace(__func__);

	SIZE	count = 0;
	FRESULT err	  = f_write(&fatfile, &c, 1, &count);
	if unlikely (err) throw tostr(err);
	if unlikely (count < 1) throw END_OF_FILE;
	return count;
}

ADDR FatFile::getSize() const noexcept
{
	trace(__func__);

	if constexpr (sizeof(ADDR) >= sizeof(FSIZE_t)) return f_size(&fatfile);
	FSIZE_t fsize = f_size(&fatfile);
	if (fsize == ADDR(fsize)) return ADDR(fsize);
	debugstr("FatFile: file size exceeds 4GB\n");
	return 0xffffffffu;
}

ADDR FatFile::getFpos() const noexcept
{
	trace(__func__);

	if constexpr (sizeof(ADDR) >= sizeof(FSIZE_t)) return f_tell(&fatfile);
	FSIZE_t fpos = f_tell(&fatfile);
	if (fpos == ADDR(fpos)) return ADDR(fpos);
	debugstr("FatFile: file position beyond 4GB\n");
	return 0xffffffffu;
}

void FatFile::setFpos(ADDR addr)
{
	trace(__func__);

	clear_eof_pending();
	FRESULT err = f_lseek(&fatfile, addr);
	if (err) throw tostr(err);
}

void FatFile::close()
{
	// the docs don't tell what to if close fails.
	// we assume that the file handle has become invalid
	// and we can dispose of it.

	trace(__func__);

	FRESULT err = f_close(&fatfile);
	device		= nullptr;
	if (err) throw tostr(err);
}

FatFile::~FatFile() noexcept
{
	trace(__func__);

	if (device) // else the file is closed
	{
		FRESULT err = f_close(&fatfile);
		if (err) debugstr("%s", tostr(err));
	}
}

} // namespace kio::Devices

/*











































*/
