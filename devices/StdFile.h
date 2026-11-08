// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "File.h"

/*
	class StdFile represents a regular file in the normal filesystem of the host system.
	it can be created in two ways:

	- indirectly by calling the FileSystem function `openFile("disk:/some/path",mode)` 
	  then the file uses 1-byte character encoding for path and filename,
	  currently always latin-1, the standard encoding on the RP Pico.
	  StdFile is used in the desktop emulator only, not on the Pico itself.

	- directly by creating it with `new StdFile("/some/path",mode)`
	  then the file uses utf-8 character encoding for path and filename,
	  assuming this is not the emulator but some desktop tool.

	currently it can also be used for stdio, but this will be replaced by support in StdIO. (TODO)
*/

namespace kilipili
{

class StdFile : public File
{
	NO_COPY_MOVE(StdFile);

public:
	StdFile(cstr fpath_utf8, FileOpenMode = READ);
	StdFile(FILE*, FSFlags); // intended for stdin|out|err: file will not be closed in dtor

	virtual ~StdFile() override;
	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;

	virtual SIZE read(void* data, SIZE, bool partial = false) override;
	virtual SIZE write(const void* data, SIZE, bool partial = false) override;

	using File::read;
	using File::write;

	virtual int	 getc(uint timeout_us) override;
	virtual char getc() override;
	// virtual str	gets();
	virtual void putc(char) override;
	// virtual SIZE puts(cstr)override;
	// virtual SIZE printf(cstr fmt, ...) __printflike(2, 3);

	virtual ADDR getSize() const noexcept override;
	virtual ADDR getFpos() const noexcept override;
	virtual void setFpos(ADDR) override;
	virtual void close() override;
	virtual void truncate() override { throw "truncate() not supported"; }

private:
	FILE* file;
	void  throw_error();
};


} // namespace kilipili
