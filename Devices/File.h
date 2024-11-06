// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "SerialDevice.h"


namespace kio::Devices
{

/* Interface class `File`
*/
class File : public SerialDevice
{
protected:
	File(Flags flags) noexcept : SerialDevice(flags) {}

public:
	virtual ~File() override	 = default;
	File(const File&) noexcept	 = default;
	File(File&&) noexcept		 = default;
	File& operator=(const File&) = delete;
	File& operator=(File&&)		 = delete;

	virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) override;

	// virtual SIZE read(char* data, SIZE, bool partial = false);
	// virtual SIZE write(const char* data, SIZE, bool partial = false);

	// virtual int	getc(uint timeout_us);
	// virtual char getc();
	// virtual str	gets();
	// virtual SIZE putc(char);
	// virtual SIZE puts(cstr);
	// virtual SIZE printf(cstr fmt, ...) __printflike(2, 3);

	// close(force=true) close the file even if a file error occurs.
	// set_fpos()        may set fpos beyond file end if this is possible.

	virtual ADDR getSize() const noexcept = 0;
	virtual ADDR getFpos() const noexcept = 0;
	virtual void setFpos(ADDR)			  = 0;
	virtual void close(bool force = true) = 0;
	virtual void truncate() { throw "truncate() not supported"; }

	bool is_eof() const noexcept { return getFpos() >= getSize(); }
};


} // namespace kio::Devices


/*






























*/
