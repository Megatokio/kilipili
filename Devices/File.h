// Copyright (c) 2023 - 2023 kio@little-bat.de
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


	// ------- Convenience Methods ---------------

	using super = SerialDevice;
	using super::read;

	template<typename T>
	T read()
	{
		T n;
		read(ptr(&n), sizeof(T));
		return n;
	}
	int8   read_int8() { return read<int8>(); }
	uint8  read_uint8() { return read<uint8>(); }
	char   read_char() { return read<char>(); }
	uchar  read_uchar() { return read<uchar>(); }
	int16  read_int16() { return read<int16>(); }
	uint16 read_uint16() { return read<uint16>(); }
	int32  read_int32() { return read<int32>(); }
	uint32 read_uint32() { return read<uint32>(); }
	int64  read_int64() { return read<int64>(); }
	uint64 read_uint64() { return read<uint64>(); }

	SIZE write_int8(int8 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_uint8(uint8 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_char(char n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_uchar(uchar n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_int16(int16 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_uint16(uint16 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_int32(int32 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_uint32(uint32 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_int64(int64 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_uint64(uint64 n) { return write(cptr(&n), sizeof(n)); }
	SIZE write_nl() { return write_char('\n'); }
};


} // namespace kio::Devices
