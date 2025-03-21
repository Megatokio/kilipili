// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BitBlit.h"
#include "standard_types.h"

namespace kio::Graphics::bitblit
{

// #############################################################

void copy_bits(uint32* zp, int zx, const uint32* qp, int qx, int cnt) noexcept
{
	// copy row of bits with incrementing addresses to lower location
	// attn: left pixel is in lsb!
	//    => shift ops in source code are opposite direction to how pixels move!
	//
	// in: qp -> word which provides the first bits
	//     qx =  offset of bits from the left side
	//     zp -> word which receives the first bits
	//     zx =  offset of bits from the left side

	assert(uint(zx) <= 31);
	assert(uint(qx) <= 31);

	if (zx == qx) // no bit shifting required?
	{
		if (zx + cnt <= 32) // very few bits go into single word
		{
			uint32 mask = (1u << cnt) - 1; // mask for cnt bits
			mask		= mask << zx;	   // mask selects bits from q
			*zp			= (*zp & ~mask) | (*qp & mask);
			return;
		}

		if (zx) // copy first partial word
		{
			uint32 mask = ~0u << zx; // mask selects bits from q
			*zp			= (*zp & ~mask) | (*qp & mask);
			zp++;
			qp++;
			cnt -= 32 - zx;
		}

		while (cnt >= 64) // copy main part
		{
			*zp++ = *qp++;
			*zp++ = *qp++;
			cnt -= 64;
		}

		if (cnt >= 32)
		{
			*zp++ = *qp++;
			cnt -= 32;
		}

		if (cnt) // copy last partial word
		{
			uint32 mask = ~0u << cnt; // mask selects bits in z
			*zp			= (*zp & mask) | (*qp & ~mask);
		}
	}
	else // bit shifting required!
	{
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
}

void rcopy_bits(uint32* zp, int zx, const uint32* qp, int qx, int cnt) noexcept
{
	// copy row of bits with decrementing addresses to higher location
	// attn: left pixel is in lsb!
	//    => shift ops in source code are opposite direction to how pixels move!
	//
	// in: qp -> word which provides the last bits
	//     qx =  offset of bits from the right side
	//     zp -> word which receives the last bits
	//     zx =  offset of bits from the right side

	assert(uint(zx) <= 31);
	assert(uint(qx) <= 31);

	if (zx == qx) // no bit shifting required?
	{
		if (zx + cnt <= 32) // very few bits go into single word
		{
			uint32 mask = ~(~0u >> cnt); // mask for cnt bits
			mask		= mask >> zx;	 // mask selects bits from q
			*zp			= (*zp & ~mask) | (*qp & mask);
			return;
		}

		if (zx) // copy first partial word
		{
			uint32 mask = ~0u >> zx; // mask selects bits from q
			*zp			= (*zp & ~mask) | (*qp & mask);
			zp--;
			qp--;
			cnt -= 32 - zx;
		}

		while (cnt >= 64) // copy main part
		{
			*zp-- = *qp--;
			*zp-- = *qp--;
			cnt -= 64;
		}

		if (cnt >= 32)
		{
			*zp-- = *qp--;
			cnt -= 32;
		}

		if (cnt) // copy last partial word
		{
			uint32 mask = ~0u >> cnt; // mask selects bits in z
			*zp			= (*zp & mask) | (*qp & ~mask);
		}
	}
	else // bit shifting required!
	{
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
			uint32 mask = ~(~0u >> cnt); // mask for cnt bits
			mask		= mask >> zx;	 // mask selects bits from q

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

		if (cnt) // copy partial last word
		{
			uint32 mask = ~0u >> cnt; // mask selects bits in z
			uint32 byte = (lo >> lsl) | (hi << hsr);
			*zp			= (*zp & mask) | (byte & ~mask);
		}
	}
}

void copy_rect_of_bytes(uint8* zp, int zrow_offset, const uint8* qp, int qrow_offset, int w, int h) noexcept
{
	if (w <= 0 || h <= 0) return;

	if (zp <= qp)
	{
		while (--h >= 0)
		{
			memmove(zp, qp, uint(w));
			zp += zrow_offset;
			qp += qrow_offset;
		}
	}
	else
	{
		zp += h * zrow_offset;
		qp += h * qrow_offset;

		while (--h >= 0)
		{
			zp -= zrow_offset;
			qp -= qrow_offset;
			memmove(zp, qp, uint(w));
		}
	}
}

void copy_rect_of_bits(
	uint8* zp, int z_row_offs, int zx, const uint8* qp, int q_row_offs, int qx, int w, int h) noexcept
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

	if (w <= 0 || h <= 0) return;

	if (((zx | qx | w) & 7) == 0)
		return copy_rect_of_bytes(zp + zx / 8, z_row_offs, qp + qx / 8, q_row_offs, w >> 3, h);

	// we have some odd bits at either end and/or must shift bits:

	qp += qx >> 3;
	qx &= 7;

	zp += zx >> 3;
	zx &= 7;

	if (((q_row_offs | z_row_offs) & 3) == 0) // both pixmaps have aligned row offsets?
	{
		// if the row offset of both pixmaps is aligned to word address,
		// then zx and qx don't change from row to row:

		int		o	= ssize_t(zp) & 3;
		uint32* wzp = reinterpret_cast<uint32*>(zp - o);
		zx += o << 3;

		o				  = ssize_t(qp) & 3;
		const uint32* wqp = reinterpret_cast<const uint32*>(qp - o);
		qx += o << 3;

		if (wzp < wqp || (wzp == wqp && zx < qx)) // copy down
		{
			while (--h >= 0)
			{
				copy_bits(wzp, zx, wqp, qx, w);
				wzp += z_row_offs >> 2;
				wqp += q_row_offs >> 2;
			}
		}
		else // copy up
		{
			wzp += h * z_row_offs >> 2;
			wqp += h * q_row_offs >> 2;

			// flip the logic from ltr to rtl:
			wqp += (qx + w - 1) >> 5; // -> last word with q bits
			wzp += (zx + w - 1) >> 5; // -> last word with z bits
			qx = -(qx + w) & 31;	  // -> qx = offset of bits from the right side
			zx = -(zx + w) & 31;	  // -> zx = offset of bits from the right side

			while (--h >= 0)
			{
				wzp -= z_row_offs >> 2;
				wqp -= q_row_offs >> 2;
				rcopy_bits(wzp, zx, wqp, qx, w);
			}
		}
	}
	else // one or both pixmaps have an odd row offset!
	{
		// if a pixmap has an odd row offset,
		// then zx or qx changes from row to row!
		// if the 'oddity' of both pixmaps is different,
		// then whether zx == qx can change from row to row!

		if (zp < qp || (zp == qp && zx < qx)) // copy down
		{
			while (--h >= 0)
			{
				if (int o = ssize_t(zp) & 3)
				{
					zp -= o;
					zx += o << 3;
					zp += zx >> 5 << 2;
					zx &= 31;
				}

				if (int o = ssize_t(qp) & 3)
				{
					qp -= o;
					qx += o << 3;
					qp += qx >> 5 << 2;
					qx &= 31;
				}

				uint32*		  wzp = reinterpret_cast<uint32*>(zp);
				const uint32* wqp = reinterpret_cast<const uint32*>(qp);

				copy_bits(wzp, zx, wqp, qx, w);

				zp += z_row_offs;
				qp += q_row_offs;
			}
		}
		else // copy up
		{
			zp += z_row_offs * h;
			qp += q_row_offs * h;

			// flip the logic from ltr to rtl:
			qp += (qx + w - 1) >> 5 << 2; // -> last word with q bits
			zp += (zx + w - 1) >> 5 << 2; // -> last word with z bits
			qx = -(qx + w) & 31;		  // -> qx = offset of bits from the right side
			zx = -(zx + w) & 31;		  // -> zx = offset of bits from the right side

			while (--h >= 0)
			{
				zp -= z_row_offs;
				qp -= q_row_offs;

				if (int o = ssize_t(zp) & 3)
				{
					zp -= o;
					zx -= o << 3;
					if (zx < 0) zp += 4;
					zx &= 31;
				}

				if (int o = ssize_t(qp) & 3)
				{
					qp -= o;
					qx -= o << 3;
					if (qx < 0) qp += 4;
					qx &= 31;
				}

				uint32*		  wzp = reinterpret_cast<uint32*>(zp);
				const uint32* wqp = reinterpret_cast<const uint32*>(qp);

				rcopy_bits(wzp, zx, wqp, qx, w);
			}
		}
	}
}


// #############################################################

void clear_row(uint32* z, int w, uint32 color) noexcept
{
	while (--w >= 0) { *z++ = color; }
}

void clear_row(uint16* z, int w, uint32 color) noexcept
{
	if (unlikely(w <= 0)) return;

	if (ssize_t(z) & 2)
	{
		*z++ = uint16(color);
		w--;
	}
	if (w & 1) { z[--w] = uint16(color); }
	clear_row(reinterpret_cast<uint32*>(z), w >> 1, color);
}

void clear_row(uint8* z, int w, uint32 color) noexcept
{
	if (unlikely(w <= 0)) return;

	if (ssize_t(z) & 1)
	{
		*z++ = uint8(color);
		w--;
	}
	if (w & 1) { z[--w] = uint8(color); }
	clear_row(reinterpret_cast<uint16*>(z), w >> 1, color);
}

void clear_row_of_bits(uint8* zp, int x0, int width, uint32 color) noexcept
{
	if (width <= 0) return;

	// add full bytes from xoffs to zp:
	zp += x0 >> 3;
	x0 &= 7;

	// align zp to int32:
	int		o	= ssize_t(zp) & 3;
	uint32* wzp = reinterpret_cast<uint32*>(zp - o);
	x0 += o << 3;

	// mask for bits to set at left end:  (note: lsb is left!)
	int keep = x0;
	width += keep;
	uint32 lmask = ~0u << keep;

	// mask for bits to set at right end:
	keep = -width & 31;
	width += keep;
	uint32 rmask = ~0u >> keep;

	int cnt = width >> 5;
	assert(cnt > 0);

	if (cnt == 1)
	{
		color &= lmask & rmask;
		uint32 mask = ~(lmask & rmask); // bits to keep

		*wzp++ = (*wzp & mask) | color;
	}
	else
	{
		uint32 lcolor = color & lmask;
		uint32 rcolor = color & rmask;
		lmask		  = ~lmask; // bits to keep
		rmask		  = ~rmask; // bits to keep

		*wzp++ = (*wzp & lmask) | lcolor;
		for (int i = 0; i < cnt - 2; i++) { *wzp++ = color; }
		*wzp++ = (*wzp & rmask) | rcolor;
	}
}

void clear_rect_of_bits_with_mask(
	uint8* zp, int row_offset, int xoffs, int width, int height, uint32 color, uint32 mask) noexcept
{
	if (unlikely(width <= 0 || height <= 0)) return;

	if (unlikely(row_offset & 3))
	{
		// if row_offset is not a multiple of 4 then alignment will change from row to row.
		// => Double the row offset and clear only every 2nd row in each of two rounds
		// or quadruple the row offset and do 4 rounds.

		do {
			clear_rect_of_bits_with_mask(zp, row_offset << 1, xoffs, width, (height + 1) >> 1, color, mask);
			//clear_rect_of_bits_with_mask(zp + row_offset, row_offset << 1, xoffs, width, (height + 0) >> 1, color, mask);
			zp += row_offset;
			row_offset <<= 1;
			height >>= 1;
		}
		while (row_offset & 2);
	}

	// row_offset is a multiple of 4
	// => alignment from row to row won't change!

	// align zp to int32:
	int o = ssize_t(zp) & 3;
	xoffs += o << 3;
	uint32* p = reinterpret_cast<uint32*>(zp - o);

	// add full words from xoffs to zp:
	p += xoffs >> 5;
	xoffs &= 31;
	if (xoffs) mask = (mask << xoffs) | (mask >> (32 - xoffs));

	// mask for bits to set at left end:  (note: lsb is left!)
	int keep = xoffs;
	width += keep;
	uint32 lmask = (~0u << keep) & mask;

	// mask for bits to set at right end:
	keep = -width & 31;
	width += keep;
	uint32 rmask = (~0u >> keep) & mask;

	int cnt = width >> 5;
	int dp	= (row_offset >> 2) - cnt;
	assert(cnt > 0);

	if (cnt == 1)
	{
		color &= lmask & rmask;
		mask = ~(lmask & rmask); // bits to keep

		while (--height >= 0)
		{
			*p++ = (*p & mask) | color;
			p += dp;
		}
	}
	else
	{
		uint32 lcolor = color & lmask;
		uint32 rcolor = color & rmask;
		color &= mask;
		lmask = ~lmask; // bits to keep
		rmask = ~rmask; // bits to keep
		mask  = ~mask;	// bits to keep

		while (--height >= 0)
		{
			*p++ = (*p & lmask) | lcolor;
			for (int i = 0; i < cnt - 2; i++) { *p++ = (*p & mask) | color; }
			*p++ = (*p & rmask) | rcolor;
			p += dp;
		}
	}
}

void clear_rect_of_bits(uint8* zp, int row_offset, int xoffs, int width, int height, uint32 color) noexcept
{
	if (unlikely(width <= 0 || height <= 0)) return;

	if (unlikely(row_offset & 3))
	{
		// if row_offset is not a multiple of 4 then alignment will change from row to row.
		// => Double the row offset and clear only every 2nd row in each of two rounds
		// or quadruple the row offset and do 4 rounds.

		do {
			clear_rect_of_bits(zp, row_offset << 1, xoffs, width, (height + 1) >> 1, color);
			//clear_rect_of_bits(zp + row_offset, row_offset << 1, xoffs, width, (height + 0) >> 1, color);
			zp += row_offset;
			row_offset <<= 1;
			height >>= 1;
		}
		while (row_offset & 2);
	}

	// row_offset is a multiple of 4
	// => alignment from row to row won't change!

	// add full bytes from xoffs to zp:
	zp += xoffs >> 3;
	xoffs &= 7;

	// align zp to int32:
	int		o = ssize_t(zp) & 3;
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

	if (cnt == 1)
	{
		color &= lmask & rmask;
		uint32 mask = ~(lmask & rmask); // bits to keep

		while (--height >= 0)
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

		while (--height >= 0)
		{
			*p++ = (*p & lmask) | lcolor;
			for (int i = 0; i < cnt - 2; i++) { *p++ = color; }
			*p++ = (*p & rmask) | rcolor;
			p += dp;
		}
	}
}

void xor_rect_of_bits(uint8* zp, int row_offset, int xoffs, int width, int height, uint32 color) noexcept
{
	if (unlikely(width <= 0 || height <= 0)) return;

	if (unlikely(row_offset & 3))
	{
		// if row_offset is not a multiple of 4 then alignment will change from row to row.
		// => Double the row offset and clear only every 2nd row in each of two rounds
		// or quadruple the row offset and do 4 rounds.

		do {
			xor_rect_of_bits(zp, row_offset << 1, xoffs, width, (height + 1) >> 1, color);
			//xor_rect_of_bits(zp + row_offset, row_offset << 1, xoffs, width, (height + 0) >> 1, color);
			zp += row_offset;
			row_offset <<= 1;
			height >>= 1;
		}
		while (row_offset & 2);
	}

	// row_offset is a multiple of 4
	// => alignment from row to row won't change!

	// add full bytes from xoffs to zp:
	zp += xoffs >> 3;
	xoffs &= 7;

	// align zp to int32:
	int		o = ssize_t(zp) & 3;
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

	if (cnt == 1)
	{
		color &= lmask & rmask;

		while (height--)
		{
			*p++ ^= color;
			p += dp;
		}
	}
	else
	{
		uint32 lcolor = color & lmask;
		uint32 rcolor = color & rmask;

		while (height--)
		{
			*p++ ^= lcolor;
			for (int i = 0; i < cnt - 2; i++) { *p++ ^= color; }
			*p++ ^= rcolor;
			p += dp;
		}
	}
}


// #############################################################

template<>
void draw_bitmap<colordepth_1bpp>(
	uint8* zp, int z_row_offs, int x0, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	if (width <= 0 || height <= 0) return;

	constexpr ColorDepth CD = colordepth_1bpp;
	color					= flood_filled_color<CD>(color);
	x0						= x0 << CD; // nop

	// for colordepth_i1 we go at extra length for optimization
	// and distinguish between color = 0 and color = 1
	// because these are only 2 cases and most attribute modes use colordepth_i1 in the pixels[]

	zp += x0 >> 3;
	x0 &= 7; // low 3 bit of bit address

	if (x0 == 0) // no need to shift
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
	z_row_offs -= (width + (x0 >> CD) - 1) >> 3;

	for (int y = 0; y < height; y++)
	{
		uint8 zmask = uint8(pixelmask<CD> << x0); // mask for current pixel in zp[]
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

template<>
void draw_bitmap<colordepth_2bpp>(
	uint8* zp, int zrow_offs, int x0, const uint8* qp, int qrow_offs, int width, int height, uint color) noexcept
{
	if (width <= 0 || height <= 0) return;

	constexpr ColorDepth CD = colordepth_2bpp;

	x0 = x0 << CD;
	zp += x0 >> 3;
	x0 &= 7;
	int o = ssize_t(zp) & 1;
	zp -= o;
	x0 += o << 3;

	if (x0 == 0 && (zrow_offs & 1) == 0) // no need to shift
	{
		uint16* wzp = reinterpret_cast<uint16*>(zp);
		color		= flood_filled_color<CD>(color);

		uint mask = ~(~0u << (width & 7)); // mask for last bits in q
		width	  = width >> 3;			   // count of full bytes to copy

		for (int x, y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint16 doubled_bits_mask = bitblit::double_bits(qp[x]);
				wzp[x]					 = (wzp[x] & ~doubled_bits_mask) | (color & doubled_bits_mask);
			}
			if (mask)
			{
				uint16 doubled_bits_mask = bitblit::double_bits(qp[x] & mask);
				wzp[x]					 = (wzp[x] & ~doubled_bits_mask) | (color & doubled_bits_mask);
			}

			wzp += zrow_offs >> 1;
			qp += qrow_offs;
		}
		return;
	}

	// source qp and destination zp are not aligned. we need to shift.
	// TODO: optimize

	draw_bitmap_ref<CD>(zp, zrow_offs, x0 >> CD, qp, qrow_offs, width, height, color);
}

template<>
void draw_bitmap<colordepth_4bpp>(
	uint8* zp, int zrow_offs, int x0, const uint8* qp, int qrow_offs, int width, int height, uint color) noexcept
{
	if (width <= 0 || height <= 0) return;

	constexpr ColorDepth CD = colordepth_4bpp;

	x0 = x0 << CD;
	zp += x0 >> 3;
	x0 &= 7;
	int o = (ssize_t(zp) & 3);
	zp -= o;
	x0 += o << 3;

	if (x0 == 0) // no need to shift
	{
		uint32* wzp = reinterpret_cast<uint32*>(zp);
		color		= flood_filled_color<CD>(color);

		uint mask = ~(~0u << (width & 7)); // mask for last bits in q
		width	  = width >> 3;			   // count of full bytes to copy

		for (int x, y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				uint32 quadrupled_bits_mask = bitblit::quadruple_bits(qp[x]);
				wzp[x]						= (wzp[x] & ~quadrupled_bits_mask) | (color & quadrupled_bits_mask);
			}
			if (mask)
			{
				uint32 quadrupled_bits_mask = bitblit::quadruple_bits(qp[x] & mask);
				wzp[x]						= (wzp[x] & ~quadrupled_bits_mask) | (color & quadrupled_bits_mask);
			}

			wzp += zrow_offs >> 2;
			qp += qrow_offs;
		}
		return;
	}

	// source qp and destination zp are not aligned. we need to shift.
	// TODO: optimize

	draw_bitmap_ref<CD>(zp, zrow_offs, x0 >> CD, qp, qrow_offs, width, height, color);
}

template<>
void draw_bitmap<colordepth_8bpp>(
	uint8* zp, int z_row_offs, int x0, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	if (width <= 0 || height <= 0) return;

	zp += x0;
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

template<>
void draw_bitmap<colordepth_16bpp>(
	uint8* zp0, int z_row_offs, int x0, const uint8* qp, int q_row_offs, int width, int height, uint color) noexcept
{
	if (width <= 0 || height <= 0) return;

	assert((ssize_t(zp0) & 1) == 0);

	uint16* zp = reinterpret_cast<uint16*>(zp0);
	zp += x0;
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

template<>
void draw_char<colordepth_1bpp>(uint8* zp, int z_row_offs, int x0, const uint8* qp, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_1bpp;
	if (x0 & 7) return draw_bitmap<CD>(zp, z_row_offs, x0, qp, 1, 8, height, color);

	color = flood_filled_color<CD>(color);
	x0 <<= CD; // nop
	zp += x0 >> 3;

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

template<>
void draw_char<colordepth_2bpp>(uint8* zp, int z_row_offs, int x0, const uint8* qp, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_2bpp;

	x0 <<= CD;
	zp += x0 >> 3;
	x0 &= 7;
	int o = ssize_t(zp) & 1;
	zp -= o;
	x0 += o << 3;

	if ((x0 & 15) || (z_row_offs & 1)) { return draw_bitmap<CD>(zp, z_row_offs, x0 >> CD, qp, 1, 8, height, color); }

	color		= flood_filled_color<CD>(color);
	uint16* wzp = reinterpret_cast<uint16*>(zp);

	for (int y = 0; y < height; y++)
	{
		uint16 doubled_bits_mask = bitblit::double_bits(*qp++);
		*wzp					 = (*wzp & ~doubled_bits_mask) | (color & doubled_bits_mask);
		wzp += z_row_offs >> 1;
	}
}

template<>
void draw_char<colordepth_4bpp>(uint8* zp, int z_row_offs, int x0, const uint8* qp, int height, uint color) noexcept
{
	constexpr ColorDepth CD = colordepth_4bpp;

	x0 <<= CD;
	zp += x0 >> 3;
	x0 &= 7;
	int o = (ssize_t(zp) & 3);
	zp -= o;
	x0 += o << 3;

	if ((x0 & 31) || (z_row_offs & 3)) { return draw_bitmap<CD>(zp, z_row_offs, x0 >> CD, qp, 1, 8, height, color); }

	color		= flood_filled_color<CD>(color);
	uint32* wzp = reinterpret_cast<uint32*>(zp);

	for (int y = 0; y < height; y++)
	{
		uint32 quadrupled_bits_mask = bitblit::quadruple_bits(*qp++);
		*wzp						= (*wzp & ~quadrupled_bits_mask) | (color & quadrupled_bits_mask);
		wzp += z_row_offs >> 2;
	}
}


} // namespace kio::Graphics::bitblit

/*




































*/
