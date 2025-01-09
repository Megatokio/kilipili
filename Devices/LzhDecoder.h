/*
-1-	LZH depacking routine
	Original LZH code by Haruhiko Okumura (1991) and Kerwin F. Medina (1996)

-2-	Arnaud Carre changed to C++ object to remove global vars, so it should be thread safe now.
	ST-Sound, ATARI-ST Music Emulator
	Copyright (c) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )
	All rights reserved.

-3-	kio 2024: rework as a File wrapper.
	I presume there is nothing much left from Arnaud's changes.
	Copyright (c) 2024 - 2025 kio@little-bat.de
	BSD-2-Clause license
	https://opensource.org/licenses/BSD-2-Clause	
*/

#pragma once
#include "File.h"

#ifndef CHAR_BIT
  #define CHAR_BIT 8
#endif

#ifndef UCHAR_MAX
  #define UCHAR_MAX 255
#endif

namespace kio::Devices
{

extern bool isLzhEncoded(File*);

/*	_____________________________________________________________________________________
	LzhDecoder is a wrapper around another File.
	This allows easy decompression of all files read, because LzhDecoder implements
	the File interface just like any other file does. The data receiver just must provide
	an initialization with an open File instead of opening it itself.
	LzhDecoder supports setFPos() but it is slow.
*/
class LzhDecoder : public File
{
public:
	LzhDecoder(FilePtr);
	~LzhDecoder() noexcept override {}

	virtual SIZE read(void* data, SIZE, bool partial = false) override;
	virtual ADDR getSize() const noexcept override { return usize; }
	virtual ADDR getFpos() const noexcept override { return upos; }
	virtual void setFpos(ADDR) override;
	virtual void close() override;

	FilePtr file;
	uint32	cdata = 0; // start of compressed data
	uint32	csize = 0; // compressed size (size of cdata[])
	uint32	usize = 0; // uncompressed size (uncompressed file size)
	uint32	upos  = 0; // position inside uncompressed data
	uint32	cpos  = 0; // position inside compressed data

private:
	static constexpr uint BUFSIZE = 1024 * 4; // input buffer size

	using BITBUFTYPE = ushort;

	static constexpr uint BITBUFSIZ = CHAR_BIT * sizeof(BITBUFTYPE);
	static constexpr uint DICBIT	= 13; // 12(-lh4-) or 13(-lh5-)
	static constexpr uint DICSIZ	= 1u << DICBIT;
	static constexpr uint MAXMATCH	= 256;									// formerly F (not more than UCHAR_MAX + 1)
	static constexpr uint THRESHOLD = 3;									// choose optimal value
	static constexpr uint NC		= UCHAR_MAX + MAXMATCH + 2 - THRESHOLD; // alphabet = {0, 1, 2, ..., NC - 1}
	static constexpr uint CBIT		= 9;									// $\lfloor \log_2 NC \rfloor + 1$
	static constexpr uint CODE_BIT	= 16;									// codeword length

	static constexpr uint MAX_HASH_VAL = 3 * DICSIZ + (DICSIZ / 512 + 1) * UCHAR_MAX;

	static constexpr uint NP   = DICBIT + 1;
	static constexpr uint NT   = CODE_BIT + 3;
	static constexpr uint PBIT = 4;					// smallest integer such that (1U << PBIT) > NP
	static constexpr uint TBIT = 5;					// smallest integer such that (1U << TBIT) > NT
	static constexpr uint NPT  = NT > NP ? NT : NP; // max(NT,NP)

	//----------------------------------------------
	// Original Lzhxlib static func
	//----------------------------------------------
	void   fillbuf(int n);
	ushort getbits(int n);
	void   init_getbits();
	int	   make_table(int nchar, uchar* bitlen, int tablebits, ushort* table);
	void   read_pt_len(int nn, int nbit, int i_special);
	void   read_c_len();
	ushort decode_c();
	ushort decode_p();
	void   huf_decode_start();
	void   decode_start();
	void   decode(uint count, uchar buffer[]);


	//----------------------------------------------
	// Original Lzhxlib static vars
	//----------------------------------------------
	int		   fillbufsize;
	uchar	   buf[BUFSIZE]; // input buffer
	ushort	   left[2 * NC - 1];
	ushort	   right[2 * NC - 1];
	BITBUFTYPE bitbuf;
	uint	   subbitbuf;
	int		   bitcount;
	int		   decode_j; // remaining bytes to copy
	uchar	   c_len[NC];
	uchar	   pt_len[NPT];
	uint	   blocksize;
	ushort	   c_table[4096];
	ushort	   pt_table[256];

	uint fillbuf_i; // NOTE: these ones are not initialized at constructor time but inside the fillbuf and decode func.
	uint decode_i;

	uchar buffer[DICSIZ]; // output buffer
	uint  obu_ri;
};

} // namespace kio::Devices
