// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

#include "File.h"
#include "ff15/source/ff.h"

namespace kio::Devices
{

static_assert(sizeof(ADDR) == sizeof(FSIZE_t));

extern cstr tostr(FRESULT) noexcept;


class FatFile : public File
{
	FIL fatfile;

public:
	enum Mode : uint8 {
		READ		  = FA_READ,		  // Open for reading
		WRITE		  = FA_WRITE,		  // Open for writing. Combine with READ for read-write access.
		EXIST		  = FA_OPEN_EXISTING, // File must already exist. (Default)
		NEW			  = FA_CREATE_NEW,	  // File must not exist.
		OPEN_TRUNCATE = FA_CREATE_ALWAYS, // Create file if not exist. Truncate existing file.
		OPEN_PRESERVE = FA_OPEN_ALWAYS,	  // Create file if not exist. Don't truncate existing file.
		OPEN_APPEND	  = FA_OPEN_APPEND,	  // Same as OPEN_PRESERVE except fpos is set to end of file.
	};
	/*	POSIX fopen() flags vs. FatFs mode flags:
		"r"		FA_READ
		"r+"	FA_READ | FA_WRITE
		"w"		FA_CREATE_ALWAYS | FA_WRITE
		"w+"	FA_CREATE_ALWAYS | FA_WRITE | FA_READ
		"a"		FA_OPEN_APPEND | FA_WRITE
		"a+"	FA_OPEN_APPEND | FA_WRITE | FA_READ
		"wx"	FA_CREATE_NEW | FA_WRITE
		"w+x"	FA_CREATE_NEW | FA_WRITE | FA_READ
	*/

	FatFile(cstr fpath, uint8 mode);
	~FatFile() override;

	// *** Interface: ***

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;

	virtual SIZE read(char* data, SIZE, bool partial = false) override;
	virtual SIZE write(const char* data, SIZE, bool partial = false) override;

	virtual int	 getc(uint timeout_us) override;
	virtual char getc() override;
	virtual SIZE putc(char) override;
	//virtual str  gets() override;
	//virtual SIZE puts(cstr) override;
	//virtual SIZE printf(cstr fmt, ...) override __printflike(2, 3);

	virtual ADDR getSize() const noexcept override;
	virtual ADDR getFpos() const noexcept override;
	virtual void setFpos(ADDR) override;
	virtual void close(bool force = true) override;
	virtual void truncate() override;
};

} // namespace kio::Devices
