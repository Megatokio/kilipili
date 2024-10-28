// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFileWriter.h"
#include "common/basic_math.h"
#include "cstrings.h"

namespace kio
{

/*
	uncompressed[] =
	  char[] filename   0-terminated string
	  uint24 size       data size (after flag)
	  uint8  flag = 0
	  char[] data       uncompressed file data
*/

RsrcFileWriter::RsrcFileWriter(cstr hdr_fpath, cstr rsrc_fname)
{
	if ((file = fopen(hdr_fpath, "wb")) == NULL) throw "Unable to open output file";

	fputs("// file created by lib kilipili\n\n", file);
	fprintf(file, "// %s\n\n", rsrc_fname);

	// store filename:
	_store(rsrc_fname, strlen(rsrc_fname) + 1);

	// reserve space:
	//   3 bytes (un)compressed data size
	//   1 byte  parameters = 0
	position_of_size = int32(ftell(file));
	fputs(spaces(4 * 4), file);
	fputc('\n', file);
	// now uncompressed data follows
}

void RsrcFileWriter::_store(const void* q, uint len)
{
	cuptr p = reinterpret_cast<cuptr>(q);
	while (len > 32)
	{
		_store(p, 32);
		p += 32;
		len -= 32;
	}

	char bu[32 * 4 + 2];
	ptr	 z = bu;
	for (uint i = 0; i < len; i++) { z += sprintf(z, "%u,", p[i]); }
	*z++ = '\n';
	*z	 = 0;
	fputs(bu, file);
}

void RsrcFileWriter::_flush()
{
	_store(cbu, cbu_cnt);
	cbu_cnt = 0;
}

uint32 RsrcFileWriter::close()
{
	if (!file) return 0;
	_flush();

	fputs("\n", file);
	uint32 fsize = uint32(ftell(file));

	fseek(file, position_of_size, SEEK_SET);
	uint32 size = htole32(datasize);
	_store(&size, 4);

	fclose(file);
	file = nullptr;
	return fsize;
}

void RsrcFileWriter::store(cstr s)
{
	// store c-string into rsrc file:

	store(s, strlen(s) + 1);
}

void RsrcFileWriter::store(uint32 n)
{
	// store uint32 as little-endian into rsrc file:

	n = htole32(n);
	store(&n, 4);
}

void RsrcFileWriter::store(const void* q, uint len)
{
	// store data into rsrc file:

	datasize += len;
	cuptr p = reinterpret_cast<cuptr>(q);

	while (len)
	{
		if (cbu_cnt == sizeof(cbu)) _flush();
		uint cnt = min(len, uint(sizeof(cbu)) - cbu_cnt);
		memcpy(cbu + cbu_cnt, p, cnt);
		cbu_cnt += cnt;
		len -= cnt;
		p += cnt;
	}
}

void RsrcFileWriter::store_byte(uint8 n)
{
	// store 1 byte of data into rsrc file:

	datasize += 1;
	if (cbu_cnt == sizeof(cbu)) _flush();
	cbu[cbu_cnt++] = n;
}


} // namespace kio

/*


















































*/
