// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "RsrcFileWriter.h"
#include "cstrings.h"

// https://github.com/atomicobject/heatshrink

namespace kio
{

/*
	compressed[] =
	  char[] filename   0-terminated string
	  uint24 csize      compressed size (incl. usize)
	  uint8  flags != 0 windowsize<<4 + lookaheadsize
	  uint32 usize      uncompressed size
	  char[] data       compressed file data
*/

RsrcFileWriter::RsrcFileWriter(cstr hdr_fpath, cstr rsrc_fname)
{
	encoder = heatshrink_encoder_alloc(windowsize, lookaheadsize);
	if (!encoder) throw "out of memory";

	if ((file = fopen(hdr_fpath, "wb")) == NULL)
	{
		heatshrink_encoder_free(encoder);
		encoder = nullptr;
		throw "Unable to open output file";
	}

	fputs("// file created by lib kilipili\n\n", file);
	fprintf(file, "// %s\n\n", rsrc_fname);

	// store filename:
	_store(rsrc_fname, strlen(rsrc_fname) + 1);

	// reserve space:
	//   compressed data size:   3 bytes
	//   parameters:             1 byte
	//   uncompressed file size: 4 bytes
	position_of_size = int32(ftell(file));
	fputs(spaces(8 * 4), file);
	fputc('\n', file);
	// now compressed data follows
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

void RsrcFileWriter::_flush(bool force)
{
	// flush the encoder and write chunks of 32 bytes:

	for (;;)
	{
		size_t cnt = 0;
		int	   err = heatshrink_encoder_poll(encoder, cbu + cbu_cnt, sizeof(cbu) - cbu_cnt, &cnt);
		assert(err >= 0);
		cbu_cnt += cnt;
		if (cbu_cnt < sizeof(cbu) && !force) return;
		if (cbu_cnt == 0) return;

		_store(cbu, cbu_cnt);
		csize += cbu_cnt;
		cbu_cnt = 0;
		if (err == HSER_POLL_EMPTY) return;
	}
}

uint32 RsrcFileWriter::close()
{
	if (!file) return 0;
	heatshrink_encoder_finish(encoder);
	_flush(true);
	heatshrink_encoder_free(encoder);
	encoder = nullptr;

	fputs("\n", file);
	uint32 fsize = uint32(ftell(file));

	fseek(file, position_of_size, SEEK_SET);
	uint32 csize = this->csize + (windowsize << 28) + (lookaheadsize << 24);
	uint32 bu[2] = {htole32(csize), htole32(usize)};
	_store(bu, 8);

	fclose(file);
	file = nullptr;
	return fsize;
}

void RsrcFileWriter::store(cstr s)
{
	// store data into compressed file:

	store(s, strlen(s) + 1);
}

void RsrcFileWriter::store(const void* q, uint len)
{
	// store data into compressed file:

	usize += len;
	size_t cnt = 0;
	uint8* p   = reinterpret_cast<uint8*>(const_cast<void*>(q));
	heatshrink_encoder_sink(encoder, p, len, &cnt);
	assert(cnt == len); //else todo
	_flush();
}

void RsrcFileWriter::store_byte(uint8 n)
{
	// store data into compressed file:

	usize += 1;
	size_t cnt = 0;
	heatshrink_encoder_sink(encoder, &n, 1, &cnt);
	assert(cnt == 1); //else todo
	_flush();
}


} // namespace kio
