// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFile.h"
#include <cstring>

namespace kio ::Devices
{

RsrcFile::RsrcFile(const uchar* data, uint32 size) : //
	File(readable),
	data(data),
	fsize(size)
{}

void RsrcFile::setFpos(ADDR new_fpos)
{
	clear_eof_pending();
	if unlikely (new_fpos > fsize) fpos = fsize;
	else fpos = uint32(new_fpos);
}

SIZE RsrcFile::read(void* data, SIZE size, bool partial)
{
	if unlikely (size > fsize - fpos)
	{
		size = fsize - fpos;
		if (!partial) throw END_OF_FILE;
		if (eof_pending()) throw END_OF_FILE;
		if (size == 0) set_eof_pending();
	}
	memcpy(data, this->data + fpos, size);
	fpos += size;
	return size;
}


} // namespace kio::Devices

/*





























*/
