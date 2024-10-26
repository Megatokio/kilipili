// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <cstdio>


namespace kio
{

/*	the RsrcFileWriter creates a header file for inclusion in a C++ source.
	the file will be compressed using heatshrink.
	the file will be named after rsrc_fname.
	the array will be named after the basename of rsrc_fname.
	store data into the file with store() functions.

	TODO: error handling

	uncompressed[] =
	  char[] filename   0-terminated string
	  uint24 size       data size (after flag)
	  uint8  flag = 0
	  char[] data       uncompressed file data

	compressed[] =
	  char[] filename   0-terminated string
	  uint24 csize      compressed size (incl. usize)
	  uint8  flags != 0 windowsize<<4 + lookaheadsize
	  uint32 usize      uncompressed size
	  char[] data       compressed file data
*/
class RsrcFileWriter
{
public:
	RsrcFileWriter(cstr hdr_fpath, cstr rsrc_fname);
	~RsrcFileWriter() { close(); }
	void   store(const void*, uint);
	void   store_byte(uint8 n);
	void   store(uint32 n); // little endian
	void   store(cstr s);	// store c-string (incl. 0-byte)
	uint32 close();			// returns csize

	uint32 datasize = 0;
	uint   _padding;

private:
	FILE* file = nullptr;
	int32 position_of_size;

	uint8 cbu[32];
	uint  cbu_cnt = 0;

	void _flush();
	void _store(const void*, uint); // direct store data (uncompressed)
};


} // namespace kio
