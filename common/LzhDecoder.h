/*
	ST-Sound ( YM files player library )

-1-	LZH depacking routine
	Original LZH code by Haruhiko Okumura (1991) and Kerwin F. Medina (1996)

-2-	Arnaud Carre changed to C++ object to remove global vars, so it should be thread safe now.

	ST-Sound, ATARI-ST Music Emulator
	Copyright (c) 1995-1999 Arnaud Carre ( http://leonard.oxg.free.fr )
	All rights reserved.

		Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

		THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGE.

-3-	kio 2024: minor rework and rename
*/

#pragma once
#include "standard_types.h"

#ifndef CHAR_BIT
  #define CHAR_BIT 8
#endif

#ifndef UCHAR_MAX
  #define UCHAR_MAX 255
#endif


class LzhDecoder
{
public:
	bool unpack(const void* pSrc, uint srcSize, void* pDst, uint dstSize);

private:
	static constexpr uint BUFSIZE = 1024 * 4;

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
	// New stuff to handle memory IO
	//----------------------------------------------
	const uchar* m_pSrc;
	uint		 m_srcSize;
	uchar*		 m_pDst;
	uint		 m_dstSize;

	uint DataIn(void* pBuffer, uint nBytes);
	uint DataOut(void* pOut, uint nBytes);


	//----------------------------------------------
	// Original Lzhxlib static func
	//----------------------------------------------
	void   fillbuf(int n);
	ushort getbits(int n);
	void   init_getbits(void);
	int	   make_table(int nchar, uchar* bitlen, int tablebits, ushort* table);
	void   read_pt_len(int nn, int nbit, int i_special);
	void   read_c_len(void);
	ushort decode_c(void);
	ushort decode_p(void);
	void   huf_decode_start(void);
	void   decode_start(void);
	void   decode(uint count, uchar buffer[]);


	//----------------------------------------------
	// Original Lzhxlib static vars
	//----------------------------------------------
	int		   fillbufsize;
	uchar	   buf[BUFSIZE];
	uchar	   outbuf[DICSIZ];
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
	int		   with_error;

	uint fillbuf_i; // NOTE: these ones are not initialized at constructor time but inside the fillbuf and decode func.
	uint decode_i;
};
