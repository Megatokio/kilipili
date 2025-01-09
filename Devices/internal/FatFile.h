// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

#include "File.h"
#include "ff15/source/ff.h"

namespace kio::Devices
{

class FatFS;
using FatFSPtr = RCPtr<FatFS>;


class FatFile : public File
{
public:
	~FatFile() noexcept override;

	// *** Interface: ***

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;

	virtual SIZE read(void* data, SIZE, bool partial = false) override;
	virtual SIZE write(const void* data, SIZE, bool partial = false) override;

	virtual int	 getc(uint timeout_us) override;
	virtual char getc() override;
	virtual SIZE putc(char) override;
	//virtual str  gets() override;
	//virtual SIZE puts(cstr) override;
	//virtual SIZE printf(cstr fmt, ...) override __printflike(2, 3);

	virtual ADDR getSize() const noexcept override;
	virtual ADDR getFpos() const noexcept override;
	virtual void setFpos(ADDR) override;
	virtual void close() override;
	virtual void truncate() override;

private:
	FatFSPtr device; // keep alive
	FIL		 fatfile;

	FatFile(FatFSPtr device, cstr path, FileOpenMode mode);
	friend class FatDir;
	friend class FatFS;
};

using FatFilePtr = RCPtr<FatFile>;

} // namespace kio::Devices

extern cstr tostr(FRESULT) noexcept;
