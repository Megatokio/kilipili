// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BlockDevice.h"
#include "File.h"
#include "basic_math.h"

namespace kio::Devices
{

/*
	BlockDeviceFile is a concrete implementation of the interface class File.	
	It wraps a BlockDevice in order to support sequential reading and writing on that device.
*/

class BlockDeviceFile : public File
{
	RCPtr<BlockDevice> bdev;
	const ADDR		   fsize;
	ADDR			   fpos = 0;


public:
	BlockDeviceFile(RCPtr<BlockDevice>) noexcept;
	virtual ~BlockDeviceFile() noexcept override = default;

	// *** Interface: ***

	virtual uint32 ioctl(IoCtl ctl, void* arg1 = nullptr, void* arg2 = nullptr) override;

	// SerialDevice:

	virtual SIZE read(void* data, SIZE, bool partial = false) override;
	virtual SIZE write(const void* data, SIZE, bool partial = false) override;

	// virtual int  getc(uint timeout_us) override;
	// virtual char getc() override;
	// virtual SIZE putc(char) override;
	// virtual str	gets() override;
	// virtual SIZE puts(cstr);
	// virtual SIZE printf(cstr fmt, ...) __printflike(2, 3);

	// File:

	virtual ADDR getSize() const noexcept override { return fsize; }
	virtual ADDR getFpos() const noexcept override { return fpos; }
	virtual void setFpos(ADDR new_fpos) override
	{
		clear_eof_pending();
		fpos = min(new_fpos, fsize);
	}
	virtual void close() override;
	//virtual void truncate();
};


} // namespace kio::Devices
