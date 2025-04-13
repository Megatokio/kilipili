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

#include "LzhDecoder.h"
#include <cstring>
#include <memory.h>

namespace kio::Devices
{

struct __packed LzhHeader
{
	uint8  size; // of header
	uint8  sum;
	char   id[5]; // "-lh5-"
	uint32 csize;
	uint32 usize;
	uint8  reserved[5];
	uint8  level;
	uint8  name_length;
	// char name[];
	// uint16 crc;
};
static_assert(sizeof(LzhHeader) == 22);


bool isLzhEncoded(File* file)
{
	if (file == nullptr) return false;
	ADDR fpos  = file->getFpos();
	ADDR fsize = file->getSize();
	if (fpos + sizeof(LzhHeader) > fsize) return false;
	LzhHeader header;
	file->read(&header, sizeof(header)); // hdr_size, sum, id
	file->setFpos(fpos);
	if (memcmp(header.id, "-lh5-", 5) != 0) return false;
	if (header.size != sizeof(LzhHeader) + header.name_length) return false;
	if (header.size + 2 + header.csize > file->getSize()) return false;
	return true;
}

LzhDecoder::LzhDecoder(FilePtr file) : File(readable), file(file)
{
	assert(file != nullptr);

	LzhHeader header;
	file->read(&header, 1 + 1 + 5);			// hdr_size, sum, id
	csize = file->read_LE<uint32>();		// csize
	usize = file->read_LE<uint32>();		// usize
	file->read(header.reserved, 5 + 1 + 1); // reserved,level,name_length

	if (memcmp(header.id, "-lh5-", 5) != 0) throw "LZH: no lh5 file";
	if (header.size != sizeof(LzhHeader) + header.name_length) throw "LZH: wrong header size";
	if (header.size + 2 + csize > file->getSize()) throw "LZH: file truncated";

	cdata = uint32(file->getFpos()) + header.name_length + 2; // skip name & crc
	file->setFpos(cdata);

	decode_start();
	obu_ri = sizeof(buffer); // => exhausted
}

SIZE LzhDecoder::read(void* _data, SIZE size, bool partial)
{
	if unlikely (size > usize - upos)
	{
		size = usize - upos;
		if (!partial) throw END_OF_FILE;
		if (eof_pending()) throw END_OF_FILE;
		if (size == 0) set_eof_pending();
	}

	uint8* data = reinterpret_cast<uint8*>(_data);

	for (SIZE remaining = size; remaining;)
	{
		if (obu_ri == sizeof(buffer))
		{
			decode(std::min(DICSIZ, usize - upos), buffer);
			obu_ri = 0;
		}

		SIZE n = std::min(remaining, DICSIZ - obu_ri);
		memcpy(data, buffer + obu_ri, n);
		data += n;
		obu_ri += n;
		remaining -= n;
		upos += n;
	}

	return size;
}

void LzhDecoder::setFpos(ADDR new_upos)
{
	clear_eof_pending();

	if unlikely (new_upos >= usize)
	{
		upos = usize;
		return;
	}

	if (uint32(new_upos) < upos)
	{
		decode_start();
		file->setFpos(cdata);
		upos   = 0;
		cpos   = 0;
		obu_ri = sizeof(buffer);
	}

	while (upos < uint32(new_upos))
	{
		uint8 buffer[100];
		read(buffer, std::min(uint32(new_upos) - upos, uint32(sizeof(buffer))));
	}
}

void LzhDecoder::close()
{
	if (file) file->close();
	file = nullptr;
}


/*
 * io.c
 */

/** Shift bitbuf n bits left, read n bits */
void LzhDecoder::fillbuf(int n)
{
	bitbuf = (bitbuf << n) & 0xffff;
	while (n > bitcount)
	{
		bitbuf |= subbitbuf << (n -= bitcount);
		if (fillbufsize == 0)
		{
			fillbuf_i	= 0;
			fillbufsize = int(file->read(buf, std::min(BUFSIZE - 32, csize - cpos)));
			cpos += uint(fillbufsize);
		}
		if (fillbufsize > 0) fillbufsize--, subbitbuf = buf[fillbuf_i++];
		else subbitbuf = 0;
		bitcount = CHAR_BIT;
	}
	bitbuf |= subbitbuf >> (bitcount -= n);
}

ushort LzhDecoder::getbits(int n)
{
	ushort x;
	x = bitbuf >> (BITBUFSIZ - n);
	fillbuf(n);
	return x;
}

void LzhDecoder::init_getbits()
{
	bitbuf	  = 0;
	subbitbuf = 0;
	bitcount  = 0;
	fillbuf(BITBUFSIZ);
}


// #########################################################
// more or less original source follows:


/*
 * maketbl.c
 */

int LzhDecoder::make_table(int nchar, uchar* bitlen, int tablebits, ushort* table)
{
	ushort count[17], weight[17], start[18], *p;
	uint   jutbits, avail, mask;
	int	   i, ch, len, nextcode;

	for (i = 1; i <= 16; i++) count[i] = 0;
	for (i = 0; i < nchar; i++) count[bitlen[i]]++;

	start[1] = 0;
	for (i = 1; i <= 16; i++) start[i + 1] = start[i] + (count[i] << (16 - i));
	if (start[17] != (ushort)(1U << 16)) return (1); /* error: bad table */

	jutbits = 16 - tablebits;
	for (i = 1; i <= tablebits; i++)
	{
		start[i] >>= jutbits;
		weight[i] = 1U << (tablebits - i);
	}
	while (i <= 16)
	{
		weight[i] = 1U << (16 - i);
		i++;
	}

	i = start[tablebits + 1] >> jutbits;
	if (i != (ushort)(1U << 16))
	{
		int k = 1U << tablebits;
		while (i != k) table[i++] = 0;
	}

	avail = nchar;
	mask  = 1U << (15 - tablebits);
	for (ch = 0; ch < nchar; ch++)
	{
		if ((len = bitlen[ch]) == 0) continue;
		nextcode = start[len] + weight[len];
		if (len <= tablebits)
		{
			for (i = start[len]; i < nextcode; i++) table[i] = ch;
		}
		else
		{
			uint k = start[len];
			p	   = &table[k >> jutbits];
			i	   = len - tablebits;
			while (i != 0)
			{
				if (*p == 0)
				{
					right[avail] = left[avail] = 0;
					*p						   = avail++;
				}
				if (k & mask) p = &right[*p];
				else p = &left[*p];
				k <<= 1;
				i--;
			}
			*p = ch;
		}
		start[len] = nextcode;
	}
	return (0);
}

/*
 * huf.c
 */

void LzhDecoder::read_pt_len(int nn, int nbit, int i_special)
{
	int	   i, n;
	short  c;
	ushort mask;

	n = getbits(nbit);
	if (n == 0)
	{
		c = getbits(nbit);
		for (i = 0; i < nn; i++) pt_len[i] = 0;
		for (i = 0; i < 256; i++) pt_table[i] = c;
	}
	else
	{
		i = 0;
		while (i < n)
		{
			c = bitbuf >> (BITBUFSIZ - 3);
			if (c == 7)
			{
				mask = 1U << (BITBUFSIZ - 1 - 3);
				while (mask & bitbuf)
				{
					mask >>= 1;
					c++;
				}
			}
			fillbuf((c < 7) ? 3 : c - 3);
			pt_len[i++] = uchar(c);
			if (i == i_special)
			{
				c = getbits(2);
				while (--c >= 0) pt_len[i++] = 0;
			}
		}
		while (i < nn) pt_len[i++] = 0;
		make_table(nn, pt_len, 8, pt_table);
	}
}

void LzhDecoder::read_c_len()
{
	short  i, c, n;
	ushort mask;

	n = getbits(CBIT);
	if (n == 0)
	{
		c = getbits(CBIT);
		for (i = 0; i < NC; i++) c_len[i] = 0;
		for (i = 0; i < 4096; i++) c_table[i] = c;
	}
	else
	{
		i = 0;
		while (i < n)
		{
			c = pt_table[bitbuf >> (BITBUFSIZ - 8)];
			if (c >= NT)
			{
				mask = 1U << (BITBUFSIZ - 1 - 8);
				do {
					if (bitbuf & mask) c = right[c];
					else c = left[c];
					mask >>= 1;
				}
				while (c >= NT);
			}
			fillbuf(pt_len[c]);
			if (c <= 2)
			{
				if (c == 0) c = 1;
				else if (c == 1) c = getbits(4) + 3;
				else c = getbits(CBIT) + 20;
				while (--c >= 0) c_len[i++] = 0;
			}
			else c_len[i++] = c - 2;
		}
		while (i < NC) c_len[i++] = 0;
		make_table(NC, c_len, 12, c_table);
	}
}

ushort LzhDecoder::decode_c()
{
	ushort j, mask;

	if (blocksize == 0)
	{
		blocksize = getbits(16);
		read_pt_len(NT, TBIT, 3);
		read_c_len();
		read_pt_len(NP, PBIT, -1);
	}
	blocksize--;
	j = c_table[bitbuf >> (BITBUFSIZ - 12)];
	if (j >= NC)
	{
		mask = 1U << (BITBUFSIZ - 1 - 12);
		do {
			if (bitbuf & mask) j = right[j];
			else j = left[j];
			mask >>= 1;
		}
		while (j >= NC);
	}
	fillbuf(c_len[j]);
	return j;
}

ushort LzhDecoder::decode_p()
{
	ushort j, mask;

	j = pt_table[bitbuf >> (BITBUFSIZ - 8)];
	if (j >= NP)
	{
		mask = 1U << (BITBUFSIZ - 1 - 8);
		do {
			if (bitbuf & mask) j = right[j];
			else j = left[j];
			mask >>= 1;
		}
		while (j >= NP);
	}
	fillbuf(pt_len[j]);
	if (j != 0) j = (1U << (j - 1)) + getbits(j - 1);
	return j;
}

void LzhDecoder::huf_decode_start()
{
	init_getbits();
	blocksize = 0;
}

/*
 * decode.c
 */

void LzhDecoder::decode_start()
{
	fillbufsize = 0;
	huf_decode_start();
	decode_j = 0;
}

/*
 * The calling function must keep the number of bytes to be processed.  This
 * function decodes either 'count' bytes or 'DICSIZ' bytes, whichever is
 * smaller, into the array 'buffer[]' of size 'DICSIZ' or more. Call
 * decode_start() once for each new file before calling this function.
 */
void LzhDecoder::decode(uint count, uchar buffer[])
{
	uint r, c;

	r = 0;
	while (--decode_j >= 0)
	{
		buffer[r] = buffer[decode_i];
		decode_i  = (decode_i + 1) & (DICSIZ - 1);
		if (++r == count) return;
	}
	for (;;)
	{
		c = decode_c();
		if (c <= UCHAR_MAX)
		{
			buffer[r] = c;
			if (++r == count) return;
		}
		else
		{
			decode_j = c - (UCHAR_MAX + 1 - THRESHOLD);
			decode_i = (r - decode_p() - 1) & (DICSIZ - 1);
			while (--decode_j >= 0)
			{
				buffer[r] = buffer[decode_i];
				decode_i  = (decode_i + 1) & (DICSIZ - 1);
				if (++r == count) return;
			}
		}
	}
}


} // namespace kio::Devices
