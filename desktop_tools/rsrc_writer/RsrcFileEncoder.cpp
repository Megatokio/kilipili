// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFileEncoder.h"
#include "cstrings.h"

namespace kio::Devices
{

RsrcFileEncoder::RsrcFileEncoder(FilePtr file, cstr rsrc_fname, bool write_fsize) :
	File(Flags::writable),
	file(file),
	write_fsize(write_fsize)
{
	assert(file != nullptr);

	file->puts("// file created by lib kilipili\n\n");
	file->printf("// %s\n\n", rsrc_fname);

	// store filename:
	write(rsrc_fname, strlen(rsrc_fname) + 1);
	file->putc('\n');

	// reserve space for fsize:
	if (write_fsize) file->puts(spaces(4 * 4 + 1));

	// now the data written by caller follows
	fpos0 = uint32(file->getFpos());
	fpos  = 0;
}

void RsrcFileEncoder::finalize()
{
	if (fpos >= fsize) file->puts("\n");
	if (write_fsize)
	{
		file->setFpos(fpos0 - (4 * 4 + 1));
		uint32 sz = getSize();
		file->printf("%3u,%3u,%3u,%3u,\n", uchar(sz), uchar(sz >> 8), uchar(sz >> 16), uchar(sz >> 24));
	}
}

RsrcFileEncoder::~RsrcFileEncoder() noexcept
{
	// don't close the target file.
	// if close() wasn't called then the target file will be closed in it's d'tor too.

	if (file == nullptr) return;
	try
	{
		finalize();
	}
	catch (...)
	{
		debugstr("close() failed\n");
	}
}

SIZE RsrcFileEncoder::write(const void* q, SIZE len, bool)
{
	const uint8* p = reinterpret_cast<cuptr>(q);
	for (uint i = 0; i < len; i++)
	{
		file->printf("%3u,", p[i]);
		if (((fpos + i) & 0x1f) == 0x1f) file->putc('\n');
	}
	fpos += len;
	return len;
}

void RsrcFileEncoder::close()
{
	if (!file) return;
	finalize();
	file->close();
	file = nullptr;
}

void RsrcFileEncoder::setFpos(ADDR new_fpos)
{
	if (fpos >= fsize)
	{
		fsize = fpos;
		file->puts("\n\n");
	}

	fpos = new_fpos <= fsize ? uint32(new_fpos) : fsize;
	file->setFpos(fpos0 + fpos * 4 + fpos / 32);
}


} // namespace kio::Devices

/*


















































*/
