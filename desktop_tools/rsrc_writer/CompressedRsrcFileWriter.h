// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "extern/heatshrink/heatshrink_encoder.h"
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
class CompressedRsrcFileWriter
{
public:
	CompressedRsrcFileWriter(cstr hdr_fpath, cstr rsrc_fname, uint8 wsize = 12, uint8 lsize = 6);
	~CompressedRsrcFileWriter() { close(); }
	void   store(cstr s);
	void   store(const void*, uint);
	void   store_byte(uint8 n);
	void   store(uint32 n); // little endian
	uint32 close();			// returns csize

	uint windowsize	   = 12; // 2<<10 = 2048 bytes + 4<<10 bytes for index (if enabled)
	uint lookaheadsize = 6;	 // 2<<6 = 128 bytes

	uint32 usize = 0;
	uint32 csize = 0;

private:
	FILE*				file	= nullptr;
	heatshrink_encoder* encoder = nullptr;
	int32				position_of_size;

	uint8 cbu[32];
	uint  cbu_cnt = 0;

	void _flush(bool force = false);
	void _store(const void*, uint); // direct store data (uncompressed)
};


} // namespace kio
