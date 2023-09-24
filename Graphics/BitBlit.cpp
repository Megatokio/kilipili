// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BitBlit.h"

namespace kio::Graphics::bitblit
{

// #############################################################

void copy_rect_of_bits(uint32* zp, int zx, const uint32* qp, int qx, int cnt) noexcept
{
	// copy non-aligned bits with incrementing addresses
	// attn: left pixel is in lsb!
	//    => shift ops in source code are opposite direction to how pixels move!
	//
	// in: qp -> word which provides the first bits
	//     qx =  offset of bits from the left side
	//     zp -> word which receives the first bits
	//     zx =  offset of bits from the left side

	assert(uint(zx) <= 31);
	assert(uint(qx) <= 31);
	assert(zx != qx);

	// we walk from left to right => pixel come in from the high side:
	uint32 lo, hi; // words holding bits from the source row
	int	   lsl;	   // left shift for pixels / right shift for bits
	int	   hsr;	   // shift for hi

	if (qx > zx)
	{
		lsl = qx - zx;
		hsr = 32 - lsl;
		lo	= *qp++;
		hi	= *qp++;
	}
	else
	{
		hsr = zx - qx;
		lsl = 32 - hsr;
		lo	= 0; // silence warning
		hi	= *qp++;
	}

	if (zx + cnt < 32) // very few bits go into single word
	{
		uint32 mask = (1 << cnt) - 1; // mask for cnt bits
		mask		= mask << zx;	  // mask selects bits from q

		uint32 byte = (lo >> lsl) | (hi << hsr);
		*zp			= (*zp & ~mask) | (byte & mask);
		return;
	}

	if (zx) // copy partial first word
	{
		uint32 mask = ~0u << zx; // mask selects bits from q
		uint32 byte = (lo >> lsl) | (hi << hsr);
		*zp			= (*zp & ~mask) | (byte & mask);
		zp++;
		lo = hi;
		hi = *qp++;
		cnt -= 32 - zx;
	}

	while (cnt >= 64) // copy main part
	{
		*zp++ = (lo >> lsl) | (hi << hsr);
		lo	  = *qp++;					   // lo=hi and hi=lo
		*zp++ = (hi >> lsl) | (lo << hsr); // lo=hi and hi=lo
		hi	  = *qp++;
		cnt -= 64;
	}

	if (cnt >= 32)
	{
		*zp++ = (lo >> lsl) | (hi << hsr);
		lo	  = hi;
		hi	  = *qp++;
		cnt -= 32;
	}

	if (cnt) // copy partial last halfword
	{
		uint32 mask = ~0u << cnt; // mask selects bits in z
		uint32 byte = (lo >> lsl) | (hi << hsr);
		*zp			= (*zp & mask) | (byte & ~mask);
	}
}

void rcopy_bits(uint32* zp, int zx, const uint32* qp, int qx, int cnt) noexcept
{
	// copy non-aligned bits with decrementing addresses
	// attn: left pixel is in lsb!
	//    => shift ops in source code are opposite direction to how pixels move!
	//
	// in: qp -> word which provides the last bits
	//     qx =  offset of bits from the right side
	//     zp -> word which receives the last bits
	//     zx =  offset of bits from the right side

	assert(uint(zx) <= 31);
	assert(uint(qx) <= 31);
	assert(zx != qx);

	// we walk from right to left => pixel come in from the low side:
	uint32 lo, hi; // words holding bits from the source row
	int	   hsr;	   // right shift for pixels / left shift for bits
	int	   lsl;	   // shift for lo

	if (qx > zx)
	{
		hsr = qx - zx;
		lsl = 32 - hsr;
		hi	= *qp--;
		lo	= *qp--;
	}
	else
	{
		lsl = zx - qx;
		hsr = 32 - lsl;
		hi	= 0; // silence warning
		lo	= *qp--;
	}

	if (zx + cnt < 32) // very few bits go into single word
	{
		zx			= 32 - (zx + cnt); // zx now from the left (as in copy_bits())
		uint32 mask = (1 << cnt) - 1;  // mask for cnt bits
		mask		= mask << zx;	   // mask selects bits from q

		uint32 byte = (lo >> lsl) | (hi << hsr);
		*zp			= (*zp & ~mask) | (byte & mask);
		return;
	}

	if (zx) // copy partial first word
	{
		uint32 mask = ~0u >> zx; // mask selects bits from q
		uint32 byte = (lo >> lsl) | (hi << hsr);
		*zp			= (*zp & ~mask) | (byte & mask);
		zp--;
		hi = lo;
		lo = *qp--;
		cnt -= 32 - zx;
	}

	while (cnt >= 64) // copy main part
	{
		*zp-- = (lo >> lsl) | (hi << hsr);
		hi	  = *qp--;					   // lo=hi and hi=lo
		*zp-- = (hi >> lsl) | (lo << hsr); // lo=hi and hi=lo
		lo	  = *qp--;
		cnt -= 64;
	}

	if (cnt >= 32)
	{
		*zp-- = (lo >> lsl) | (hi << hsr);
		hi	  = lo;
		lo	  = *qp--;
		cnt -= 32;
	}

	if (cnt) // copy partial last halfword
	{
		uint32 mask = ~0u >> cnt; // mask selects bits in z
		uint32 byte = (lo >> lsl) | (hi << hsr);
		*zp			= (*zp & mask) | (byte & ~mask);
	}
}

template<typename UINT>
void copy_bits_aligned(UINT* zp, const UINT* qp, int x, int cnt) noexcept
{
	// copy 8/16/32 bit aligned bits with incrementing address
	//
	// attn: left pixel is in lsb!
	//    => shift ops in source code are opposite direction to pixels!
	//
	// in: qp -> word which provides the first bits
	//     zp -> word which receives the first bits
	//     x =  offset of bits from the left side

	constexpr int BITS = sizeof(UINT) << 3;
	constexpr int MASK = BITS - 1;

	assert(uint(x) < BITS);

	if (x + cnt <= BITS) // very few bits go into single word
	{
		UINT mask = UINT((1 << cnt) - 1); // mask for cnt bits
		mask	  = UINT(mask << x);	  // mask selects bits from q
		*zp		  = (*zp & ~mask) | (*qp & mask);
		return;
	}

	if (x) // copy first partial word
	{
		UINT mask = UINT(~0u << x); // mask selects bits from q
		*zp		  = (*zp & ~mask) | (*qp & mask);
		zp++;
		qp++;
		cnt -= BITS - x;
	}

	for (uint i = 0; i < uint(cnt) / (BITS * 2); i++) // copy main part
	{
		*zp++ = *qp++;
		*zp++ = *qp++;
	}

	if (cnt & BITS) // copy last odd word
	{
		*zp++ = *qp++;
	}

	if (cnt & MASK) // copy last partial word
	{
		uint32 mask = ~0u << cnt; // mask selects bits in z
		*zp			= (*zp & mask) | (*qp & ~mask);
	}
}

template<typename UINT>
void rcopy_bits_aligned(UINT* zp, const UINT* qp, int x, int cnt) noexcept
{
	// copy 8/16/32 bit aligned bits with decrementing address
	//
	// attn: left pixel is in lsb!
	//    => shift ops in source code are opposite direction to pixels!
	//
	// in: qp -> word which provides the last bits
	//     zp -> word which receives the last bits
	//     x =  offset of bits from the right side

	constexpr uint BITS = sizeof(UINT) << 3;
	constexpr uint MASK = BITS - 1;

	assert(uint(x) < BITS);

	if (x + cnt <= BITS) // very few bits go into single word
	{
		x		  = 32 - (x + cnt); // zx now from the left (as in copy_bits_aligned())
		UINT mask = (1 << cnt) - 1; // mask for cnt bits
		mask	  = mask << x;		// mask selects bits from q
		*zp		  = (*zp & ~mask) | (*qp & mask);
		return;
	}

	if (x) // copy first partial word
	{
		UINT mask = UINT(~0u >> x); // mask selects bits from q
		*zp		  = (*zp & ~mask) | (*qp & mask);
		zp--;
		qp--;
		cnt -= BITS - x;
	}

	for (uint i = 0; i < uint(cnt) / (BITS * 2); i++) // copy main part
	{
		*zp-- = *qp--;
		*zp-- = *qp--;
	}

	if (cnt & BITS) // copy last odd word
	{
		*zp-- = *qp--;
	}

	if (cnt & MASK) // copy last partial word
	{
		uint32 mask = ~0u >> cnt; // mask selects bits in z
		*zp			= (*zp & mask) | (*qp & ~mask);
	}
}

void copy_rect_of_bytes(uint8* zp, int z_row_offs, const uint8* qp, int q_row_offs, int w, int h) noexcept
{
	assert(h >= 0 && w >= 0);

	if (zp <= qp)
	{
		while (h--)
		{
			memmove(zp, qp, uint(w));
			zp += z_row_offs;
			qp += q_row_offs;
		}
	}
	else
	{
		zp += h * z_row_offs;
		qp += h * q_row_offs;

		while (h--)
		{
			zp -= z_row_offs;
			qp -= q_row_offs;
			memmove(zp, qp, uint(w));
		}
	}
}

void copy_rect_of_bits(
	uint8* zp, int zx, int z_row_offs, const uint8* qp, int qx, int q_row_offs, int w, int h) noexcept
{
	/*	copy rectangular area of bits from source to destination.

		zp -> base address of destination
		zxoffs = row offset in destination
		zx = offset in bits from zp to the left border of the destination rect

		qp -> base address of source
		qxoffs = row offset in source
		qx = offset in bits from zp to the left border of the source rect

		w = width in bits
		h = height in rows
	*/

	assert(h >= 0 && w > 0);

	if (zp <= qp) // work in y++ and x++ direction:
	{
		// test if we can use byte copy:

		if (((int(zx) ^ int(qx)) & 7) == 0)
		{
			zp -= zx >> 3;
			zx &= 7;
			qp -= qx >> 3; //qx &= 7;

			if (zx == 0 && (w & 7) == 0)
			{
				while (h--)
				{
					memmove(zp, qp, uint(w >> 3));
					zp += z_row_offs;
					qp += q_row_offs;
				}
				return;
			}
			else
			{
				while (h--)
				{
					copy_bits_aligned(zp, qp, zx, w);
					zp += z_row_offs;
					qp += q_row_offs;
				}
				return;
			}
		}

		// align to word addresses:

		int o;
		o = int(zp) & 3;
		zp -= o;
		zx += o << 3;
		zp += (zx >> 5) << 2;
		zx &= 31;
		o = int(qp) & 3;
		qp -= o;
		qx += o << 3;
		qp += (qx >> 5) << 2;
		qx &= 31;

		// test if row offsets are word aligned:

		if (((z_row_offs | q_row_offs) & 3) == 0)
		{
			while (h--)
			{
				copy_rect_of_bits(reinterpret_cast<uint32ptr>(zp), zx, reinterpret_cast<cuint32ptr>(qp), qx, w);
				zp += z_row_offs;
				qp += q_row_offs;
			}
		}
		else
		{
			while (h--)
			{
				copy_rect_of_bits(reinterpret_cast<uint32ptr>(zp), zx, reinterpret_cast<cuint32ptr>(qp), qx, w);
				zp += z_row_offs;
				qp += q_row_offs;
				o = int(zp) & 3;
				if (o)
				{
					zp -= o;
					zx += o << 3;
					zp += (zx >> 5) << 2;
					zx &= 31;
				}
				o = int(qp) & 3;
				if (o)
				{
					qp -= o;
					qx += 8 << 3;
					qp += (qx >> 5) << 2;
					qx &= 31;
				}
			}
		}
	}
	else // work in y-- and x-- direction:
	{
		zp += z_row_offs * h; // start with last row
		qp += q_row_offs * h;

		// test if we can use byte copy:

		if (((int(zx) ^ int(qx)) & 7) == 0)
		{
			zp -= zx >> 3;
			zx &= 7;
			qp -= qx >> 3; //qx &= 7;

			if (zx == 0 && (w & 7) == 0)
			{
				while (h--)
				{
					zp -= z_row_offs;
					qp -= q_row_offs;
					memmove(zp, qp, uint(w >> 3));
				}
				return;
			}
			else
			{
				qp += (qx + w - 1) >> 3; // -> at byte which provides the last bits
				zp += (zx + w - 1) >> 3; // -> at byte which receives the last bits

				zx = -(zx + w) & 7; // bits to skip from the right
				//qx = -(qx+w) & 7;

				while (h--)
				{
					zp -= z_row_offs;
					qp -= q_row_offs;
					copy_bits_aligned(zp, qp, zx, w);
				}
				return;
			}
		}

		// switch to right-to-left:

		zp += (zx + w - 1) >> 3; // -> last byte
		qp += (qx + w - 1) >> 3;

		zx = -(zx + w) & 7; // bits to skip from the right
		qx = -(qx + w) & 7;

		// test if row offsets are word aligned:

		if (((z_row_offs | q_row_offs) & 3) == 0)
		{
			// align to word addresses:
			if (int o = int(zp) & 3)
			{
				zp -= o;
				zx += o << 3;
				if (zx > 31)
				{
					zx -= 32;
					zp += 4;
				}
			}
			if (int o = int(qp) & 3)
			{
				qp -= o;
				qx += o << 3;
				if (qx > 31)
				{
					qx -= 32;
					qp += 4;
				}
			}

			while (h--)
			{
				zp -= z_row_offs;
				qp -= q_row_offs;
				rcopy_bits(reinterpret_cast<uint32ptr>(zp), zx, reinterpret_cast<cuint32ptr>(qp), qx, w);
			}
		}
		else
		{
			while (h--)
			{
				zp -= z_row_offs;
				qp -= q_row_offs;

				// align to word addresses:
				if (int o = int(zp) & 3)
				{
					zp -= o;
					zx += o << 3;
					if (zx > 31)
					{
						zx -= 32;
						zp += 4;
					}
				}
				if (int o = int(qp) & 3)
				{
					qp -= o;
					qx += o << 3;
					if (qx > 31)
					{
						qx -= 32;
						qp += 4;
					}
				}

				rcopy_bits(reinterpret_cast<uint32ptr>(zp), zx, reinterpret_cast<cuint32ptr>(qp), qx, w);
			}
		}
	}
}


// #############################################################

void clear_row(uint32* z, int cnt, uint32 color) noexcept
{
	while (--cnt >= 0) { *z++ = color; }
}

void clear_row(uint16* z, int cnt, uint32 color) noexcept
{
	if (unlikely(cnt == 0)) return;

	if (int(z) & 2)
	{
		*z++ = uint16(color);
		cnt--;
	}
	if (cnt & 1) { z[--cnt] = uint16(color); }
	clear_row(reinterpret_cast<uint32*>(z), cnt >> 1, color);
}

void clear_row(uint8* z, int cnt, uint32 color) noexcept
{
	if (unlikely(cnt == 0)) return;

	if (int(z) & 1)
	{
		*z++ = uint8(color);
		cnt--;
	}
	if (cnt & 1) { z[--cnt] = uint8(color); }
	clear_row(reinterpret_cast<uint16*>(z), cnt >> 1, color);
}

void clear_row_of_bits(uint8* zp, int xoffs, int width, uint32 color) noexcept
{
	zp += xoffs >> 3;
	xoffs &= 7;

	int		o = int(zp) & 3; // align zp to int32
	uint32* p = reinterpret_cast<uint32*>(zp - o);
	xoffs += o << 3;

	uint32 lmask = ~0u << xoffs; // mask bits to set at left end
	*p			 = (*p & ~lmask) | (color & lmask);

	width -= 32 - xoffs;
	uint32 rmask = ~0u << ((32 - width) & 31); // mask bits to set at right end

	if (width > 0)
	{
		p++;
		int cnt = width >> 5;
		for (int i = 0; i < cnt; i++) { *p++ = color; }
	}

	*p = (*p & ~rmask) | (color & rmask);
}

void xor_row_of_bits(uint8* zp, int xoffs, int width, uint32 color) noexcept
{
	zp += xoffs >> 3;
	xoffs &= 7;

	int		o = int(zp) & 3; // align zp to int32
	uint32* p = reinterpret_cast<uint32*>(zp - o);
	xoffs += o << 3;

	uint32 lmask = ~0u << xoffs; // mask bits to set at left end
	//*p = (*p & ~lmask) | ((color^*p) & lmask);
	*p ^= lmask & color;

	width -= 32 - xoffs;
	uint32 rmask = ~0u << ((32 - width) & 31); // mask bits to set at right end

	if (width > 0)
	{
		p++;
		int cnt = width >> 5;
		for (int i = 0; i < cnt; i++) { *p++ ^= color; }
	}

	//*p = (*p & ~rmask) | ((color^*p) & rmask);
	*p ^= rmask & color;
}

void clear_row_of_bits_with_mask(uint8* zp, int zx, int width, uint32 color, uint32 mask) noexcept
{
	zp += zx >> 3;
	zx &= 7;

	if (int o = int(zp) & 3) // align zp to int32
	{
		zp -= o;
		o <<= 3;
		zx += o;
		mask = (mask << o) | (mask >> (32 - o));
	}

	uint32* p = reinterpret_cast<uint32*>(zp);

	uint32 lmask = mask & (~0u << zx); // mask bits to set at left end
	*p			 = (*p & ~lmask) | (color & lmask);

	width -= 32 - zx;
	uint32 rmask = mask & (~0u << ((32 - width) & 31)); // mask bits to set at right end

	if (width > 0)
	{
		p++;
		int cnt = width >> 5;
		for (int i = 0; i < cnt; i++) { *p++ = color; }
	}

	*p = (*p & ~rmask) | (color & rmask);
}

void clear_rect_of_bits(uint8* zp, int xoffs, int row_offset, int width, int height, uint32 color) noexcept
{
	//printf("clear_rect_of_bits:\n");
	//printf("  xoffs=%i\n", xoffs);
	//printf("  row_offs=%i\n", row_offset);
	//printf("  width=%i\n", width);
	//printf("  height=%i\n", height);

	if (unlikely(width <= 0 || height <= 0)) return;

	if (unlikely(row_offset & 3))
	{
		// if row_offset is not a multiple of 4
		// then alignment will change from row to row.
		// a simple way to fix this fast is to double the row offset
		// and clear only every 2nd row in each of two rounds
		// or quadruple the row offset and do 4 rounds.

		if (row_offset & 1)
		{
			clear_rect_of_bits(zp, xoffs, row_offset << 2, width, (height + 3) >> 2, color);
			clear_rect_of_bits(zp + row_offset * 1, xoffs, row_offset << 2, width, (height + 2) >> 2, color);
			clear_rect_of_bits(zp + row_offset * 2, xoffs, row_offset << 2, width, (height + 1) >> 2, color);
			clear_rect_of_bits(zp + row_offset * 3, xoffs, row_offset << 2, width, (height + 0) >> 2, color);
		}
		else // if (row_offset & 2)
		{
			clear_rect_of_bits(zp, xoffs, row_offset << 1, width, (height + 1) >> 1, color);
			clear_rect_of_bits(zp + row_offset, xoffs, row_offset << 1, width, (height + 0) >> 1, color);
		}
		return;
	}

	// row_offset is a multiple of 4
	// => alignment from row to row won't change!

	// add full bytes from xoffs to zp:
	zp += xoffs >> 3;
	xoffs &= 7;

	// align zp to int32:
	int		o = int(zp) & 3;
	uint32* p = reinterpret_cast<uint32*>(zp - o);
	xoffs += o << 3;

	// mask for bits to set at left end:  (note: lsb is left!)
	int keep = xoffs;
	width += keep;
	uint32 lmask = ~0u << keep;

	// mask for bits to set at right end:
	keep = -width & 31;
	width += keep;
	uint32 rmask = ~0u >> keep;

	int cnt = width >> 5;
	int dp	= (row_offset >> 2) - cnt;

	assert(cnt > 0);

	//printf("  cnt = %i\n", cnt);
	//printf("  dp  = %i\n", dp);
	//printf("  lmask = 0x%08x\n", lmask);
	//printf("  rmask = 0x%08x\n", rmask);
	//printf("  p=%u\n", uint(p));

	if (cnt == 1)
	{
		color &= lmask & rmask;
		uint32 mask = ~(lmask & rmask); // bits to keep

		while (height--)
		{
			*p++ = (*p & mask) | color;
			p += dp;
		}
	}
	else
	{
		uint32 lcolor = color & lmask;
		uint32 rcolor = color & rmask;
		lmask		  = ~lmask; // bits to keep
		rmask		  = ~rmask; // bits to keep

		while (height--)
		{
			*p++ = (*p & lmask) | lcolor;
			for (int i = 0; i < cnt - 2; i++) { *p++ = color; }
			*p++ = (*p & rmask) | rcolor;
			p += dp;
		}
	}

	//printf("  p=%u\n", uint(p));
}

void xor_rect_of_bits(uint8* zp, int xoffs, int row_offset, int width, int height, uint32 color) noexcept
{
	if (unlikely(width <= 0 || height <= 0)) return;

	if (unlikely(row_offset & 3))
	{
		// if row_offset is not a multiple of 4
		// then alignment will change from row to row.
		// a simple way to fix this fast is to double the row offset
		// and clear only every 2nd row in each of two rounds
		// or quadruple the row offset and do 4 rounds.

		if (row_offset & 1)
		{
			xor_rect_of_bits(zp, xoffs, row_offset << 2, width, (height + 3) >> 2, color);
			xor_rect_of_bits(zp + row_offset * 1, xoffs, row_offset << 2, width, (height + 2) >> 2, color);
			xor_rect_of_bits(zp + row_offset * 2, xoffs, row_offset << 2, width, (height + 1) >> 2, color);
			xor_rect_of_bits(zp + row_offset * 3, xoffs, row_offset << 2, width, (height + 0) >> 2, color);
		}
		else // if (row_offset & 2)
		{
			xor_rect_of_bits(zp, xoffs, row_offset << 1, width, (height + 1) >> 1, color);
			xor_rect_of_bits(zp + row_offset, xoffs, row_offset << 1, width, (height + 0) >> 1, color);
		}
		return;
	}

	// row_offset is a multiple of 4
	// => alignment from row to row won't change!

	zp += xoffs >> 3;
	xoffs &= 7;

	int		o = int(zp) & 3; // align zp to int32
	uint32* p = reinterpret_cast<uint32*>(zp - o);
	xoffs += o << 3;

	uint32 lmask = ~0u << xoffs; // mask bits to set at left end
	width -= 32 - xoffs;
	uint32 rmask = ~0u << ((32 - width) & 31); // mask bits to set at right end

	if (width <= 0)
	{
		while (height--)
		{
			*p ^= lmask & rmask & color;
			p += row_offset >> 2;
		}
	}
	else
	{
		int cnt = width >> 5;					 // number of full words
		int dp	= (row_offset >> 2) - (cnt + 1); // remaining offset (words)

		while (height--)
		{
			*p++ ^= lmask & color;
			for (int i = 0; i < cnt; i++) { *p++ ^= color; }
			*p ^= rmask & color;
			p += dp;
		}
	}
}


// #############################################################

void draw_bmp_1bpp(
	uint8* zp, int zx, int z_row_offs, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_1bpp;
	color					= flood_filled_color<CD>(color);
	assert(width > 0);

	// for colordepth_i1 we go at extra length for optimization
	// and distinguish between color = 0 and color = 1
	// because these are only 2 cases and most attribute modes use colordepth_i1 in the pixels[]

	zp += zx >> 3;
	zx &= 7; // low 3 bit of bit address

	if (zx == 0) // no need to shift
	{
		uint mask = ~(~0u << (width & 7)); // mask for last bits in q
		width	  = width >> 3;

		if (color != 0)
		{
			for (int x, y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++) { zp[x] |= qp[x]; }
				if (mask) { zp[x] |= qp[x] & mask; }

				zp += z_row_offs;
				qp += q_row_offs;
			}
		}
		else
		{
			for (int x, y = 0; y < height; y++)
			{
				for (x = 0; x < width; x++) { zp[x] &= ~qp[x]; }
				if (mask) { zp[x] &= ~(qp[x] & mask); }

				zp += z_row_offs;
				qp += q_row_offs;
			}
		}
		return;
	}

	// source qp and destination zp are not aligned. we need to shift.
	// this could be optimized as in `copy_bits(uint32* zp, int zx, const uint32* qp, int qx, int cnt)`
	// with even more code bloating, especially as separate versions are needed for colormode_i1, _i2 and _i4.

	q_row_offs -= (width + 7) >> 3;
	z_row_offs -= (width + (zx >> CD) - 1) >> 3;

	for (int y = 0; y < height; y++)
	{
		uint8 zmask = uint8(pixelmask<CD> << zx); // mask for current pixel in zp[]
		uint8 zbyte = *zp;						  // target byte read from and stored back to zp[]
		uint8 qbyte = 0;						  // byte read from qp[]

		if (color != 0)
		{
			for (int x = 0; x < width; x++)
			{
				if (unlikely((x & 7) == 0)) { qbyte = *qp++; }
				if (unlikely(zmask == 0))
				{
					*zp = zbyte;
					zp++;
					zbyte = *zp;
					zmask = pixelmask<CD>;
				}
				if (qbyte & 1) { zbyte |= zmask; }
				qbyte >>= 1;
				zmask <<= 1 << CD;
			}
		}
		else
		{
			for (int x = 0; x < width; x++)
			{
				if (unlikely((x & 7) == 0)) { qbyte = *qp++; }
				if (unlikely(zmask == 0))
				{
					*zp = zbyte;
					zp++;
					zbyte = *zp;
					zmask = pixelmask<CD>;
				}
				if (qbyte & 1) { zbyte &= ~zmask; }
				qbyte >>= 1;
				zmask <<= 1 << CD;
			}
		}

		*zp = zbyte;

		qp += q_row_offs;
		zp += z_row_offs;
	}
}

void draw_bmp_2bpp(
	uint8* zp0, int zx, int z_row_offs, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_2bpp;
	color					= flood_filled_color<CD>(color);
	assert(width > 0);

	zp0 += zx >> 3;
	zx &= 7; // low 3 bit of bit address
	if (int(zp0) & 1)
	{
		zp0--;
		zx += 8;
	}

	uint16* zp = reinterpret_cast<uint16*>(zp0);

	if (zx == 0) // no need to shift
	{
		uint mask = ~(~0u << (width & 7)); // mask for last bits in q
		width	  = width >> 3;			   // count of full bytes to copy

		for (int x, y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint16 doubled_bits_mask = bitblit::double_bits(qp[x]);
				zp[x]					 = (zp[x] & ~doubled_bits_mask) | (color & doubled_bits_mask);
			}
			if (mask)
			{
				uint16 doubled_bits_mask = bitblit::double_bits(qp[x] & mask);
				zp[x]					 = (zp[x] & ~doubled_bits_mask) | (color & doubled_bits_mask);
			}

			zp += z_row_offs >> 1;
			qp += q_row_offs;
		}
		return;
	}

	// source qp and destination zp are not aligned. we need to shift.
	// this could be optimized as in `copy_bits(uint32* zp, int zx, const uint32* qp, int qx, int cnt)`
	// with even more code bloating, especially as separate versions are needed for colormode_i1, _i2 and _i4.

	q_row_offs -= (width + 7) >> 3;
	z_row_offs -= (width + (zx >> CD) - 1) >> 3;

	for (int y = 0; y < height; y++)
	{
		uint16 zmask = uint8(pixelmask<CD> << zx); // mask for current pixel in zp[]
		uint16 zbyte = *zp;						   // target byte read from and stored back to zp[]
		uint8  qbyte = 0;						   // byte read from qp[]

		for (int x = 0; x < width; x++)
		{
			if (unlikely((x & 7) == 0)) { qbyte = *qp++; }
			if (unlikely(zmask == 0))
			{
				*zp = zbyte;
				zp++;
				zbyte = *zp;
				zmask = pixelmask<CD>;
			}
			if (qbyte & 1) { zbyte = (zbyte & ~zmask) | (color & zmask); }
			qbyte >>= 1;
			zmask <<= 1 << CD;
		}

		*zp = zbyte;

		qp += q_row_offs;
		zp += z_row_offs;
	}
}

void draw_bmp_4bpp(
	uint8* zp0, int zx, int z_row_offs, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_4bpp;
	color					= flood_filled_color<CD>(color);
	assert(width > 0);

	zp0 += zx >> 3;
	zx &= 7; // low 3 bit of bit address
	if (int o = (int(zp0) & 3))
	{
		zp0 -= o;
		zx += o << 3;
	}

	uint32* zp = reinterpret_cast<uint32*>(zp0);

	if (zx == 0) // no need to shift
	{
		uint mask = ~(~0u << (width & 7)); // mask for last bits in q
		width	  = width >> 3;			   // count of full bytes to copy

		for (int x, y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint32 quadrupled_bits_mask = bitblit::quadruple_bits(qp[x]);
				zp[x]						= (zp[x] & ~quadrupled_bits_mask) | (color & quadrupled_bits_mask);
			}
			if (mask)
			{
				uint32 quadrupled_bits_mask = bitblit::quadruple_bits(qp[x] & mask);
				zp[x]						= (zp[x] & ~quadrupled_bits_mask) | (color & quadrupled_bits_mask);
			}

			zp += z_row_offs >> 2;
			qp += q_row_offs;
		}
		return;
	}

	// source qp and destination zp are not aligned. we need to shift.
	// this could be optimized as in `copy_bits(uint32* zp, int zx, const uint32* qp, int qx, int cnt)`
	// with even more code bloating, especially as separate versions are needed for colormode_i1, _i2 and _i4.

	q_row_offs -= (width + 7) >> 3;
	z_row_offs -= (width + (zx >> CD) - 1) >> 3;

	for (int y = 0; y < height; y++)
	{
		uint32 zmask = uint8(pixelmask<CD> << zx); // mask for current pixel in zp[]
		uint32 zbyte = *zp;						   // target byte read from and stored back to zp[]
		uint8  qbyte = 0;						   // byte read from qp[]

		for (int x = 0; x < width; x++)
		{
			if (unlikely((x & 7) == 0)) { qbyte = *qp++; }
			if (unlikely(zmask == 0))
			{
				*zp = zbyte;
				zp++;
				zbyte = *zp;
				zmask = pixelmask<CD>;
			}
			if (qbyte & 1) { zbyte = (zbyte & ~zmask) | (color & zmask); }
			qbyte >>= 1;
			zmask <<= 1 << CD;
		}

		*zp = zbyte;

		qp += q_row_offs;
		zp += z_row_offs >> 2;
	}
}

void draw_bmp_8bpp(
	uint8* zp, int z_row_offs, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	uint8 byte = 0;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (unlikely((x & 7) == 0)) { byte = qp[x >> 3]; }
			if (byte & 1) zp[x] = uint8(color);
			byte >>= 1;
		}

		qp += q_row_offs;
		zp += z_row_offs;
	}
}

void draw_bmp_16bpp(
	uint8* zp0, int z_row_offs, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	assert((int(zp0) & 1) == 0);

	uint16* zp = reinterpret_cast<uint16*>(zp0);
	z_row_offs = z_row_offs >> 1; // row offset is always in bytes
	uint8 byte = 0;

	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			if (unlikely((x & 7) == 0)) { byte = qp[x >> 3]; }
			if (byte & 1) zp[x] = uint16(color);
			byte >>= 1;
		}

		qp += q_row_offs;
		zp += z_row_offs;
	}
}

void draw_chr_1bpp(uint8* zp, int zx, int z_row_offs, const uint8* qp, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_1bpp;
	if (zx & 7) return draw_bmp_1bpp(zp, zx, z_row_offs, qp, 1, 8, height, color);

	color = flood_filled_color<CD>(color);
	zp += zx >> (3 - CD);

	if (color != 0)
	{
		for (int y = 0; y < height; y++)
		{
			*zp |= *qp++;
			zp += z_row_offs;
		}
	}
	else
	{
		for (int y = 0; y < height; y++)
		{
			*zp &= ~*qp++;
			zp += z_row_offs;
		}
	}
}

void draw_chr_2bpp(uint8* zp0, int zx, int z_row_offs, const uint8* qp, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_2bpp;

	zp0 += zx >> 3;
	zx &= 7;
	if (int(zp0) & 1)
	{
		zp0--;
		zx += 8;
	}

	if ((zx & 15) || (z_row_offs & 1)) { return draw_bmp_2bpp(zp0, zx, z_row_offs, qp, 1, 8, height, color); }

	color	   = flood_filled_color<CD>(color);
	uint16* zp = reinterpret_cast<uint16*>(zp0);

	for (int y = 0; y < height; y++)
	{
		uint16 doubled_bits_mask = bitblit::double_bits(*qp++);
		*zp						 = (*zp & ~doubled_bits_mask) | (color & doubled_bits_mask);
		zp += z_row_offs >> 1;
	}
}

void draw_chr_4bpp(uint8* zp0, int zx, int z_row_offs, const uint8* qp, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_4bpp;
	color					= flood_filled_color<CD>(color);

	zp0 += zx >> 3;
	zx &= 7;
	if (int o = (int(zp0) & 3))
	{
		zp0 -= o;
		zx += o << 3;
	}

	if ((zx & 31) || (z_row_offs & 3)) { return draw_bmp_4bpp(zp0, zx, z_row_offs, qp, 1, 8, height, color); }

	uint32* zp = reinterpret_cast<uint32*>(zp0);

	for (int y = 0; y < height; y++)
	{
		uint32 quadrupled_bits_mask = bitblit::quadruple_bits(*qp++);
		*zp							= (*zp & ~quadrupled_bits_mask) | (color & quadrupled_bits_mask);
		zp += z_row_offs >> 2;
	}
}


} // namespace kio::Graphics::bitblit
