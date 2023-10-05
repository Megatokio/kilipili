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

/** Helper: calculate mask for n bits.
*/
inline constexpr uint32 bitmask(uint n) noexcept { return (1 << n) - 1; }

/** Helper: calculate mask for one pixel.
*/
template<ColorDepth CD>
constexpr uint32 pixelmask = bitmask(1 << CD);

/** Helper: spread color across whole uint32.
*/
template<ColorDepth CD>
constexpr uint32 flood_filled_color(uint color) noexcept;

} // namespace kio::Graphics


namespace kio::Graphics::bitblit
{

/** Read single pixel from Pixmap.
	@tparam CD		ColorDepth of the Pixmap
	@param row		start of row with the pixel
	@param x		x offset from `row` in pixels
	@return			the pixel value
*/
template<ColorDepth CD>
uint get_pixel(const uint8* row, int x) noexcept;


/** Set single pixel in Pixmap.
	@tparam CD		ColorDepth of the Pixmap
	@param row		start of row for the pixel
	@param x		x offset from `row` in pixels
	@param color	new color for the pixel
*/
template<ColorDepth CD>
void set_pixel(uint8* row, int x, uint color) noexcept;


/** Draw vertical line in Pixmap.
	@tparam CD		ColorDepth of the Pixmap
	@param row		start of top row
	@param roffs	address offset between rows
	@param x		xpos measured from `row` in pixels
	@param height	height of vertical line, measured in pixels
	@param color	color to draw
*/
template<ColorDepth CD>
void draw_vline(uint8* row, int roffs, int x, int height, uint color) noexcept;

template<ColorDepth CD>
void draw_vline_ref(uint8* row, int roffs, int x, int height, uint color) noexcept;


/** Draw horizontal line in Pixmap.
	@tparam CD		ColorDepth of the Pixmap
	@param row		start of row
	@param x    	xpos measured from `row` in pixels
	@param width	width of horizontal line, measured in pixels
	@param color	color for drawing
*/
template<ColorDepth CD>
void draw_hline(uint8* row, int x, int width, uint color) noexcept;

template<ColorDepth CD>
void draw_hline_ref(uint8* row, int x, int width, uint color) noexcept;


/** Clear rectangular area with color.
	@tparam CD		ColorDepth of the Pixmap
	@param row		start of top row
	@param roffs	address offset between rows
	@param x1		xpos of rect
	@param width	width in pixels
	@param height	height in pixels
	@param color	color for painting
*/
template<ColorDepth CD>
void clear_rect(uint8* row, int row_offset, int x1, int width, int height, uint color) noexcept;


/** Toggle color in rectangular area with xor_color.
	Main application is to draw the cursor blob in a text editor.
	@tparam CD		ColorDepth of the Pixmap
	@param row		start of top row
	@param roffs	address offset between rows
	@param x1		xpos of rect
	@param width	width in pixels
	@param height	height in pixels
	@param xor_color  xor_pattern for toggling the colors of all pixels
*/
template<ColorDepth CD>
void xor_rect(uint8* row, int row_offset, int x1, int width, int height, uint xor_color) noexcept;


/**	Clear every 2nd, 4th or 16th column in rectangle depending on AttrMode AM.
	This is intended to set the colors in the color attributes for a rectangle in Pixmaps with attributes.
	
	Note that x1 and width are the coordinates in the attributes[],
		not the x coordinate and width used to draw the line in the pixels[] of the Pixmap.
	Therefore:
		x1 = x/attrwidth * num_colors_in_attr + pixel_value
		   = (x >> AW) << (1 << AM) + pixel
		   
	@tparam	AM		AttrMode, essentially the color depth of the pixmap itself
	@tparam CD		ColorDepth of the colors in the attributes
	@param row		start of top row
	@param row_offs	address offset between rows
	@param x1    	xpos measured from `row` in pixels (colors)
	@param w		width of rectangle, measured in pixels (colors) in the attributes
	@param h		width of rectangle, measured in pixels (rows) in the attributes
	@param color	color for drawing
*/
template<AttrMode AM, ColorDepth CD>
void attr_clear_rect(uint8* row, int row_offset, int x1, int width, int height, uint color) noexcept;


/** Copy rectangular area inside a pixmap or from one pixmap to another.
	Handles overlap properly.

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
void copy_rect(uint8* zp, int zrow_offs, int zx, const uint8* qp, int qrow_offs, int qx, int w, int h) noexcept;


/** Draw Bitmap into destination Pixmap of any color depth.
	Draws the '1' bits in the given color, while '0' bits are left transparent.
	if you want to draw the '0' in a certain color too then clear the area with that color first.

	@param zp		 start of the first row in destination Pixmap
	@param zrow_offs row offset in destination Pixmap measured in bytes
	@param x0		 x offset from zp in pixels
	@param qp		 start of the first byte of source Bitmap
	@param qrow_offs row offset in source Bitmap measured in bytes
	@param w		 width in pixels
	@param h		 height in pixels
	@param color	 color for drawing the bitmap
*/
template<ColorDepth CD>
void draw_bitmap(uint8* zp, int zrow_offs, int x0, const uint8* qp, int qrow_offs, int w, int h, uint color) noexcept;

template<ColorDepth CD>
void draw_bitmap_ref(
	uint8* zp, int zrow_offs, int x0, const uint8* qp, int qrow_offs, int w, int h, uint color) noexcept;


/** Draw character glyph into destination pixmap of any color depth.
	It draws the '1' bits in the given color, while '0' bits are left transparent.
	If you want to draw the '0' in a certain color too then clear the area with that color first.
	This is a variant of draw_bitmap() specialized for drawing character glyphs.
	It assumes these fixed properties:
		width     = 8 pixel
		qrow_offs = 1 byte (for 8 bit)

	@param zp		 start of first row in destination Pixmap
	@param zrow_offs address offset between rows in destination Pixmap
	@param x0		 x position in pixels measured from zp
	@param qp		 start of character glyph (source bitmap)
	@param height	 height in pixels
	@param color	 color for drawing the character glyph
*/
template<ColorDepth CD>
void draw_char(uint8* zp, int zrow_offset, int x0, const uint8* qp, int height, uint color) noexcept;


/** Convert row from a Pixmap with ColorDepth CD to a row with 1bpp.
	This is a helper function for copy_rect_as_bitmap(..)
	@param zp		start of destination 1bpp row (byte-aligned)
	@param qp		start of source row (byte-aligned)
	@param w		width measured in pixel octets
	@param color	color to compare with, may be viewed as 'foreground color' or 'background color'
	@param toggle	mask with preset result for 'colors match': bits will be toggled for pixels which don't match color
*/
template<ColorDepth CD>
void copy_row_as_1bpp(uint8* zp, const uint8* qp, int w, uint color, uint8 toggle);


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
void copy_rect_as_bitmap(uint8* zp, int zrow_offs, uint8* qp, int qrow_offs, int w, int h, uint color, bool set);


/**	Compare two rows of pixels in ColorDepth CD.
	Both rows must start on a byte boundary (TODO?) but the width may be odd.
	@tparam CD		ColorDepth of the Pixmaps
	@param zp		start of first row
	@param qp		start of other row
	@param width	width of rows in pixels
	@return			zero if equal, else result of mismatch comparison ((same as memcmp))
*/
template<ColorDepth CD>
int compare_row(const uint8* zp, const uint8* qp, int width) noexcept;


// ###############################################################################
// ###############################################################################


/** Clear row of bytes, halfwords or words with flood_filled_color.
	@param zp             pointer to the first byte, halfword or word
	@param num_bytes      width / number of bytes
	@param num_halfwords  width / number of halfwords (uint16)
	@param num_words      width / number of words (uint32)
	@param flood_filled_color  32 bit flood filled color
*/
void clear_row(uint8* zp, int num_bytes, uint32 flood_filled_color) noexcept;
void clear_row(uint16* zp, int num_halfwords, uint32 flood_filled_color) noexcept;
void clear_row(uint32* zp, int num_words, uint32 flood_filled_color) noexcept;


/** Clear row of bits with flood_filled_color.

	@param zp			pointer to the start of the row
	@param xoffs_bits	x offset from zp measured in bits
	@param num_bits		width / number of bits
	@param flood_filled_color  32 bit flood filled color
*/
void clear_row_of_bits(uint8* zp, int xoffs_bits, int num_bits, uint32 flood_filled_color) noexcept;


/** Toggle colors in a row of bits with flood_filled_xor_color.

	@param zp			pointer to the start of the row
	@param xoffs_bits	x offset from zp measured in bits
	@param num_bits		width / number of bits
	@param flood_filled_xor_color  32 bit flood filled xor pattern for color
*/
void xor_row_of_bits(uint8* zp, int xoffs_bits, int num_bits, uint32 flood_filled_xor_color) noexcept;


/** Clear row of bits with flood_filled_color, masked with flood_filled_mask.
    This is intended to set color attributes for a horizontal line
    to set only one color in each attribute.

	 @param zp			pointer to the start of the row
	 @param row_offset	row offset in bytes
	 @param xoffs_bits	x offset from zp measured in bits
	 @param width_bits	width in bits
	 @param height		height in rows
	 @param flood_filled_color  32 bit flood filled color
	 @param flood_filled_mask   32 bit wide mask for bits to set
*/
void clear_rect_of_bits_with_mask(
	uint8* zp, int row_offset, int xoffs_bits, int width_bits, int height, uint32 flood_filled_color,
	uint32 flood_filled_mask) noexcept;


/** Clear a rectangular area with bit boundary precision.
	The color is not rolled for (xoffs & 31) because it is assumed that xoffs is a multiple of the color width and 
	flood_filled_color is a repetition of a single color; then rolling would always result in the same value again.

	@param zp			pointer to the first row
	@param row_offset	row offset in bytes
	@param xoffs_bits	x position measured in bits
	@param width_bits	width in bits
	@param height		height in rows
	@param flood_filled_color   32 bit flood filled color
*/
void clear_rect_of_bits(
	uint8* zp, int row_offset, int xoffs_bits, int width_bits, int height, uint32 flood_filled_color) noexcept;


/** Toggle colors in a rectangular area with bit boundary precision.

	@param zp			pointer to first row
	@param row_offset	row offset in bytes
	@param xoffs_bits	x offset from zp measured in bits 
	@param width_bits	width in bits
	@param height		height in rows
	@param flood_filled_xor_color  32 bit flood filled xor pattern for color
*/
void xor_rect_of_bits(
	uint8* zp, int row_offset, int xoffs_bits, int width_bits, int height, uint32 flood_filled_xor_color) noexcept;


/** Copy rectangular area within one or between two pixmap with bit boundary precision.

	@param zp			pointer to the first row in destination
	@param zroffs		row offset in bytes
	@param zx0_bits		x offest from zp measured in bits
	@param qp			pointer to the first row in source
	@param qroffs		row offset in bytes
	@param qx0_bits		x offset from qp measured in bits
	@param w_bits		width in bits
	@param h			height in rows
*/
void copy_rect_of_bits(
	uint8* zp, int zroffs, int zx0_bits, const uint8* qp, int qroffs, int qx0_bits, int w_bits, int h) noexcept;


/** Copy rectangular area within one or between two pixmaps with byte boundary precision.

	@param zp			pointer to the first byte of destination (top left corner)
	@param zrow_offset  row offset in bytes
	@param qp			pointer to the first byte of source (top left corner)
	@param qrow_offset  row offset in bytes
	@param w			width in bytes
	@param h			height in rows
*/
void copy_rect_of_bytes(uint8* zp, int zrow_offset, const uint8* qp, int qrow_offset, int w, int h) noexcept;


} // namespace kio::Graphics::bitblit


//
//
// ######################################################################################################
// #################################### IMPLEMENTATIONS #################################################
// ######################################################################################################
//
//


namespace kio::Graphics
{
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

inline constexpr uint16 double_bits(uint8 bits) noexcept
{
	uint16 n = bits;
	n		 = (n | (n << 4)) & 0x0f0f;
	n		 = (n | (n << 2)) & 0x3333;
	n		 = (n | (n << 1)) & 0x5555;
	return n * 3;
}

inline constexpr uint32 quadruple_bits(uint8 bits) noexcept
{
	uint32 n = bits;
	n		 = (n | (n << 12)) & 0x000f000f;
	n		 = (n | (n << 6)) & 0x03030303;
	n		 = (n | (n << 3)) & 0x11111111;
	return n * 15;
}

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

template<ColorDepth CD>
void draw_vline_ref(uint8* row, int row_offset, int x, int height, uint color) noexcept
{
	while (--height >= 0) set_pixel<CD>(row + height * row_offset, x, color);
}

template<ColorDepth CD>
void draw_hline(uint8* row, int x, int width, uint color) noexcept
{
	color = flood_filled_color<CD>(color);
	if constexpr (CD <= colordepth_4bpp) { clear_row_of_bits(row, x << CD, width << CD, color); }
	else if constexpr (CD == colordepth_8bpp) { clear_row(row + x, width, color); }
	else if constexpr (CD == colordepth_16bpp) { clear_row(reinterpret_cast<uint16*>(row) + x, width, color); }
	else IERR();
}

template<ColorDepth CD>
void draw_hline_ref(uint8* row, int x, int width, uint color) noexcept
{
	while (--width >= 0) set_pixel<CD>(row, x + width, color);
}

template<ColorDepth CD>
void clear_rect(uint8* row, int roffs, int x1, int width, int height, uint color) noexcept
{
	clear_rect_of_bits(row, roffs, x1 << CD, width << CD, height, flood_filled_color<CD>(color));
}

template<ColorDepth CD>
void xor_rect(uint8* row, int row_offset, int x1, int width, int height, uint xor_color) noexcept
{
	xor_rect_of_bits(row, row_offset, x1 << CD, width << CD, height, flood_filled_color<CD>(xor_color));
}

template<AttrMode AM, ColorDepth CD>
void attr_clear_rect(uint8* row, int row_offset, int x1, int width, int height, uint color) noexcept
{
	constexpr int colors_per_attr = 1 << (1 << AM);

	if constexpr (CD == colordepth_16bpp)
	{
		uint16* p = reinterpret_cast<uint16*>(row) + x1;
		while (--height >= 0)
		{
			for (int i = 0; i < width; i += colors_per_attr) { p[i] = uint16(color); }
			p += row_offset >> 1;
		}
	}
	else if constexpr (CD == colordepth_8bpp)
	{
		uint8* p = row + x1;
		while (--height >= 0)
		{
			for (int i = 0; i < width; i += colors_per_attr) { p[i] = uint8(color); }
			p += row_offset;
		}
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

		clear_rect_of_bits_with_mask(
			row, row_offset, x1 << CD, width << CD, height, flood_filled_color<CD>(color), mask);
	}
}

template<ColorDepth CD>
void copy_rect(uint8* zp, int zrow_offs, int zx, const uint8* qp, int qrow_offs, int qx, int w, int h) noexcept
{
	if constexpr (CD >= colordepth_8bpp)
	{
		copy_rect_of_bytes(zp + (zx << (CD - 3)), zrow_offs, qp + (qx << (CD - 3)), qrow_offs, w << (CD - 3), h);
	}
	else { copy_rect_of_bits(zp, zrow_offs, zx << CD, qp, qrow_offs, qx << CD, w << CD, h); }
}

template<ColorDepth CD>
void draw_bitmap_ref(uint8* zp, int zroffs, int x0, const uint8* qp, int qroffs, int w, int h, uint color) noexcept
{
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
			if (get_pixel<colordepth_1bpp>(qp, x)) set_pixel<CD>(zp, x0 + x, color);
		qp += qroffs;
		zp += zroffs;
	}
}

// clang-format off
template<>void draw_bitmap<colordepth_1bpp>(uint8*, int, int, const uint8*, int, int, int, uint) noexcept;
template<>void draw_bitmap<colordepth_2bpp>(uint8*, int, int, const uint8*, int, int, int, uint) noexcept;
template<>void draw_bitmap<colordepth_4bpp>(uint8*, int, int, const uint8*, int, int, int, uint) noexcept;
template<>void draw_bitmap<colordepth_8bpp>(uint8*, int, int, const uint8*, int, int, int, uint) noexcept;
template<>void draw_bitmap<colordepth_16bpp>(uint8*, int, int, const uint8*, int, int, int, uint) noexcept;
template<>void draw_char<colordepth_1bpp>(uint8*, int, int, const uint8*, int, uint) noexcept;
template<>void draw_char<colordepth_2bpp>(uint8*, int, int, const uint8*, int, uint) noexcept;
template<>void draw_char<colordepth_4bpp>(uint8*, int, int, const uint8*, int, uint) noexcept;
// clang-format on

template<>
inline void
draw_char<colordepth_8bpp>(uint8* zp, int zrow_offset, int x0, const uint8* qp, int height, uint color) noexcept
{
	draw_bitmap<colordepth_8bpp>(zp, zrow_offset, x0, qp, 1 /*row_offs*/, 8 /*width*/, height, color);
}

template<>
inline void
draw_char<colordepth_16bpp>(uint8* zp, int zrow_offset, int x0, const uint8* qp, int height, uint color) noexcept
{
	draw_bitmap<colordepth_16bpp>(zp, zrow_offset, x0, qp, 1 /*row_offs*/, 8 /*width*/, height, color);
}

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
		if ((size_t(qp) & 3) == 0)
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
		if ((size_t(qp) & 1) == 0)
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


/*






































*/
