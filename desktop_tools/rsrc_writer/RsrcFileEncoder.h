// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Devices/File.h"
#include "standard_types.h"

namespace kio::Devices
{

/*	the RsrcFileEncoder encodes the input data into comma separated ascii encoded values.
	The user file data is preceded by the (encoded) 0-terminated file name.
	The stored file name is not accounted in fsize or fpos.

	data written by the RsrcFileEncoder:
	  char[] filename   0-terminated string
	  char[] data       data as written to the encoder

	data expected by the resource file system:

	uncompressed:
	  char[] filename   0-terminated string
	  uint32 usize
	  char[] data       uncompressed data

	compressed:
	  char[] filename   0-terminated string
	  uint32 usize		usize | 0x80000000
	  uint24 csize
	  uint8  cflags		wbits << 4 + lbits
	  char[] data       compressed data
*/
class RsrcFileEncoder : public File
{
public:
	RsrcFileEncoder(FilePtr file, cstr rsrc_fname);
	~RsrcFileEncoder() noexcept override;

	virtual SIZE write(const void* data, SIZE, bool partial = false) override;
	virtual ADDR getSize() const noexcept override { return std::max(fpos, fsize); }
	virtual ADDR getFpos() const noexcept override { return fpos; }
	virtual void setFpos(ADDR) override;
	void		 close() override;

private:
	FilePtr file;
	uint32	fpos0	 = 0; // start of file data
	uint32	fsize	 = 0; // size of file data
	uint32	fpos	 = 0; // write position in file data
	uint32	_padding = 0;
};


} // namespace kio::Devices
