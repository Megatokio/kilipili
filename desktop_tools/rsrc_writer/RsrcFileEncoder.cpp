// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFileEncoder.h"
#include "cstrings.h"

namespace kio::Devices
{

RsrcFileEncoder::RsrcFileEncoder(FilePtr file, cstr rsrc_fname) : //
	File(Flags::writable),
	file(file)
{
	assert(file != nullptr);

	file->puts("// file created by lib kilipili\n\n");
	file->printf("// %s\n\n", rsrc_fname);

	// store filename:
	write(rsrc_fname, strlen(rsrc_fname) + 1);
	file->putc('\n');

	// now the data written by caller follows
	fpos0 = uint32(file->getFpos());
	fpos  = 0;
}

RsrcFileEncoder::~RsrcFileEncoder() noexcept
{
	// don't close the target file.
	// if close() wasn't called then the target file will be closed in it's d'tor too.

	if (file == nullptr) return;
	try
	{
		if (fpos >= fsize) file->puts("\n");
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
	if (fpos >= fsize) file->puts("\n");
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
