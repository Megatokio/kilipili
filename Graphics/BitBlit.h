// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "graphics_types.h"
#include "kilipili_common.h"


/*
	CPU Rendering Bit Blit Library

	There are two sections:
	1. color depth agnostic functions working on bits
	2. templates (mostly) which work on pixmaps with known color depth
*/


namespace kio::Graphics
{

/* Helper: calculate mask for n bits:
*/
inline constexpr uint32 bitmask(uint n) noexcept { return (1 << n) - 1; }

/* Helper: calculate mask for one pixel in ColorDepth CD:
*/
template<ColorDepth CD>
constexpr uint32 pixelmask = bitmask(1 << CD);

/* Helper: spread color across whole uint32:
*/
template<ColorDepth>
constexpr uint32 flood_filled_color(uint color) noexcept;
template<>
constexpr uint32 flood_filled_color<colordepth_1bpp>(uint color) noexcept
{
	return (color & 0x01) * 0xffffffffu;
}
template<>
constexpr uint32 flood_filled_color<colordepth_2bpp>(uint color) noexcept
{
	return (color & 0x03) * 0x55555555u;
}
template<>
constexpr uint32 flood_filled_color<colordepth_4bpp>(uint color) noexcept
{
	return (color & 0x0f) * 0x11111111u;
}
template<>
constexpr uint32 flood_filled_color<colordepth_8bpp>(uint color) noexcept
{
	return uint8(color) * 0x01010101u;
}
template<>
constexpr uint32 flood_filled_color<colordepth_16bpp>(uint color) noexcept
{
	return uint16(color) * 0x00010001u;
}
template<ColorMode CM>
constexpr uint32 flood_filled_color(uint color) noexcept
{
	return flood_filled_color<get_colordepth(CM)>(color);
}
} // namespace kio::Graphics


namespace kio::Graphics::bitblit
{

inline constexpr uint32 bitmask(uint n) noexcept { return (1 << n) - 1; }

/* stretch bitmask to double width:
*/
inline constexpr uint16 double_bits(uint8 bits) noexcept
{
	uint16 n = bits;
	n		 = (n | (n << 4)) & 0x0f0f;
	n		 = (n | (n << 2)) & 0x3333;
	n		 = (n | (n << 1)) & 0x5555;
	return n * 3;
}

static_assert(double_bits(0x0f) == 0x00ff);
static_assert(double_bits(0xA5) == 0xcc33);

/* stretch bitmask to quadruple width:
*/
inline constexpr uint32 quadruple_bits(uint8 bits) noexcept
{
	uint32 n = bits;
	n		 = (n | (n << 12)) & 0x000f000f;
	n		 = (n | (n << 6)) & 0x03030303;
	n		 = (n | (n << 3)) & 0x11111111;
	return n * 15;
}

static_assert(quadruple_bits(0x0f) == 0x0000ffff);
static_assert(quadruple_bits(0xA5) == 0xf0f00f0f);

inline constexpr uint8 reduce_bits_2bpp(uint bits) noexcept
{
	// reduces every 2 bits -> 1 bit
	// bit = bits != 0b00

	bits = (bits | (bits >> 1)) & 0x5555; // for all 8 bits:  bit = bits != 0b00
	bits = (bits | (bits >> 1)) & 0x3333; // now shift them into position
	bits = (bits | (bits >> 2)) & 0x0f0f;
	bits = (bits | (bits >> 4));
	return uint8(bits);
}

inline constexpr uint8 reduce_bits_4bpp(uint32 bits) noexcept
{
	// reduces every 4 bits -> 1 bit
	// bit = bits != 0b0000

	bits = (bits | (bits >> 2));			  // for 8 pairs:     xx = aa | bb
	bits = (bits | (bits >> 1)) & 0x11111111; // for all 8 bits:  bit = bits != 0b0000
	bits = (bits | (bits >> 3)) & 0x03030303; // now shift them into position
	bits = (bits | (bits >> 6)) & 0x000f000f;
	bits = (bits | (bits >> 12));
	return uint8(bits);
}

static_assert(reduce_bits_4bpp(uint32(0xffff0000)) == 0xf0);
static_assert(reduce_bits_4bpp(uint32(0x00110101)) == 0x35);
static_assert(reduce_bits_4bpp(uint32(0x00000804)) == 0x05);
static_assert(reduce_bits_2bpp(uint16(0xff00)) == 0xf0);
static_assert(reduce_bits_2bpp(uint16(0xc8A5)) == 0xaf);


/* clear row of bytes, halfwords or words with flood_filled_color.
*/
extern void clear_row(uint8* z, int num_bytes, uint32 flood_filled_color) noexcept;
extern void clear_row(uint16* z, int num_halfwords, uint32 flood_filled_color) noexcept;
extern void clear_row(uint32* z, int num_words, uint32 flood_filled_color) noexcept;

/* clear row of bits with color

	row = pointer to the start of the row
	xoffs_bits = x position measured in bits
	width_bits = width in bits
	flood_filled_color = 32 bit flood filled color
*/
extern void clear_row_of_bits(uint8* row, int xoffs_bits, int width_bits, uint32 flood_filled_color) noexcept;
extern void xor_row_of_bits(uint8* row, int xoffs_bits, int width_bits, uint32 flood_filled_xor_color) noexcept;

/* clear row of bits with color, masked with bitmask
   this is intended to set color attributes for a hline
   to set only one color in each attribute.

	row = pointer to the start of the row
	xoffs_bits = x position measured in bits
	width_bits = width in bits
	flood_filled_color = 32 bit flood filled color
	flood_filled_mask = 32 bit wide mask for bits to set
*/
extern void clear_row_of_bits_with_mask(
	uint8* row, int xoffs_bits, int width_bits, uint32 flood_filled_color, uint32 flood_filled_mask) noexcept;

/* clear a rectangular area with bit boundary precision.

	row0 = pointer to the first row
	xpos_bits = x position measured in bits
	row_offset = row offset in bytes
	width_bits = width in bits
	height = height in rows
	flood_filled_color = 32 bit flood filled color
*/
extern void clear_rect_of_bits(
	uint8* row0, int xpos_bits, int row_offset, int width_bits, int height, uint32 flood_filled_color) noexcept;

/* clear a rectangular area with byte boundary precision.

	pos0 = pointer to the first byte (top-left corner)
	row_offset = row offset in bytes
	width_bytes = width in bytes
	height = height in rows
	flood_filled_color = 32 bit flood filled color
*/
inline void
clear_rect_of_bytes(uint8* pos0, int row_offset, int width_bytes, int height, uint32 flood_filled_color) noexcept
{
	clear_rect_of_bits(pos0, 0, row_offset, width_bytes << 3, height, flood_filled_color);
}

/* toggle colors in a rectangular area with bit boundary precision.

	pos0 = pointer to first row
	xpos_bits = x position measured in bits from pos0
	row_offset = row offset in bytes
	width_bits = width in bits
	height = height in rows
	flood_filled_xor_color = 32 bit flood filled xor pattern for color
*/
extern void xor_rect_of_bits(
	uint8* pos0, int xpos_bits, int row_offset, int width_bits, int height, uint32 flood_filled_xor_color) noexcept;

inline void
xor_rect_of_bytes(uint8* pos0, int row_offset, int width_bytes, int height, uint32 flood_filled_xor_color) noexcept
{
	xor_rect_of_bits(pos0, 0, row_offset, width_bytes << 3, height, flood_filled_xor_color);
}

/* copy rectangular area within one or between two pixmap to another with bit boundary precision.

	zrow0 = pointer to the first row in destination
	zxpos_bits = x position measured in bits
	z_row_offs = row offset in bytes
	qrow0 = pointer to the first row in source
	qxpos_bits = x position measured in bits
	q_row_offs = row offset in bytes
	w_bits = width in bits
	h = height in rows
*/
extern void copy_rect_of_bits(
	uint8* zrow0, int zxpos_bits, int z_row_offs, const uint8* qrow0, int qxpos_bits, int q_row_offs, int w_bits,
	int h) noexcept;

/* copy rectangular area within one or between two pixmaps with byte boundary precision.

	zpos0 = pointer to the first byte of destination (top left corner)
	z_row_offs = row offset in bytes
	qpos0 = pointer to the first byte of source (top left corner)
	q_row_offs = row offset in bytes
	w = width in bytes
	h = height in rows
*/
extern void copy_rect_of_bytes(uint8* zpos0, int z_row_offs, const uint8* qpos0, int q_row_offs, int w, int h) noexcept;


// ######################################################################################################
// ######################################################################################################
// ######################################################################################################


/** Read single pixel from Pixmap in ColorDepth CD.
	@param row	 start of row with the pixel
	@param x	 x offset from `row` in pixels
	@return		 the pixel value
*/
template<ColorDepth CD>
uint get_pixel(const uint8* row, int x) noexcept
{
	if constexpr (CD == colordepth_1bpp)
	{
		int shift = (x & 7) << 0;
		return (row[x >> 3] >> shift) & 1;
	}
	else if constexpr (CD == colordepth_2bpp)
	{
		int shift = (x & 3) << 1;
		return (row[x >> 2] >> shift) & 3;
	}
	else if constexpr (CD == colordepth_4bpp)
	{
		uint8 byte = row[x >> 1];
		return x & 1 ? byte >> 4 : byte & 0x0f;
	}
	else if constexpr (CD == colordepth_8bpp) { return row[x]; }
	else if constexpr (CD == colordepth_16bpp) { return reinterpret_cast<const uint16*>(row)[x]; }
	else IERR();
}

/** Set single pixel in Pixmap in ColorDepth CD.
	@param row	  start of row for the pixel
	@param x	  x offset from `row` in pixels
	@param color  new color for the pixel
*/
template<ColorDepth CD>
void set_pixel(uint8* row, int x, uint color) noexcept
{
	if constexpr (CD == colordepth_1bpp)
	{
		row += x >> 3;
		if (color & 1) *row |= 0x01 << (x & 7);
		else *row &= ~(0x01 << (x & 7));
	}
	else if constexpr (CD == colordepth_2bpp)
	{
		row += x >> 2;
		int shift = 2 * (x & 3);
		*row	  = uint8((*row & ~(0x03 << shift)) | ((color & 0x03) << shift));
	}
	else if constexpr (CD == colordepth_4bpp)
	{
		row += x >> 1;
		if (x & 1) *row = uint8((*row & 0x0f) | (color << 4));
		else *row = uint8((*row & 0xf0) | (color & 0x0f));
	}
	else if constexpr (CD == colordepth_8bpp) { row[x] = uint8(color); }
	else if constexpr (CD == colordepth_16bpp) { reinterpret_cast<uint16*>(row)[x] = uint16(color); }
	else IERR();
}

/** draw vertical line in pixmap
	@tparam CD		ColorDepth of the pixmap
	@param row		start of top row
	@param row_offset address offset between rows
	@param x    	xpos measured from `row` in pixels
	@param height	height of vertical line, measured in pixels
	@param color	color to draw
*/
template<ColorDepth CD>
void draw_vline(uint8* row, int row_offset, int x, int height, uint color) noexcept
{
	if constexpr (CD == colordepth_1bpp)
	{
		row += x >> 3;
		uint mask = 1u << (x & 7);
		if (color)
		{
			while (--height >= 0)
			{
				*row |= mask;
				row += row_offset;
			}
		}
		else
		{
			mask = ~mask;
			while (--height >= 0)
			{
				*row &= mask;
				row += row_offset;
			}
		}
	}
	else if constexpr (CD <= colordepth_4bpp)
	{
		x <<= CD;
		row += x >> 3;
		x &= 7;
		uint mask = pixelmask<CD> << x;
		color	  = (color << x) & mask;
		mask	  = ~mask;
		while (--height >= 0)
		{
			*row = uint8((*row & mask) | color);
			row += row_offset;
		}
	}
	else if constexpr (CD == colordepth_8bpp)
	{
		row += x;
		while (--height >= 0)
		{
			*row = uint8(color);
			row += row_offset;
		}
	}
	else if constexpr (CD == colordepth_16bpp)
	{
		row += x << 1;
		while (--height >= 0)
		{
			*reinterpret_cast<uint16*>(row) = uint16(color);
			row += row_offset;
		}
	}
	else IERR();
}

/** draw horizontal line in pixmap
	@tparam CD		ColorDepth of the pixmap
	@param row		start of top row
	@param x    	xpos measured from `row` in pixels
	@param width	width of horizontal line, measured in pixels
	@param color	color for drawing
*/
template<ColorDepth CD>
void draw_hline(uint8* row, int x, int width, uint color) noexcept
{
	if constexpr (CD <= colordepth_4bpp)
	{
		clear_row_of_bits(row, x << CD, width << CD, flood_filled_color<CD>(color));
	}
	else if constexpr (CD == colordepth_8bpp) { clear_row(row + x, width, flood_filled_color<colordepth_8bpp>(color)); }
	else if constexpr (CD == colordepth_16bpp)
	{
		clear_row(reinterpret_cast<uint16*>(row) + x, width, flood_filled_color<colordepth_16bpp>(color));
	}
	else IERR();
}

/** draw dotted horizontal line in pixmap.
	draws only every 2nd, 4th or 16th pixel depending on AttrMode AM.
	this is intended to set the colors in the color attributes for a horizontal line in tiled Pixmaps.

	note that x1 and width are the coordinates in the attributes[],
		not the x coordinate and width used to draw the line in the pixels[] of the Pixmap.
	therefore:
		x1 = x/attrwidth * num_colors_in_attr + pixel_value
		   = (x >> AW) << (1 << AM) + pixel

	@tparam	AM		AttrMode, essentially the color depth of the pixmap itself
	@tparam CD		ColorDepth of the colors in the attributes
	@param row		start of top row
	@param x    	xpos measured from `row` in pixels
	@param width	width of horizontal line, measured in pixels
	@param color	color for drawing
*/
template<AttrMode AM, ColorDepth CD>
void attr_draw_hline(uint8* row, int x, int width, uint color) noexcept
{
	constexpr int num_colors = 1 << (1 << AM); // number of colors per attribute

	if constexpr (CD == colordepth_16bpp)
	{
		uint16* p = reinterpret_cast<uint16*>(row) + x;
		for (int i = 0; i < width; i += num_colors) { p[i] = uint16(color); }
	}
	else if constexpr (CD == colordepth_8bpp)
	{
		uint8* p = row + x;
		for (int i = 0; i < width; i += num_colors) { p[i] = uint8(color); }
	}
	else // 1bpp .. 4bpp colors
	{
		constexpr int bits_per_pixel = 1 << AM; // 1,2,4
		constexpr int bits_per_color = 1 << CD; // 1,2,4
		constexpr int bits_per_attr	 = bits_per_color << bits_per_pixel;

		// assuming that it's only useful to have more bits per colors than bits per pixel
		// so that the pixels actually choose from more colors than they could directly encode themself,
		// we would assume only combinations 1bpp+2bpc, 1bpp+4bpc and 2bpp+4bpc reasonable here.
		// this gives at most 16 bit per attribute (color table).
		// though other combinations may be useful in special cases, e.g. for inverting or blinking colors.
		// combination 4bpp+4bpc can actually not be represented here because it results in 64 bits.
		// the largest colortable is for 4bpp+2bpc = 2<<4 = 32 bit in size, if ever deemed useful.
		static_assert(bits_per_attr <= 32);

		auto constexpr flooded = [](int bits, uint32 v) noexcept {
			while (bits < 32)
			{
				v += v << bits;
				bits *= 2;
			}
			return v;
		};
		constexpr uint32 mask = flooded(bits_per_attr, pixelmask<CD>);

		if (unlikely(width <= 0)) return;
		clear_row_of_bits_with_mask(row, x << CD, width << CD, flood_filled_color<CD>(color), mask);
	}
}

/**	clear every 2nd, 4th or 16th column in rectangle depending on AttrMode AM.
	this is intended to set the colors in the color attributes for a rectangle in Pixmaps with attributes.
	@tparam	AM		AttrMode, essentially the color depth of the pixmap itself
	@tparam CD		ColorDepth of the colors in the attributes
	@param row		start of top row
	@param row_offs	address offset between rows
	@param x    	xpos measured from `row` in pixels (colors)
	@param w		width of rectangle, measured in pixels (colors) in the attributes
	@param h		width of rectangle, measured in pixels (rows) in the attributes
	@param color	color for drawing
*/
template<AttrMode AM, ColorDepth CD>
void attr_clear_rect(uint8* row, int row_offs, int x, int w, int h, uint color) noexcept
{
	while (--h >= 0)
	{
		attr_draw_hline<AM, CD>(row, x, w, color);
		row += row_offs;
	}
}

/** copy rectangular area inside a pixmap or from one pixmap to another.
	handles overlap properly.

	@param zp		 ptr to start of destiantion row
	@param zrow_offs row offset in destination measured in bytes
	@param zx		 x position in pixels
	@param qp		 ptr to start of source row
	@param qrow_offs row offset in source measured in bytes
	@param qx		 x position in pixels
	@param w		 width in pixels
	@param h		 height in pixels
*/
template<ColorDepth CD>
void copy_rect(uint8* zp, int zrow_offs, int zx, const uint8* qp, int qrow_offs, int qx, int w, int h) noexcept
{
	if constexpr (CD >= colordepth_8bpp)
	{
		if (w > 0 && h > 0)
			bitblit::copy_rect_of_bytes(
				zp + (zx << (CD - 3)), zrow_offs, qp + (qx << (CD - 3)), qrow_offs, w << (CD - 3), h);
	}
	else
	{
		if (w > 0 && h > 0) bitblit::copy_rect_of_bits(zp, zx << CD, zrow_offs, qp, qx << CD, qrow_offs, w << CD, h);
	}
}

// helper:
extern void draw_bmp_1bpp(
	uint8* zp, int zx, int z_row_offs, const uint8* qp, int q_row_offs, int width_pixels, int height,
	uint color) noexcept;
extern void draw_bmp_2bpp(
	uint8* zp, int zx, int z_row_offs, const uint8* qp, int q_row_offs, int width_pixels, int height,
	uint color) noexcept;
extern void draw_bmp_4bpp(
	uint8* zp, int zx, int z_row_offs, const uint8* qp, int q_row_offs, int width_pixels, int height,
	uint color) noexcept;
extern void draw_bmp_8bpp(
	uint8* zp, int z_row_offs, const uint8* qp, int q_row_offs, int width_pixels, int height, uint color) noexcept;
extern void draw_bmp_16bpp(
	uint8* zp, int z_row_offs, const uint8* qp, int q_row_offs, int width_pixels, int height, uint color) noexcept;
extern void draw_chr_1bpp(uint8* zp, int zx, int z_row_offs, const uint8* qp, int height, uint color) noexcept;
extern void draw_chr_2bpp(uint8* zp, int zx, int z_row_offs, const uint8* qp, int height, uint color) noexcept;
extern void draw_chr_4bpp(uint8* zp, int zx, int z_row_offs, const uint8* qp, int height, uint color) noexcept;

/** draw Bitmap into destination Pixmap of any color depth.
	draws the '1' bits in the given color, while '0' bits are left transparent.
	if you want to draw the '0' in a certain color too then clear the area with that color first.

	@param zp		 start of the first row in destination Pixmap
	@param zrow_offs row offset in destination Pixmap measured in bytes
	@param x		 x offset from zp in pixels
	@param qp		 start of the first byte of source Bitmap
	@param qrow_offs row offset in source Bitmap measured in bytes
	@param w		 width in pixels
	@param h		 height in pixels
	@param color	 color for drawing the bitmap
*/
template<ColorDepth CD>
void draw_bitmap(uint8* zp, int zrow_offs, int x, const uint8* qp, int qrow_offs, int w, int h, uint color) noexcept
{
	if constexpr (CD >= colordepth_16bpp)
	{
		draw_bmp_16bpp(zp + (x << (CD >> 3)), zrow_offs, qp, qrow_offs, w, h, color);
	}
	else if constexpr (CD == colordepth_8bpp)
	{
		draw_bmp_8bpp(zp + (x << (CD >> 3)), zrow_offs, qp, qrow_offs, w, h, color);
	}
	else if constexpr (CD == colordepth_4bpp) { draw_bmp_4bpp(zp, x << CD, zrow_offs, qp, qrow_offs, w, h, color); }
	else if constexpr (CD == colordepth_2bpp) { draw_bmp_2bpp(zp, x << CD, zrow_offs, qp, qrow_offs, w, h, color); }
	else if constexpr (CD == colordepth_1bpp) { draw_bmp_1bpp(zp, x << CD, zrow_offs, qp, qrow_offs, w, h, color); }
	else IERR();
}

/** draw character glyph into destination pixmap of any color depth.
	it draws the '1' bits in the given color, while '0' bits are left transparent.
	if you want to draw the '0' in a certain color too then clear the area with that color first.
	this is a variant of draw_bitmap() specialized for drawing character glyphs.
	it assumes these fixed properties:
		width     = 8 pixel
		qrow_offs = 1 byte (for 8 bit)

	@param zp		 start of first row in destination Pixmap
	@param zrow_offs address offset between rows in destination Pixmap
	@param x		 x position in pixels measured from zp
	@param qp		 start of character glyph (source bitmap)
	@param height	 height in pixels
	@param color	 color for drawing the character glyph
*/
template<ColorDepth CD>
void draw_char(uint8* zp, int zrow_offset, int x, const uint8* qp, int height, uint color) noexcept
{
	if constexpr (CD >= colordepth_16bpp)
	{
		draw_bmp_16bpp(zp + (x << (CD - 3)), zrow_offset, qp, 1 /*row_offs*/, 8 /*width*/, height, color);
	}
	else if constexpr (CD == colordepth_8bpp)
	{
		draw_bmp_8bpp(zp + (x << (CD - 3)), zrow_offset, qp, 1 /*row_offs*/, 8 /*width*/, height, color);
	}
	else if constexpr (CD == colordepth_4bpp) { draw_chr_4bpp(zp, x << CD, zrow_offset, qp, height, color); }
	else if constexpr (CD == colordepth_2bpp) { draw_chr_2bpp(zp, x << CD, zrow_offset, qp, height, color); }
	else if constexpr (CD == colordepth_1bpp) { draw_chr_1bpp(zp, x << CD, zrow_offset, qp, height, color); }
	else IERR();
}

/** Convert row from a Pixmap with ColorDepth CD to a row with 1bpp.
   this is a helper function for copy_rect_as_bitmap(..)
	@param zp		start of destination 1bpp row (byte-aligned)
	@param qp		start of source row (byte-aligned)
	@param w		width measured in pixel octets
	@param color	color to compare with, may be viewed as 'foreground color' or 'background color'
	@param toggle	mask with preset result for 'colors match': bits will be toggled for pixels which don't match color
*/
template<ColorDepth CD>
void copy_row_as_1bpp(uint8* zp, const uint8* qp, int w, uint color, uint8 toggle)
{
	if constexpr (CD == colordepth_16bpp)
	{
		const uint16* qptr = reinterpret_cast<const uint16*>(qp);
		while (--w >= 0)
		{
			uint8 byte = toggle;
			for (int i = 0; i < 8; i++) { byte ^= (qptr[i] != uint16(color)) << i; }
			*zp++ = byte;
			qptr += 8;
		}
	}
	else if constexpr (CD == colordepth_8bpp)
	{
		while (--w >= 0)
		{
			uint8 byte = toggle;
			for (int i = 0; i < 8; i++) { byte ^= (qp[i] != uint8(color)) << i; }
			*zp++ = byte;
			qp += 8;
		}
	}
	else if constexpr (CD == colordepth_4bpp)
	{
		if ((int(qp) & 3) == 0)
		{
			const uint32* qptr = reinterpret_cast<const uint32*>(qp);
			while (--w >= 0) { *zp++ = bitblit::reduce_bits_4bpp(*qptr++ ^ color) ^ toggle; }
		}
		else
		{
			while (--w >= 0)
			{
				uint32 word;
				memcpy(&word, qp, 4);
				qp += 4;
				*zp++ = bitblit::reduce_bits_4bpp(word ^ color) ^ toggle;
			}
		}
	}
	else if constexpr (CD == colordepth_2bpp)
	{
		if ((int(qp) & 1) == 0)
		{
			const uint16* qptr = reinterpret_cast<const uint16*>(qp);
			while (--w >= 0) { *zp++ = bitblit::reduce_bits_2bpp(*qptr++ ^ color) ^ toggle; }
		}
		else
		{
			while (--w >= 0)
			{
				uint16 word = *qp++;
				word += *qp++ << 8;
				*zp++ = bitblit::reduce_bits_2bpp(word ^ color) ^ toggle;
			}
		}
	}
	else if constexpr (CD == colordepth_1bpp)
	{
		toggle ^= color;
		while (--w >= 0) { *zp++ = *qp++ ^ toggle; }
	}
}

/**	Convert rectangular area of a Pixmap to a 1bpp Bitmap.
	@param zp		 address of destination Bitmap
	@param zrow_offs row address offset in zp[]
	@param qp		 address of source Pixmap. source must be byte-aligned. (TODO?)
	@param qrow_offs row address offset in qp[]
	@param w		 width of area in pixels
	@param h		 height of area in pixels
	@param color	 color to compare with, may be viewed as 'foreground color' or 'background color'
	@param set		 true: set bit in bmp if color matches foreground color; false: clear bit if color matches background color
*/
template<ColorDepth CD>
void copy_rect_as_bitmap(uint8* zp, int zrow_offs, uint8* qp, int qrow_offs, int w, int h, uint color, bool set)
{
	if constexpr (CD <= colordepth_4bpp) color = flood_filled_color<CD>(color);
	uint8 toggle = -set; // is_set ? 0xff : 0x00

	int r = w & 7;			 // odd bits
	if (r) r = (1 << r) - 1; // make bitmask from odd bits
	w = (w + 7) >> 3;		 // width of bitmap in bytes

	while (--h >= 0)
	{
		copy_row_as_1bpp<CD>(zp, qp, w, color, toggle);
		if (r) zp[w - 1] &= r; // mask off the surplus bits
		zp += zrow_offs;
		qp += qrow_offs;
	}
}

/**	compare two rows of pixels in ColorDepth CD.
	Both rows must start on a byte boundary (TODO?) but the width may be odd.
	@tparam CD		ColorDepth of the Pixmaps
	@param zp		start of first row
	@param qp		start of other row
	@param width	width of rows in pixels
	@return			zero if equal, else result of mismatch comparison ((same as memcmp))
*/
template<ColorDepth CD>
int compare_row(const uint8* zp, const uint8* qp, int width) noexcept
{
	if constexpr (CD == colordepth_16bpp) { return memcmp(zp, qp, uint(width) << 1); }
	else if constexpr (CD == colordepth_8bpp) { return memcmp(zp, qp, uint(width)); }
	else
	{
		constexpr uint ss = 3 - CD;

		int r = memcmp(zp, qp, width >> ss);
		if (r != 0 || (width & bitmask(ss)) == 0) return r;

		uint mm = bitmask((width & bitmask(ss)) << CD);
		return (zp[width >> ss] & mm) - (qp[width >> ss] & mm);
	}
}


} // namespace kio::Graphics::bitblit
