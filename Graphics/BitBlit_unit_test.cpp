// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BitBlit.h"
#include "basic_math.h"
#include "doctest.h"
#include <memory>

// clang-format off

using namespace kio::Graphics::bitblit;
using namespace kio::Graphics;
using namespace kio;


// #######################################################
// helpers:

static constexpr uint8 flip(uint8 n)
{
	n = ((n << 1) & 0xAA) + ((n >> 1) & 0x55);
	n = ((n << 2) & 0xCC) + ((n >> 2) & 0x33);
	n = ((n << 4) & 0xF0) + ((n >> 4) & 0x0F);
	return n;
}

static constexpr uint16 flip(uint16 n)
{
	n = ((n << 1) & 0xAAAA) + ((n >> 1) & 0x5555);
	n = ((n << 2) & 0xCCCC) + ((n >> 2) & 0x3333);
	n = ((n << 4) & 0xF0F0) + ((n >> 4) & 0x0F0F);
	n = ((n << 8) & 0xFF00) + ((n >> 8) & 0x00FF);
	return n;
}

static constexpr uint32 flip(uint32 n)
{
	n = ((n << 1) & 0xAAAAAAAA) + ((n >> 1) & 0x55555555);
	n = ((n << 2) & 0xCCCCCCCC) + ((n >> 2) & 0x33333333);
	n = ((n << 4) & 0xF0F0F0F0) + ((n >> 4) & 0x0F0F0F0F);
	n = ((n << 8) & 0xFF00FF00) + ((n >> 8) & 0x00FF00FF);
	n = (n << 16) + (n >> 16);
	return n;
}

static_assert(flip(uint8(0xAA))==0x55u);
static_assert(flip(uint16(0xAAAA))==0x5555u);
static_assert(flip(0xAAAAAAAAu)==0x55555555u);


#define n1(byte, len) byte + ((len) << 8)
#define n2(byte, len) n1(byte,len), n1((byte) >> 8, len
#define n4(byte, len) n2((byte) >> 16, len), n2(byte, len)

static void store(ptr& p, uint8 n)
{
	for (uint i = 0; i < 8; i++)
	{
		*p++ = '0' + (n & 1);
		n >>= 1;
	}
}


// #######################################################
// buffer for bitblit functions to work on:

template<uint width, uint height=1>
struct Buffer
{
	uchar px[width * height];

	Buffer(const uint16* compressed_source)
	{
		uchar* z = px;
		while (z < px + width * height)
		{
			uint16 v	= *compressed_source++;
			uchar  byte = v & 0xff;
			uchar  len	= v >> 8 ? v >> 8 : 1;
			byte = flip(byte);
			while (len--) *z++ = byte;
		}
		assert(z == px + width * height);
	}

	Buffer(const uint8* compressed_source)
	{
		for (uint i=0; i<width*height; i++)
		{
			px[i] = flip(compressed_source[i]);
		}
	}

	bool operator==(const Buffer& other) const { return memcmp(px, other.px, width * height) == 0; }
};

template<uint width, uint height>
doctest::String toString(const Buffer<width, height>& src)
{
	char		 bu[(width * 9 + 1) * height];
	ptr			 z = bu;
	const uchar* q = src.px;

	for (uint y = 0; y < height; y++)
	{
		for (uint x = 0; x < width; x++)
		{
			*z++ = x == 0 ? '\n' : ' ';
			store(z, *q++);
		}
	}
	*z++ = '\n';
	return doctest::String(bu, z - bu);
}


// #######################################################
// unit test cases:

TEST_CASE("BitBlit: flood_filled_color")
{
	//
}

TEST_CASE("BitBlit: double_bits, quadruple_bits")
{
	static_assert(double_bits(0x0f) == 0x00ff);
	static_assert(double_bits(0xA5) == 0xcc33);
	static_assert(quadruple_bits(0x0f) == 0x0000ffff);
	static_assert(quadruple_bits(0xA5) == 0xf0f00f0f);
}

TEST_CASE("BitBlit: reduce_bits_2bpp, reduce_bits_4bpp")
{
	static_assert(reduce_bits_4bpp(uint32(0xffff0000)) == 0xf0);
	static_assert(reduce_bits_4bpp(uint32(0x00110101)) == 0x35);
	static_assert(reduce_bits_4bpp(uint32(0x00000804)) == 0x05);
	static_assert(reduce_bits_2bpp(uint16(0xff00)) == 0xf0);
	static_assert(reduce_bits_2bpp(uint16(0xc8A5)) == 0xaf);
}

TEST_CASE("BitBlit: clear_rect_of_bits")
{
	// clear_rect_of_bits(uint8* zp, int row_offset, int xoffs, int width, int height, uint32 color)
	//	zp = pointer to the first row
	//	xoffs      = x position measured in bits
	//	row_offset = row offset in bytes
	//	width      = width in bits
	//	height     = height in rows
	//	color      = flood filled color

	// aligned to int32:
	{ 
		constexpr int width = 16, height = 4;
		const uint16 q[] = {0x40ff}; // 0x40 * '0xff' 
		const uint16 z[] = {
			0x10ff, 
			0x04ff, 0x08AC, 0x04ff, 
			0x04ff, 0x08AC, 0x04ff, 
			0x10ff, 
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		clear_rect_of_bits(pixmap.px, width, 8*width + 32, 64, 2, flip(0xACACACACu));
		CHECK_EQ(pixmap, expect);
	}

	// aligned to int32, width = N*4+i:
	{ 
		constexpr int width = 17, height = 9;
		constexpr int x0=32,w=64,h=7;
		const uint16 q[] = {0x99ff}; // 0x44 * '0xff' 
		const uint16 z[] = {
			0x11ff, 
			0x04ff, 0x08AC, 0x05ff, 
			0x04ff, 0x08AC, 0x05ff, 
			0x04ff, 0x08AC, 0x05ff, 
			0x04ff, 0x08AC, 0x05ff, 
			0x04ff, 0x08AC, 0x05ff, 
			0x04ff, 0x08AC, 0x05ff, 
			0x04ff, 0x08AC, 0x05ff, 
			0x11ff, 
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		clear_rect_of_bits(pixmap.px, width, 8*width + x0, w, h, flip(0xACACACACu));
		CHECK_EQ(pixmap, expect);
	}

	// x0 not aligned, w = N*4, width=N*4
	{
		constexpr int width = 16, height = 4;
		constexpr int x0=11,w=64,h=2;
		const uint16 q[] = {0x40ff}; // 0x40 * '0xff' 
		const uint16 z[] = {
			0x10ff, 
			0xff,0b11110101,0x0755,0b01011111, 0x06ff, 
			0xff,0b11110101,0x0755,0b01011111, 0x06ff, 
			0x10ff, 
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		clear_rect_of_bits(pixmap.px, width, 8*width + x0, w, h, flip(0x55555555u));
		CHECK_EQ(pixmap, expect);
	}

	// x0 not aligned, w = N*4+i
	{
		constexpr int width = 16, height = 3;
		constexpr int x0=20,w=30,h=1;
		const uint16 q[] = {0x30ff};  
		const uint16 z[] = {
			0x10ff, 
			0x02ff, 0xf6, 0x0366, 0b01111111, 0x09ff, 
			0x10ff, 
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		clear_rect_of_bits(pixmap.px, width, 8*width + x0, w, h, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
	}
	
	// x0 not aligned, small w = N*4+i
	{
		constexpr int width = 16, height = 3;
		constexpr int x0=36,w=20,h=1;
		const uint16 q[] = {0x30ff};  
		const uint16 z[] = {
			0x10ff, 
			0x04ff, 0xf6, 0x0266, 0x09ff, 
			0x10ff, 
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		clear_rect_of_bits(pixmap.px, width, 8*width + x0, w, h, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
	}

	// w = 0
	{
		constexpr int width = 16, height = 3;
		constexpr int x0 = 36;
		const uint16 q[] = {0x30ff};  
		const uint16 z[] = {0x30ff};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		clear_rect_of_bits(pixmap.px, width, 8*width + x0, 0, 2, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, width, 8*width + x0, 33, 0, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, width, 8*width + x0, 33, -1, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, width, 8*width + x0, -1, 2, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, width, 8*width + x0, 33, -999, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, width, 8*width + x0, -999, 2, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
	}
}

TEST_CASE("BitBlit: clear_rect_of_bits (was: clear_rect_of_bytes)")
{
	// 8bpp:	
	{
		constexpr int width = 17, height = 10;
		const uint16 q[] = { 0xAA00 };  
		
		Buffer<width, height> pixmap(q);
		Buffer<width, height> expect(q);

		for(int bg=0; bg<=255; bg+=255)
		{
			uint fg = 0x55;
			for(int x=0;x<=4;x++)
			{
				memset(pixmap.px, bg, width*height);
				memset(expect.px, bg, width*height);
				
				int y=1,w=10,h=5;
				clear_rect_of_bits(pixmap.px + y*width + x, width, 0, w<<3, h, fg*0x01010101);
				for(int i=0; i<w; i++) 
					draw_vline<colordepth_8bpp>(expect.px + y*width, width, x+i, h, fg);
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}

	// 16bpp:	
	{
		constexpr int width = 18, height = 10;
		const uint16 q[] = { 0xB400 };  
		
		Buffer<width, height> pixmap(q);
		Buffer<width, height> expect(q);

		for(int bg=0; bg<=255; bg+=255)
		{
			uint fg = 0x5678;
			for(int x=0;x<=4;x+=2)
			{
				memset(pixmap.px, bg, width*height);
				memset(expect.px, bg, width*height);
				
				int y=1,w=12,h=5;
				clear_rect_of_bits(pixmap.px + y*width + x, width, 0, w<<3, h, fg*0x00010001);
				for(int i=0; i<w; i+=2) 
					draw_vline<colordepth_16bpp>(expect.px + y*width, width, (x+i)/2, h, fg);
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}

	// 32bpp:	
	{
		constexpr int width = 20, height = 10;
		const uint16 q[] = { 0xC800 };  
		
		Buffer<width, height> pixmap(q);
		Buffer<width, height> expect(q);

		for(int bg=0; bg<=255; bg+=255)
		{
			uint fg = 0x23456789;
			for(int x=0;x<=4;x+=4)
			{
				memset(pixmap.px, bg, width*height);
				memset(expect.px, bg, width*height);
				
				int y=1,w=12,h=5;
				clear_rect_of_bits(pixmap.px + y*width + x, width, 0, w<<3, h, fg*0x00000001);
				for(int i=0; i<w; i+=4) 
				{
					draw_vline<colordepth_16bpp>(expect.px + y*width, width, (x+i)/2+0, h, uint16(fg)*0x00010001);
					draw_vline<colordepth_16bpp>(expect.px + y*width, width, (x+i)/2+1, h, (fg>>16)*0x00010001);
				}
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}
}

TEST_CASE("BitBlit: clear_rect_of_bits_with_mask")
{
	// clear rect of bits with color, masked with bitmask
	// 
	// row = pointer to the start of the row
	// x0_bits     = x position measured in bits
	// row_offset = row offset in bytes
	// width_bits = width in bits
	// height = height in rows
	// flood_filled_color = 32 bit flood filled color
	// flood_filled_mask  = 32 bit wide mask for bits to set
	//
	// clear_row_of_bits_with_mask(row, x0_bits, width_bits, flood_filled_color, flood_filled_mask);

	// x0 aligned, width aligned
	{
		constexpr int width = 0x10;
		constexpr int x0=32,w=64,h=1;
		const uint16 q[] = { 0x10aa };  
		const uint16 z[] = { 0x04aa, 0x085a, 0x04aa, };

		Buffer<width> pixmap(q);
		Buffer<width> expect(z);

		clear_rect_of_bits_with_mask(pixmap.px, 0, x0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
		CHECK_EQ(pixmap, expect);
	}
		
	// x0 aligned, width not aligned
	{
		constexpr int width = 0x10;
		constexpr int x0=32,w=26,h=1;
		const uint16 q[] = {0x10aa};  
		const uint16 z[] = {
			0x04aa, 
			0x035a, 0x6a, 
			0x08aa, 
			};

		Buffer<width> pixmap(q);
		Buffer<width> expect(z);

		clear_rect_of_bits_with_mask(pixmap.px, 0, x0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
		CHECK_EQ(pixmap, expect);
	}
		
	// x0 not aligned, width not aligned
	{
		constexpr int width = 0x10;
		constexpr int x0=20,w=12+32+4,h=1;
		const uint16 q[] = { 0x10aa };
		const uint16 z[] = { 0x02aa, 0xa5, 0xa5, 0x04a5, 0xaa, 0x07aa };

		Buffer<width> pixmap(q);
		Buffer<width> expect(z);

		clear_rect_of_bits_with_mask(pixmap.px, 0, x0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
		CHECK_EQ(pixmap, expect);
	}
		
	// zp not aligned
	{
		constexpr int width = 0x10;
		constexpr int x0=20,w=12+32+8,h=1;
		const uint16 q[] = { 0x10aa };
		const uint16 z[] = { 0x02aa, 0xa5, 0xa5, 0x04a5, 0xa5, 0x07aa };

		Buffer<width> pixmap(q);
		Buffer<width> expect(z);

		clear_rect_of_bits_with_mask(pixmap.px+1, 0, x0-8, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
		CHECK_EQ(pixmap, expect);
	}
		
	// short width accross int32
	{
		constexpr int width = 0x10;
		constexpr int x0=20,w=12+12,h=1;
		const uint16 q[] = { 0x10aa };
		const uint16 z[] = { 0x02aa, 0xa5, 0xa5, 0xa5, 0x0Baa };

		Buffer<width> pixmap(q);
		Buffer<width> expect(z);

		clear_rect_of_bits_with_mask(pixmap.px, 0, x0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
		CHECK_EQ(pixmap, expect);
	}
		
	// short width inside int32
	{
		constexpr int width = 0x10;
		constexpr int x0=12,w=12,h=1;
		const uint16 q[] = { 0x10aa };
		const uint16 z[] = { 0xaa, 0xa5, 0xa5, 0xaa, 0x0Caa };

		Buffer<width> pixmap(q);
		Buffer<width> expect(z);

		clear_rect_of_bits_with_mask(pixmap.px, 0, x0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
		CHECK_EQ(pixmap, expect);
	}
		
	// width=0
	{
		constexpr int width = 0x10,height=3;
		constexpr int x0=12;
		const uint16 q[] = { 0x3000 };
		const uint16 z[] = { 0x3000 };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		clear_rect_of_bits_with_mask(pixmap.px, width, 8*width + x0, 0, 2, 0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, width, 8*width + x0, 33, 0,  0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, width, 8*width + x0, 33, -1, 0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, width, 8*width + x0, -1, 2,  0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);		
		clear_rect_of_bits_with_mask(pixmap.px, width, 8*width + x0, 33, -999, 0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, width, 8*width + x0, -999, 2,  0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);		
	}
}

TEST_CASE("BitBlit: attr_clear_rect")
{
	// draw every 2nd, 4th or 16th pixel depending on AttrMode AM.
	// note that x1 and width are the coordinates in the attributes[]
	// 
	// @tparam	AM		AttrMode, essentially the color depth of the pixmap itself
	// @tparam CD		ColorDepth of the colors in the attributes
	// @param row		start of row
	// @param row_offs	address offset between rows
	// @param x1    	xpos measured from `row` in pixels (colors)
	// @param w			width of rectangle, measured in pixels (colors) in the attributes
	// @param width	width of horizontal line, measured in pixels (colors)
	// @param color	color for drawing
	
	// AM=1bpp, CM=rgb
	{
		constexpr int width = 0x10 * 2,height=5;
		constexpr int x0=6,w=5,h=3;
		const uint16 q[] = {0xA033};  
		const uint16 z[] = {
			0x2033,
			0x0C33, 0xAB,0xCD, 0x33,0x33, 0xAB,0xCD, 0x33,0x33, 0xAB,0xCD,  0x0A33, 
			0x0C33, 0xAB,0xCD, 0x33,0x33, 0xAB,0xCD, 0x33,0x33, 0xAB,0xCD,  0x0A33, 
			0x0C33, 0xAB,0xCD, 0x33,0x33, 0xAB,0xCD, 0x33,0x33, 0xAB,0xCD,  0x0A33, 
			0x2033,
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		attr_clear_rect<attrmode_1bpp,kio::Graphics::colordepth_16bpp>
				(pixmap.px+width, width, x0, w, h, flip(uint16(0xABCD)));
		CHECK_EQ(pixmap, expect);
	}
	
	// AM=2bpp, CM=rgb
	{
		constexpr int width = 0x14 * 2,height=3;
		constexpr int x0=6,w=9,h=1;
		const uint16 q[] = {0x2833, 0x2833, 0x2833};  
		const uint16 z[] = {
			0x2833,			
			0x0C33, 0xAB,0xCD, 0x0633, 0xAB,0xCD, 0x0633, 0xAB,0xCD, 0x0A33, 
			0x2833,
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		attr_clear_rect<attrmode_2bpp,kio::Graphics::colordepth_16bpp>
				(pixmap.px+width, width, x0, w, h, flip(uint16(0xABCD)));
		CHECK_EQ(pixmap, expect);
	}
	
	// AM=1bpp, CM=i8
	{
		constexpr int width = 0x10,height=5;
		constexpr int x0=6,w=5,h=3;
		const uint16 q[] = {0x5033};  
		const uint16 z[] = {
			0x1033,
			0x0633, 0xAB, 0x33, 0xAB, 0x33, 0xAB, 0x0533, 
			0x0633, 0xAB, 0x33, 0xAB, 0x33, 0xAB, 0x0533, 
			0x0633, 0xAB, 0x33, 0xAB, 0x33, 0xAB, 0x0533, 
			0x1033,
			};

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		attr_clear_rect<attrmode_1bpp,kio::Graphics::colordepth_8bpp>
				(pixmap.px+width, width, x0, w, h, flip(uint8(0xAB)));
		CHECK_EQ(pixmap, expect);
	}
	
	// AM=2bpp, CM=i8
	{
		constexpr int width = 0x14;
		constexpr int x0=6,w=9,h=1;
		const uint16 q[] = {0x1433};  
		const uint16 z[] = {
			0x0633, 
			0xAB, 0x0333, 0xAB, 0x0333, 0xAB, 
			0x0533, 
			};

		Buffer<width> pixmap(q);
		Buffer<width> expect(z);

		attr_clear_rect<attrmode_2bpp,kio::Graphics::colordepth_8bpp>
				(pixmap.px, 0, x0, w, h, flip(uint8(0xAB)));
		CHECK_EQ(pixmap, expect);
	}
	
	// AM=1bpp, CM=i4, aligned
	{
		constexpr int width = 0x10,height=9;
		constexpr int x0=8,w=15,h=7;
		const uint16 q[] = {0x90aa};  
		const uint16 z[] = {
			0x10aa,
			0x04aa, 0x08Ea, 0x04aa,
			0x04aa, 0x08Ea, 0x04aa,
			0x04aa, 0x08Ea, 0x04aa,
			0x04aa, 0x08Ea, 0x04aa,
			0x04aa, 0x08Ea, 0x04aa,
			0x04aa, 0x08Ea, 0x04aa,
			0x04aa, 0x08Ea, 0x04aa,
			0x10aa };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		attr_clear_rect<attrmode_1bpp,kio::Graphics::colordepth_4bpp>
				(pixmap.px+width, width, x0, w, h, flip(uint8(0xE0)));
		CHECK_EQ(pixmap, expect);
	}
	
	// AM=1bpp, CM=i4, unaligned, odd row offset
	{
		constexpr int width = 0x11,height=9;
		constexpr int x0=9,w=14,h=7;
		const uint16 q[] = {0x99aa};  
		const uint16 z[] = {
			0x11aa,
			0x04aa, 0xaE, 0x06aE, 0xaa, 0x05aa,
			0x04aa, 0xaE, 0x06aE, 0xaa, 0x05aa,
			0x04aa, 0xaE, 0x06aE, 0xaa, 0x05aa,
			0x04aa, 0xaE, 0x06aE, 0xaa, 0x05aa,
			0x04aa, 0xaE, 0x06aE, 0xaa, 0x05aa,
			0x04aa, 0xaE, 0x06aE, 0xaa, 0x05aa,
			0x04aa, 0xaE, 0x06aE, 0xaa, 0x05aa,
			0x11aa };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		attr_clear_rect<attrmode_1bpp,kio::Graphics::colordepth_4bpp>
				(pixmap.px+width, width, x0, w, h, flip(uint8(0xE0)));
		CHECK_EQ(pixmap, expect);
	}
	
	// AM=2bpp, CM=i4, unaligned, odd row offset
	{
		constexpr int width = 0x11,height=3;
		constexpr int x0=9,w=14,h=1;
		const uint16 q[] = {0x33aa};  
		const uint16 z[] = {
			0x11aa,
			0x04aa, 0xaE, 0xaa, 0xaE, 0xaa, 0xaE, 0xaa, 0xaE, 0xaa, 0x05aa,
			0x11aa };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		attr_clear_rect<attrmode_2bpp,kio::Graphics::colordepth_4bpp>
				(pixmap.px+width, width, x0, w, h, flip(uint8(0xE0)));
		CHECK_EQ(pixmap, expect);
	}
}

TEST_CASE("BitBlit: set_pixel, get_pixel")
{
	SUBCASE("1bpp")
	{
		constexpr int width = 7,height=10;
		const uint16 q[] = {0x4600};  
		const uint16 z[] = {
			0x0700,
			0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 
			0x0700 };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);
		
		for(int i=0;i<8;i++) 
			set_pixel<kio::Graphics::colordepth_1bpp>(pixmap.px+width+i*width,24+i*3,1);

		CHECK_EQ(pixmap, expect);
		
		for(int i=0;i<8;i++) 
		{
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_1bpp>(pixmap.px+width+i*width,24+i*3),1);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_1bpp>(pixmap.px+width+i*width,24+i*3-1),0);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_1bpp>(pixmap.px+width+i*width,24+i*3+1),0);
		}		
	}
	
	SUBCASE("2bpp")
	{
		constexpr int width = 11,height=10;
		const uint16 q[] = {0x6E00};  
		const uint16 z[] = {
			0x0B00,
			0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x00, 
			0x0B00 };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);
		
		for(int i=0;i<8;i++) 
			set_pixel<kio::Graphics::colordepth_2bpp>(pixmap.px+width+i*width,12+i*3,3);

		CHECK_EQ(pixmap, expect);
		
		for(int i=0;i<8;i++) 
		{
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_2bpp>(pixmap.px+width+i*width,12+i*3),3);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_2bpp>(pixmap.px+width+i*width,12+i*3-1),0);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_2bpp>(pixmap.px+width+i*width,12+i*3+1),0);
		}		
	}
	
	SUBCASE("4bpp")
	{
		constexpr int width = 13,height=10;
		const uint16 q[] = {0x8200};  
		const uint16 z[] = {
			0x0D00,
			0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x00, 
			0x0D00 };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);
		
		for(int i=0;i<8;i++) 
			set_pixel<kio::Graphics::colordepth_4bpp>(pixmap.px+width+i*width,2+i*3,15);

		CHECK_EQ(pixmap, expect);
		
		for(int i=0;i<8;i++) 
		{
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_4bpp>(pixmap.px+width+i*width,2+i*3),15);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_4bpp>(pixmap.px+width+i*width,2+i*3-1),0);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_4bpp>(pixmap.px+width+i*width,2+i*3+1),0);
		}		
	}

	SUBCASE("8bpp")
	{
		constexpr int width = 11,height=10;
		const uint16 q[] = {0x6E00};  
		const uint16 z[] = {
			0x0B00,
			0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 
			0x0B00 };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);
		
		for(int i=0;i<8;i++) 
			set_pixel<kio::Graphics::colordepth_8bpp>(pixmap.px+width+i*width,1+i,255);

		CHECK_EQ(pixmap, expect);
		
		for(int i=0;i<8;i++) 
		{
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_8bpp>(pixmap.px+width+i*width,1+i),255);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_8bpp>(pixmap.px+width+i*width,1+i-1),0);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_8bpp>(pixmap.px+width+i*width,1+i+1),0);
		}		
	}

	SUBCASE("16bpp")
	{
		constexpr int width = 18,height=10;
		const uint16 q[] = {0xB400};  
		const uint16 z[] = {
			0x1200,
			0x0200, 0x02F9, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,  
			0x0200, 0x0200, 0x02F9, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,  
			0x0200, 0x0200, 0x0200, 0x02F9, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,  
			0x0200, 0x0200, 0x0200, 0x0200, 0x02F9, 0x0200, 0x0200, 0x0200, 0x0200,  
			0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x02F9, 0x0200, 0x0200, 0x0200,  
			0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x02F9, 0x0200, 0x0200,  
			0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x02F9, 0x0200,  
			0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x02F9,  
			0x1200 };

		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);
		
		for(int i=0;i<8;i++) 
			set_pixel<kio::Graphics::colordepth_16bpp>(pixmap.px+width+i*width, 1+i, flip(uint16(0xF9F9u)));

		CHECK_EQ(pixmap, expect);
		
		for(int i=0;i<8;i++) 
		{
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_16bpp>(pixmap.px+width+i*width, 1+i), flip(uint16(0xF9F9u)));
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_16bpp>(pixmap.px+width+i*width, 1+i-1), 0);
			CHECK_EQ(get_pixel<kio::Graphics::colordepth_16bpp>(pixmap.px+width+i*width, 1+i+1), 0);
		}		
	}
}

TEST_CASE("BitBlit: compare_row")
{
	// colordepth rgb16
	{
		constexpr int width = 16;
		const uint16 p1[] = {1,2,3,4,5,6,7,8,11,12,13,14,15,16,17,18}; 

		Buffer<width> pixmap1(p1);
		Buffer<width> pixmap2(p1);

		CHECK_EQ(compare_row<kio::Graphics::colordepth_16bpp>(pixmap1.px, pixmap2.px, 8), 0);
		pixmap2.px[15]++;
		CHECK_NE(compare_row<kio::Graphics::colordepth_16bpp>(pixmap1.px, pixmap2.px, 8), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_16bpp>(pixmap1.px, pixmap2.px, 7), 0);		
	}
	
	// colordepth i8
	{
		constexpr int width = 16;
		const uint16 p1[] = {1,2,3,4,5,6,7,8,11,12,13,14,15,16,17,18}; 

		Buffer<width> pixmap1(p1);
		Buffer<width> pixmap2(p1);

		CHECK_EQ(compare_row<kio::Graphics::colordepth_8bpp>(pixmap1.px, pixmap2.px, 16), 0);
		pixmap2.px[15]++;
		CHECK_NE(compare_row<kio::Graphics::colordepth_8bpp>(pixmap1.px, pixmap2.px, 16), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_8bpp>(pixmap1.px, pixmap2.px, 15), 0);		
	}
	
	// colordepth i4
	{
		constexpr int width = 8;
		const uint16 p1[] = {0x11,0x02,0x43,0x24,0x65,0x16,0x67,0x98}; 

		Buffer<width> pixmap1(p1);
		Buffer<width> pixmap2(p1);

		CHECK_EQ(compare_row<kio::Graphics::colordepth_4bpp>(pixmap1.px, pixmap2.px, 16), 0);
		pixmap2.px[7] += 1;
		CHECK_NE(compare_row<kio::Graphics::colordepth_4bpp>(pixmap1.px, pixmap2.px, 16), 0);
		CHECK_NE(compare_row<kio::Graphics::colordepth_4bpp>(pixmap1.px, pixmap2.px, 15), 0);		
		CHECK_EQ(compare_row<kio::Graphics::colordepth_4bpp>(pixmap1.px, pixmap2.px, 14), 0);		
		pixmap2.px[7] += 16-1;
		CHECK_NE(compare_row<kio::Graphics::colordepth_4bpp>(pixmap1.px, pixmap2.px, 16), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_4bpp>(pixmap1.px, pixmap2.px, 15), 0);		
	}
	
	// colordepth i2
	{
		constexpr int width = 8;
		const uint16 p1[] = {0x11,0x02,0x43,0x24,0x65,0x16,0x67,0x00}; 

		Buffer<width> pixmap1(p1);
		Buffer<width> pixmap2(p1);

		CHECK_EQ(compare_row<kio::Graphics::colordepth_2bpp>(pixmap1.px, pixmap2.px, 32), 0);
		pixmap2.px[7] += 0x40;
		CHECK_NE(compare_row<kio::Graphics::colordepth_2bpp>(pixmap1.px, pixmap2.px, 32), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_2bpp>(pixmap1.px, pixmap2.px, 31), 0);		
		pixmap2.px[7] -= 0x40;
		pixmap2.px[7] += 0x20;
		CHECK_NE(compare_row<kio::Graphics::colordepth_2bpp>(pixmap1.px, pixmap2.px, 31), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_2bpp>(pixmap1.px, pixmap2.px, 30), 0);		
		pixmap2.px[7] -= 0x20;
		pixmap2.px[7] += 0x01;
		CHECK_NE(compare_row<kio::Graphics::colordepth_2bpp>(pixmap1.px, pixmap2.px, 29), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_2bpp>(pixmap1.px, pixmap2.px, 28), 0);		
	}
	
	// colordepth i1
	{
		constexpr int width = 4;
		const uint16 p1[] = {0x65,0x16,0x67,0x00}; 

		Buffer<width> pixmap1(p1);
		Buffer<width> pixmap2(p1);

		CHECK_EQ(compare_row<kio::Graphics::colordepth_1bpp>(pixmap1.px, pixmap2.px, 32), 0);
		pixmap2.px[3] += 0x80;
		CHECK_NE(compare_row<kio::Graphics::colordepth_1bpp>(pixmap1.px, pixmap2.px, 32), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_1bpp>(pixmap1.px, pixmap2.px, 31), 0);		
		pixmap2.px[3] -= 0x80;
		pixmap2.px[3] += 0x40;
		CHECK_NE(compare_row<kio::Graphics::colordepth_1bpp>(pixmap1.px, pixmap2.px, 31), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_1bpp>(pixmap1.px, pixmap2.px, 30), 0);		
		pixmap2.px[3] -= 0x40;
		pixmap2.px[3] += 0x01;
		CHECK_NE(compare_row<kio::Graphics::colordepth_1bpp>(pixmap1.px, pixmap2.px, 25), 0);
		CHECK_EQ(compare_row<kio::Graphics::colordepth_1bpp>(pixmap1.px, pixmap2.px, 24), 0);		
	}
}

TEST_CASE("BitBlit: draw_bitmap<1bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_1bpp;
	
	// left aligned, color=1
	{
		constexpr int width = 12, height = 8;
		constexpr int x0=32,y0=2,w=30,h=4;
		const uint16 q[] = {
			0x6000
		};  
		const uint16 z[] = {
			0x0c00, 
			0x0c00, 
			0x0400, 0x25, 0x5A, 0xA5, 0x88, 0x0400,  
			0x0400, 0x26, 0x6B, 0xB6, 0x88, 0x0400,  
			0x0400, 0x27, 0x7C, 0xC7, 0x8C, 0x0400,  
			0x0400, 0x28, 0xD8, 0xD8, 0xFC, 0x0400,  
			0x0c00, 
			0x0C00, 
			};
		const uint8 b[] = {
			0x25, 0x5A, 0xA5, 0x8A, 
			0x26, 0x6B, 0xB6, 0x8B, 
			0x27, 0x7C, 0xC7, 0x8C, 
			0x28, 0xD8, 0xD8, 0xFF, 
			};
		
		Buffer<4,4>bmp(b);
		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		draw_bitmap<CD>(pixmap.px + y0*width, width, x0, bmp.px,4, w,h, 1);
		CHECK_EQ(pixmap, expect);
	}

	// left aligned, color=0
	{
		constexpr int width = 8, height = 8;
		constexpr int x0=16,y0=2,w=31,h=4;
		const uint16 q[] = {
			0x40ff
		};  
		const uint16 z[] = {
			0x08ff, 
			0x08ff, 
			0x02ff, 0xff-0x25, 0xff-0x5A, 0xff-0xA5, 0xff-0x8A, 0x02ff,  
			0x02ff, 0xff-0x26, 0xff-0x6B, 0xff-0xB6, 0xff-0x8A, 0x02ff,  
			0x02ff, 0xff-0x27, 0xff-0x7C, 0xff-0xC7, 0xff-0x8C, 0x02ff,  
			0x02ff, 0xff-0x28, 0xff-0xD8, 0xff-0xD8, 0xff-0xFE, 0x02ff,  
			0x08ff, 
			0x08ff, 
			};
		const uint8 b[] = {
			0x25, 0x5A, 0xA5, 0x8A, 
			0x26, 0x6B, 0xB6, 0x8B, 
			0x27, 0x7C, 0xC7, 0x8C, 
			0x28, 0xD8, 0xD8, 0xFF, 
			};
		
		Buffer<4,4>bmp(b);
		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		draw_bitmap<CD>(pixmap.px + y0*width, width, x0, bmp.px,4, w,h, 0);
		CHECK_EQ(pixmap, expect);
	}

	// left unaligned, color=1
	{
		constexpr int width = 8, height = 8;
		constexpr int x0=17,y0=2,w=32,h=4;
		const uint16 q[] = {
			0x4000
		};  
		const uint16 z[] = {
			0x0800, 
			0x0800, 
			0x0200, 0x25>>1, 0x80+(0x5A>>1), 0x00+(0xA5>>1), 0x80+(0x8A>>1), 0x00+0x00, 0x00,  
			0x0200, 0xF6>>1, 0x00+(0x6B>>1), 0x80+(0xB6>>1), 0x00+(0x1B>>1), 0x80+0x00, 0x00,  
			0x0200, 0x27>>1, 0x80+(0x7C>>1), 0x00+(0xC7>>1), 0x80+(0x8C>>1), 0x00+0x00, 0x00,  
			0x0200, 0x28>>1, 0x00+(0xD8>>1), 0x00+(0xD8>>1), 0x00+(0x88>>1), 0x00+0x00, 0x00,  
			0x0800, 
			0x0800, 
			};
		const uint8 b[] = {
			0x25, 0x5A, 0xA5, 0x8A, 
			0xF6, 0x6B, 0xB6, 0x1B, 
			0x27, 0x7C, 0xC7, 0x8C, 
			0x28, 0xD8, 0xD8, 0x88, 
			};
		
		Buffer<4,4>bmp(b);
		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		draw_bitmap<CD>(pixmap.px + y0*width, width, x0, bmp.px,4, w,h, 1);
		CHECK_EQ(pixmap, expect);
	}

	// left unaligned, color=0
	{
		constexpr int width = 8, height = 8;
		constexpr int x0=17,y0=2,w=32,h=4;
		const uint16 q[] = {
			0x40ff
		};  
		const uint16 z[] = {
			0x08ff, 
			0x08ff, 
			0x02ff, 0x80+(0x25>>1), 0x80+(0x5A>>1), 0x00+(0xA5>>1), 0x80+(0x8A>>1), 0x00+0x7f, 0xff,  
			0x02ff, 0x80+(0xF6>>1), 0x00+(0x6B>>1), 0x80+(0xB6>>1), 0x00+(0x1B>>1), 0x80+0x7f, 0xff,  
			0x02ff, 0x80+(0x27>>1), 0x80+(0x7C>>1), 0x00+(0xC7>>1), 0x80+(0x8C>>1), 0x00+0x7f, 0xff,  
			0x02ff, 0x80+(0x28>>1), 0x00+(0xD8>>1), 0x00+(0xD8>>1), 0x00+(0x89>>1), 0x80+0x7f, 0xff,  
			0x08ff, 
			0x08ff, 
			};
		const uint8 b[] = {
			0xff-0x25, 0xff-0x5A, 0xff-0xA5, 0xff-0x8A, 
			0xff-0xF6, 0xff-0x6B, 0xff-0xB6, 0xff-0x1B, 
			0xff-0x27, 0xff-0x7C, 0xff-0xC7, 0xff-0x8C, 
			0xff-0x28, 0xff-0xD8, 0xff-0xD8, 0xff-0x89, 
			};
		
		Buffer<4,4>bmp(b);
		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		draw_bitmap<CD>(pixmap.px + y0*width, width, x0, bmp.px,4, w,h, 0);
		CHECK_EQ(pixmap, expect);
	}

	// left unaligned, right aligned, color=1
	{
		constexpr int width = 8, height = 4;
		constexpr int x0=17,y0=1,w=31,h=2;
		const uint16 q[] = {
			0x2000
		};  
		const uint16 z[] = {
			0x0800, 
			0x0200, 0x25>>1, 0x80+(0x5A>>1), 0x00+(0xA5>>1), 0x80+(0x9F>>1), 0x0200,  
			0x0200, 0x25>>1, 0x80+(0x5A>>1), 0x00+(0xA5>>1), 0x80+(0x9F>>1), 0x0200,  
			0x0800, 
			};
		const uint8 b[] = {
			0x25, 0x5A, 0xA5, 0x9F, 
			0x25, 0x5A, 0xA5, 0x9F, 
			};
		
		Buffer<4,2>bmp(b);
		Buffer<width,height> pixmap(q);
		Buffer<width,height> expect(z);

		draw_bitmap<CD>(pixmap.px + y0*width, width, x0, bmp.px,4, w,h, 1);
		CHECK_EQ(pixmap, expect);
	}
}

TEST_CASE("BitBlit: draw_bitmap<2bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_2bpp;

	// x1 not|aligned
	// x2 not|aligned
	// xoffs&1
	// color 0,1,2,3

	constexpr int bmp_roffs=5, bmp_width=32, bmp_height=4;
	const uint8 b[] = {
		0x81, 0x23, 0x47, 0x67, 0xff,
		0x34, 0x33, 0x45, 0xf1, 0xff,
		0x71, 0x73, 0xDF, 0x73, 0xff,
		0xC3, 0xe3, 0xCE, 0x01, 0xff,
		};
	
	Buffer<bmp_roffs,bmp_height> bmp{b};

	constexpr int width = 16, height = 6;
	const uint16 null[] = {0x6000};
	Buffer<width,height> pixmap(null);
	Buffer<width,height> expect(null);
	
	// l+r aligned, all colors:	
	{
		for(uint bg=0; bg<=3; bg++)
			for(uint fg=0; fg<=3; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 16, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l aligned, r unaligned, all colors:	
	{
		for(uint bg=0; bg<=3; bg++)
			for(uint fg=0; fg<=3; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 16, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l unaligned, all colors:	
	{
		for(uint bg=0; bg<=3; bg++)
			for(uint fg=0; fg<=3; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 13, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// odd pixmap width:
	{
		constexpr int width = 17, height = 6;
		const uint16 null[] = {0x6600};
		Buffer<width,height> pixmap(null);
		Buffer<width,height> expect(null);
		
		for(uint bg=0; bg<=3; bg++)
			for(uint fg=0; fg<=3; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 20, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 20, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
}

TEST_CASE("BitBlit: draw_bitmap<4bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_4bpp;

	// x1 not|aligned
	// x2 not|aligned
	// xoffs&1
	// color 0,1,2,3 … 15

	constexpr int bmp_roffs=5, bmp_width=32, bmp_height=4;
	const uint8 b[] = {
		0x81, 0x23, 0x47, 0x67, 0xff,
		0x34, 0x33, 0x45, 0xf1, 0xff,
		0x71, 0x73, 0xDF, 0x73, 0xff,
		0xC3, 0xe3, 0xCE, 0x01, 0xff,
		};
	
	Buffer<bmp_roffs,bmp_height> bmp{b};

	constexpr int width = 32, height = 6;
	const uint16 null[] = {0xC000};
	Buffer<width,height> pixmap(null);
	Buffer<width,height> expect(null);
	
	// l+r aligned, all colors:	
	{
		for(uint bg=0; bg<=15; bg+=3)
			for(uint fg=0; fg<=15; fg+=3)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 16, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l aligned, r unaligned, all colors:	
	{
		for(uint bg=0; bg<=15; bg+=3)
			for(uint fg=0; fg<=15; fg+=3)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 16, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l unaligned, all colors:	
	{
		for(uint bg=0; bg<=15; bg+=3)
			for(uint fg=0; fg<=15; fg+=3)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 13, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// odd pixmap width:
	{
		constexpr int width = 33, height = 6;
		const uint16 null[] = {0xC600};
		Buffer<width,height> pixmap(null);
		Buffer<width,height> expect(null);
		
		for(uint bg=0; bg<=15; bg+=3)
			for(uint fg=3; fg<=12; fg+=3)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 20, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 20, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
}

TEST_CASE("BitBlit: draw_bitmap<8bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_8bpp;

	// x1 not|aligned
	// x2 not|aligned
	// xoffs&1
	// color 0,1,2,3 … 255

	constexpr int bmp_roffs=4, bmp_width=24, bmp_height=4;
	const uint8 b[] = {
		0x81, 0x23, 0x67, 0xff,
		0x34, 0x33, 0xf1, 0xff,
		0x71, 0x73, 0x73, 0xff,
		0xC3, 0xe3, 0x01, 0xff,
		};
	
	Buffer<bmp_roffs,bmp_height> bmp{b};

	constexpr int width = 32, height = 6;
	const uint16 null[] = {0xC000};
	Buffer<width,height> pixmap(null);
	Buffer<width,height> expect(null);
	
	// l+r aligned, all colors:	
	{
		for(uint bg=0; bg<=255; bg+=51)
			for(uint fg=0; fg<=255; fg+=85)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l aligned, r unaligned, all colors:	
	{
		for(uint bg=0; bg<=255; bg+=51)
			for(uint fg=0; fg<=255; fg+=85)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 4, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 4, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l unaligned, all colors:	
	{
		for(uint bg=0; bg<=255; bg+=51)
			for(uint fg=0; fg<=255; fg+=85)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 5, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 5, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// odd pixmap width:
	{
		constexpr int width = 33, height = 6;
		const uint16 null[] = {0xC600};
		Buffer<width,height> pixmap(null);
		Buffer<width,height> expect(null);
		
		for(uint bg=0; bg<=255; bg+=51)
			for(uint fg=0; fg<=255; fg+=85)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
}

TEST_CASE("BitBlit: draw_bitmap<16bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_16bpp;

	// x1 not|aligned
	// x2 not|aligned
	// xoffs&1
	// color 0,1,2,3 … 0xffff

	constexpr int bmp_roffs=4, bmp_width=24, bmp_height=4;
	const uint8 b[] = {
		0x81, 0x23, 0x67, 0xff,
		0x34, 0x33, 0xf1, 0xff,
		0x71, 0x73, 0x73, 0xff,
		0xC3, 0xe3, 0x01, 0xff,
		};
	
	Buffer<bmp_roffs,bmp_height> bmp{b};

	constexpr int width = 64, height = 6;
	const uint16 null[] = {0xC000,0xC000};
	Buffer<width,height> pixmap(null);
	Buffer<width,height> expect(null);
	
	// l+r aligned, all colors:	
	{
		for(uint bg=0; bg<=0xffff; bg+=0xffff/3)
			for(uint fg=0; fg<=0xffff; fg+=0xffff/5)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l aligned, r unaligned, all colors:	
	{
		for(uint bg=0; bg<=0xffff; bg+=0xffff/3)
			for(uint fg=0; fg<=0xffff; fg+=0xffff/5)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 4, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 4, bmp.px, bmp_roffs, bmp_width-1, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// l unaligned, all colors:	
	{
		for(uint bg=0; bg<=0xffff; bg+=0xffff/3)
			for(uint fg=0; fg<=0xffff; fg+=0xffff/5)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 5, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 5, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// odd pixmap width:
	{
		constexpr int width = 66, height = 6;
		const uint16 null[] = {0xC600,0xC600};
		Buffer<width,height> pixmap(null);
		Buffer<width,height> expect(null);
		
		for(uint bg=0; bg<=0xffff; bg+=0xffff/3)
			for(uint fg=0; fg<=0xffff; fg+=0xffff/5)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_bitmap<CD>(pixmap.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 4, bmp.px, bmp_roffs, bmp_width, bmp_height, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
}

TEST_CASE("BitBlit: draw_char<1bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_1bpp;
	
	constexpr int width = 8, height = 14;
	constexpr int chr_roffs=1, chr_height=12; // chr_width=8, 
	const uint16 q[] = { 0x7000 };  
	const uint8 b[] = {
		0x25, 0x5A, 0xA5, 0x8A, 0x26, 0x6B, 0xB6, 0x8B, 0x71, 0x73, 0xDF, 0x73, 
		0xA7, 0x7C, 0xC7, 0x8C, 0x28, 0xD8, 0xD8, 0xFF, 0xC3, 0xe3, 0xCE, 0x01, 
		};
	
	Buffer<chr_roffs, 2*chr_height> chr(b);
	Buffer<width, height> pixmap(q);
	Buffer<width, height> expect(q);
	
	// aligned:
	{
		for(uint bg=0; bg<=1; bg++)
			for(uint fg=0; fg<=1; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_char<CD>(pixmap.px+width, width, 16, chr.px, 12, fg);
				draw_char<CD>(pixmap.px+width, width, 16+8, chr.px+12, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16, chr.px, 1, 8, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16+8, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// unaligned
	{
		for(uint bg=0; bg<=1; bg++)
			for(uint fg=0; fg<=1; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_char<CD>(pixmap.px+width, width, 13, chr.px, 12, fg);
				draw_char<CD>(pixmap.px+width, width, 13+8, chr.px+12, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13, chr.px, 1, 8, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13+8, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// odd row offset
	{
		constexpr int width = 7, height = 16;
		const uint16 q[] = { 0x7000 };  
		
		Buffer<width, height> pixmap(q);
		Buffer<width, height> expect(q);
		
		for(uint bg=0; bg<=1; bg++)
			for(uint fg=0; fg<=1; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_char<CD>(pixmap.px+width*2, width, 16, chr.px, 12, fg);
				draw_char<CD>(pixmap.px+width*2, width, 16+9, chr.px+12, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width*2, width, 16, chr.px, 1, 8, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width*2, width, 16+9, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
}

TEST_CASE("BitBlit: draw_char<2bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_2bpp;
	
	constexpr int width = 16, height = 14;
	constexpr int chr_roffs=1, chr_height=12; // chr_width=8, 
	const uint16 q[] = { 0xE000 };  
	const uint8 b[] = {
		0x25, 0x5A, 0xA5, 0x8A, 0x26, 0x6B, 0xB6, 0x8B, 0x71, 0x73, 0xDF, 0x73, 
		0xA7, 0x7C, 0xC7, 0x8C, 0x28, 0xD8, 0xD8, 0xFF, 0xC3, 0xe3, 0xCE, 0x01, 
		};
	
	Buffer<chr_roffs, 2*chr_height> chr(b);
	Buffer<width, height> pixmap(q);
	Buffer<width, height> expect(q);
	
	// aligned:
	{
		for(uint bg=0; bg<=3; bg++)
			for(uint fg=0; fg<=3; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);
				draw_char<CD>(pixmap.px+width, width, 16, chr.px, 12, fg);
				draw_char<CD>(pixmap.px+width, width, 16+8, chr.px+12, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16, chr.px, 1, 8, 12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16+8, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// unaligned
	{
		for(uint bg=0; bg<=3; bg++)
			for(uint fg=0; fg<=3; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x55, width*height);
				memset(expect.px, int(bg)*0x55, width*height);				
				draw_char<CD>      (pixmap.px+width, width, 13, chr.px,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13, chr.px, 1, 8, 12, fg);				
				draw_char<CD>      (pixmap.px+width, width, 13+8, chr.px+12,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13+8, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// odd row offset
	{
		constexpr int width = 15, height = 16;
		const uint16 q[] = { 0xF000 };  
		
		Buffer<width, height> pixmap(q);
		Buffer<width, height> expect(q);
		
		for(uint bg=0; bg<=1; bg++)
			for(uint fg=0; fg<=1; fg++)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0xAA, width*height);
				memset(expect.px, int(bg)*0xAA, width*height);
				draw_char<CD>      (pixmap.px+width*2, width, 16, chr.px,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width*2, width, 16, chr.px, 1, 8, 12, fg);
				draw_char<CD>      (pixmap.px+width*1, width, 16+9, chr.px+12,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width*1, width, 16+9, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
}

TEST_CASE("BitBlit: draw_char<4bpp>")
{
	constexpr ColorDepth CD = kio::Graphics::colordepth_4bpp;
	
	constexpr int width = 32, height = 14;
	constexpr int chr_roffs=1, chr_height=12; // chr_width=8, 
	const uint16 q[] = { 0xE000,0xE000 };  
	const uint8 b[] = {
		0x25, 0x5A, 0xA5, 0x8A, 0x26, 0x6B, 0xB6, 0x8B, 0x71, 0x73, 0xDF, 0x73, 
		0xA7, 0x7C, 0xC7, 0x8C, 0x28, 0xD8, 0xD8, 0xFF, 0xC3, 0xe3, 0xCE, 0x01, 
		};
	
	Buffer<chr_roffs, 2*chr_height> chr(b);
	Buffer<width, height> pixmap(q);
	Buffer<width, height> expect(q);
	
	// aligned:
	{
		for(uint bg=0; bg<=15; bg+=5)
			for(uint fg=0; fg<=15; fg+=3)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x11, width*height);
				memset(expect.px, int(bg)*0x11, width*height);

				draw_char<CD>      (pixmap.px+width, width, 16, chr.px,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16, chr.px, 1, 8, 12, fg);
				
				draw_char<CD>      (pixmap.px+width, width, 16+8, chr.px+12,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 16+8, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
	
	// unaligned
	{
		for(uint bg=0; bg<=15; bg+=5)
			for(uint fg=0; fg<=15; fg+=3)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x11, width*height);
				memset(expect.px, int(bg)*0x11, width*height);				

				draw_char<CD>      (pixmap.px+width, width, 13, chr.px,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13, chr.px, 1, 8, 12, fg);				
				
				draw_char<CD>      (pixmap.px+width, width, 13+8, chr.px+12,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width, width, 13+8, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}

	// odd row offset
	{
		constexpr int width = 31, height = 16;
		const uint16 q[] = { 0xF800,0xF800 };  
		
		Buffer<width, height> pixmap(q);
		Buffer<width, height> expect(q);
		
		for(uint bg=0; bg<=15; bg+=5)
			for(uint fg=0; fg<=15; fg+=3)
			{
				if (bg==fg) continue;
				memset(pixmap.px, int(bg)*0x11, width*height);
				memset(expect.px, int(bg)*0x11, width*height);
				
				draw_char<CD>      (pixmap.px+width*2, width, 16, chr.px,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width*2, width, 16, chr.px, 1, 8, 12, fg);
				
				draw_char<CD>      (pixmap.px+width*1, width, 16+9, chr.px+12,       12, fg);
				draw_bitmap_ref<CD>(expect.px+width*1, width, 16+9, chr.px+12, 1, 8, 12, fg);
				CHECK_EQ(pixmap,expect);
			}
	}
}

TEST_CASE("BitBlit: clear_row_of_bits")
{
	constexpr int width = 20, height = 3;
	const uint16 q[] = { 0x3C00 };  
	
	Buffer<width, height> pixmap(q);
	Buffer<width, height> expect(q);
	
	for(uint c=0;c<=1;c++)
	{
		uint bg = flood_filled_color<colordepth_1bpp>(c);
		uint fg = ~bg;
		
		for(int x=0;x<=32;x++)
		{			
			memset(pixmap.px, int(bg), width*height);
			memset(expect.px, int(bg), width*height);
			
			clear_row_of_bits(pixmap.px+width, 32+x, 96-x, fg);
			draw_hline_ref<colordepth_1bpp>(expect.px+width,32+x,96-x,fg);
			CHECK_EQ(pixmap,expect);
		}
		
		for(int w=64;w<=96;w++)
		{			
			memset(pixmap.px, int(bg), width*height);
			memset(expect.px, int(bg), width*height);
			
			clear_row_of_bits(pixmap.px+width, 32, w, fg);
			draw_hline_ref<colordepth_1bpp>(expect.px+width, 32, w, fg);
			CHECK_EQ(pixmap,expect);
		}

		{			
			memset(pixmap.px, int(bg), width*height);
			memset(expect.px, int(bg), width*height);
			
			clear_row_of_bits(pixmap.px,       32,    1, fg);
			clear_row_of_bits(pixmap.px+width, 32+31, 1, fg);
			clear_row_of_bits(pixmap.px,       64,   -1, fg);
			set_pixel<colordepth_1bpp>(expect.px,       32,    fg);
			set_pixel<colordepth_1bpp>(expect.px+width, 32+31, fg);
			CHECK_EQ(pixmap,expect);
		}
	}
}

TEST_CASE("BitBlit: draw_hline")
{	
	constexpr int width = 32, height = 3;
	const uint16 q[] = { 0x6000 };  
	
	Buffer<width, height> pixmap(q);
	Buffer<width, height> expect(q);
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_1bpp;
		
		for(uint bg=0; bg<=1; bg++)
		{
			uint fg = 1 - bg;
			
			for(int w = (width*8/2 - 8) >> CD; w >= -7; w-=7)
			{				
				int x = ((width*8-w)>>CD) / 3;
				
				memset(pixmap.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				
				draw_hline<CD>    (pixmap.px+width, x, w, fg);
				draw_hline_ref<CD>(expect.px+width, x, w, fg);
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_2bpp;
		
		for(uint bg=0; bg<=3; bg++)
		{
			uint fg = 3 - bg;
			
			for(int w = (width*8/2 - 8) >> CD; w >= -5; w-=5)
			{				
				int x = ((width*8-w)>>CD) / 3;
				
				memset(pixmap.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				
				draw_hline<CD>    (pixmap.px+width, x, w, fg);
				draw_hline_ref<CD>(expect.px+width, x, w, fg);
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_4bpp;
		
		for(uint bg=0; bg<=15; bg+=5)
		{
			uint fg = 15 - bg;
			
			for(int w = (width*8/2 - 8) >> CD; w >= -5; w-=5)
			{				
				int x = ((width*8-w)>>CD) / 3;
				
				memset(pixmap.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				
				draw_hline<CD>    (pixmap.px+width, x, w, fg);
				draw_hline_ref<CD>(expect.px+width, x, w, fg);
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}

	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_8bpp;
		
		for(uint bg=0; bg<=255; bg+=51)
		{
			uint fg = 255-bg;
			
			for(int w = (width*8/2 - 8) >> CD; w >= -5; w-=5)
			{				
				int x = ((width*8-w)>>CD) / 3;
				
				memset(pixmap.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				
				draw_hline<CD>    (pixmap.px+width, x, w, fg);
				draw_hline_ref<CD>(expect.px+width, x, w, fg);
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}
	
	{
		constexpr int width = 32, height = 3;
		const uint16 q[] = { 0x6000 };  
		
		Buffer<width, height> pixmap(q);
		Buffer<width, height> expect(q);
		
		constexpr ColorDepth CD = kio::Graphics::colordepth_16bpp;
		
		for(uint bg = 0; bg<=0xffff; bg+=0xffff/3)
		{
			uint fg = 0xffff - bg;
			
			for(int w = (width*8/2 - 8) >> CD; w >= -5; w-=5)
			{				
				int x = ((width*8-w)>>CD) / 3;
				
				memset(pixmap.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				
				draw_hline<CD>    (pixmap.px+width, x, w, fg);
				draw_hline_ref<CD>(expect.px+width, x, w, fg);
				
				CHECK_EQ(pixmap,expect);
			}
		}
	}
}

TEST_CASE("BitBlit: draw_vline")
{
	constexpr int width = 8, height = 16;
	constexpr int width2 = 11;
	const uint16 q1[] = { 0x8000 };  
	const uint16 q2[] = { 0x8000, 0x3000 };  
	
	Buffer<width, height> pixmap1(q1);
	Buffer<width2,height> pixmap2(q2);
	Buffer<width, height> expect(q1);
	Buffer<width2,height> expect2(q2);
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_1bpp;
		
		for(uint bg=0; bg<=1; bg++)
		{
			uint fg = 1 - bg;
			
			for(int x = 0; x < 64; x+=7)
			{		
				int h = height -2 - x/6;
				
				memset(pixmap1.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(pixmap2.px, int(flood_filled_color<CD>(bg)), width2*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect2.px, int(flood_filled_color<CD>(bg)), width2*height);
				
				draw_vline<CD>    (pixmap1.px+width, width, x, h, fg);
				draw_vline<CD>    (pixmap2.px+width2, width2, x, h, fg);
				draw_vline_ref<CD>(expect.px+width, width, x, h, fg);
				draw_vline_ref<CD>(expect2.px+width2, width2, x, h, fg);
				
				CHECK_EQ(pixmap1,expect);
				CHECK_EQ(pixmap2,expect2);
			}
		}
	}
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_2bpp;
		
		for(uint bg=0; bg<=3; bg+=3)
		{
			uint fg = 3 - bg;
			
			for(int x = 0; x < 32; x+=5)
			{		
				int h = height -2 - x/3;
				
				memset(pixmap1.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(pixmap2.px, int(flood_filled_color<CD>(bg)), width2*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect2.px, int(flood_filled_color<CD>(bg)), width2*height);
				
				draw_vline<CD>    (pixmap1.px+width, width, x, h, fg);
				draw_vline<CD>    (pixmap2.px+width2, width2, x, h, fg);
				draw_vline_ref<CD>(expect.px+width, width, x, h, fg);
				draw_vline_ref<CD>(expect2.px+width2, width2, x, h, fg);
				
				CHECK_EQ(pixmap1,expect);
				CHECK_EQ(pixmap2,expect2);
			}
		}
	}
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_4bpp;
		
		for(uint bg=0; bg<=15; bg+=15)
		{
			uint fg = 15 - bg;
			
			for(int x = 0; x < 16; x+=5)
			{		
				int h = height -2 - x*2/3;
				
				memset(pixmap1.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(pixmap2.px, int(flood_filled_color<CD>(bg)), width2*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect2.px, int(flood_filled_color<CD>(bg)), width2*height);
				
				draw_vline<CD>    (pixmap1.px+width, width, x, h, fg);
				draw_vline<CD>    (pixmap2.px+width2, width2, x, h, fg);
				draw_vline_ref<CD>(expect.px+width, width, x, h, fg);
				draw_vline_ref<CD>(expect2.px+width2, width2, x, h, fg);
				
				CHECK_EQ(pixmap1,expect);
				CHECK_EQ(pixmap2,expect2);
			}
		}
	}
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_8bpp;
		
		for(uint bg=0; bg<=255; bg+=255)
		{
			uint fg = 255 - bg;
			
			for(int x = 3; x < 8; x+=1)
			{		
				int h = height -2 - x*4/3;
				
				memset(pixmap1.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(pixmap2.px, int(flood_filled_color<CD>(bg)), width2*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect2.px, int(flood_filled_color<CD>(bg)), width2*height);
				
				draw_vline<CD>    (pixmap1.px+width, width, x, h, fg);
				draw_vline<CD>    (pixmap2.px+width2, width2, x, h, fg);
				draw_vline_ref<CD>(expect.px+width, width, x, h, fg);
				draw_vline_ref<CD>(expect2.px+width2, width2, x, h, fg);
				
				CHECK_EQ(pixmap1,expect);
				CHECK_EQ(pixmap2,expect2);
			}
		}
	}
	
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_16bpp;
		
		for(uint bg=0; bg<=0xffff; bg+=0xffff)
		{
			uint fg = 0xffff - bg;
			
			for(int x = 0; x < 4; x++)
			{		
				int h = height -2 - x*3;
				
				memset(pixmap1.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(pixmap2.px, int(flood_filled_color<CD>(bg)), width2*height);
				memset(expect.px, int(flood_filled_color<CD>(bg)), width*height);
				memset(expect2.px, int(flood_filled_color<CD>(bg)), width2*height);
				
				draw_vline<CD>    (pixmap1.px+width, width, x, h, fg);
				draw_vline<CD>    (pixmap2.px+width2, width2, x, h, fg);
				draw_vline_ref<CD>(expect.px+width, width, x, h, fg);
				draw_vline_ref<CD>(expect2.px+width2, width2, x, h, fg);
				
				CHECK_EQ(pixmap1,expect);
				CHECK_EQ(pixmap2,expect2);
			}
		}
	}
}

TEST_CASE("BitBlit: clear_row: uint8*, uint16*, uint32*")
{
	uint16 p[]= { 0xa000, 0xa000 };
	uint16 e[]=
	{
		0x1400,
		0x0400, 0x0CAA, 0x0400,
		0x0300, 0x0CAA, 0x0500,
		0x0200, 0x0CAA, 0x0600,
		0x0100, 0x0CAA, 0x0700,
		
		0x0400, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0x0400,
		0x0200, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0xab,0xcd, 0x0600,		
		0x0400, 0x23,0x45,0x67,0x89, 0x23,0x45,0x67,0x89, 0x23,0x45,0x67,0x89, 0x0400,
		
		0x0400, 0x01AA, 0x0F00,
		0x0300, 0x01AA, 0x1000,
		0x0200, 0x01AA, 0x1100,
		0x0100, 0x01AA, 0x1200,
		
		0x0400, 0xab,0xcd, 0x0E00,
		0x0200, 0xab,0xcd, 0x1000,		
		
		0x0400, 0x23,0x45,0x67,0x89, 0x0C00,
		0x1400,
	};

	constexpr int width=20, height=16;	
	Buffer<width,height> pixmap(p);
	Buffer<width,height> expect(e);

	clear_row(pixmap.px + width*1 + 4, 12, flip(0xAAAAAAAAu));
	clear_row(pixmap.px + width*2 + 3, 12, flip(0xAAAAAAAAu));
	clear_row(pixmap.px + width*3 + 2, 12, flip(0xAAAAAAAAu));
	clear_row(pixmap.px + width*4 + 1, 12, flip(0xAAAAAAAAu));
	clear_row(reinterpret_cast<uint16*>(pixmap.px + width*5 + 4), 6, flip(0xabcdabcdu));
	clear_row(reinterpret_cast<uint16*>(pixmap.px + width*6 + 2), 6, flip(0xabcdabcdu));
	clear_row(reinterpret_cast<uint32*>(pixmap.px + width*7 + 4), 3, flip(0x23456789u));
	
	clear_row(pixmap.px + width*8  + 4, 1, flip(0xAAAAAAAAu));
	clear_row(pixmap.px + width*9  + 3, 1, flip(0xAAAAAAAAu));
	clear_row(pixmap.px + width*10 + 2, 1, flip(0xAAAAAAAAu));
	clear_row(pixmap.px + width*11 + 1, 1, flip(0xAAAAAAAAu));
	clear_row(reinterpret_cast<uint16*>(pixmap.px + width*12 + 4), 1, flip(0xabcdabcdu));
	clear_row(reinterpret_cast<uint16*>(pixmap.px + width*13 + 2), 1, flip(0xabcdabcdu));
	clear_row(reinterpret_cast<uint32*>(pixmap.px + width*14 + 4), 1, flip(0x23456789u));
	
	CHECK_EQ(pixmap,expect);
}

TEST_CASE("BitBlit: xor_rect, xor_rect_of_bits")
{
	// 1bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_1bpp;

		constexpr int width = 8, height = 8;
		const uint16 q[] = { 
			0x0800,
			0x0800,
			0x0805,
			0x080A,
			0x08A0,
			0x08AA,
			0x08FF,
			0x0800,
		};  
		
		for(int x=8;x<=16;x++) // long line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=6*8, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(1));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(1));
			CHECK_EQ(pixmap,expect);
		}
	
		for(int x=8;x<=16;x++) // short line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=6, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(1));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(1));
			CHECK_EQ(pixmap,expect);
		}

		// empty rect:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			
			xor_rect<CD>(pixmap.px+width, width, 5, 0, 4, flood_filled_color<CD>(1));
			xor_rect<CD>(pixmap.px+width, width, 5, 8, -1, flood_filled_color<CD>(1));
			xor_rect<CD>(pixmap.px+width, width, 8, -2, 4, flood_filled_color<CD>(1));
			xor_rect<CD>(pixmap.px+width, width, 8, 8, 0, flood_filled_color<CD>(1));
			CHECK_EQ(pixmap,expect);
		}
	}
	
	// 2bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_2bpp;

		constexpr int width = 16, height = 8;
		const uint16 q[] = { 
			0x1000,
			0x1000,
			0x1005,
			0x100A,
			0x10A0,
			0x10AA,
			0x10FF,
			0x1000,
		};  
		
		for(int x=8;x<=16;x++) // long line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=6*8, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(1));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(1));
			CHECK_EQ(pixmap,expect);
		}
	
		for(int x=8;x<=16;x++) // short line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=3, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(1));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(1));
			CHECK_EQ(pixmap,expect);
		}

		// empty rect:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			
			xor_rect<CD>(pixmap.px+width, width, 5, 0, 4, flood_filled_color<CD>(1));
			xor_rect<CD>(pixmap.px+width, width, 5, 8, -1, flood_filled_color<CD>(1));
			xor_rect<CD>(pixmap.px+width, width, 8, -2, 4, flood_filled_color<CD>(1));
			xor_rect<CD>(pixmap.px+width, width, 8, 8, 0, flood_filled_color<CD>(1));
			CHECK_EQ(pixmap,expect);
		}
	}
	
	// 4bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_4bpp;

		constexpr int width = 16, height = 8;
		const uint16 q[] = { 
			0x1000,
			0x1000,
			0x1005,
			0x100A,
			0x10A0,
			0x10AA,
			0x10FF,
			0x1000,
		};  
		
		for(int x=2;x<=8;x++) // long line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=20, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(0xA));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(0xA));
			CHECK_EQ(pixmap,expect);
		}
	
		for(int x=2;x<=8;x++) // short line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=1, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(0xA));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(0xA));
			CHECK_EQ(pixmap,expect);
		}

		// empty rect:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			
			xor_rect<CD>(pixmap.px+width, width, 5, 0,  4, flood_filled_color<CD>(0xA));
			xor_rect<CD>(pixmap.px+width, width, 5, 8, -1, flood_filled_color<CD>(0xA));
			xor_rect<CD>(pixmap.px+width, width, 8, -2, 4, flood_filled_color<CD>(0xA));
			xor_rect<CD>(pixmap.px+width, width, 8, 8,  0, flood_filled_color<CD>(0xA));
			CHECK_EQ(pixmap,expect);
		}
	}
	
	// 8bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_8bpp;

		constexpr int width = 32, height = 8;
		const uint16 q[] = { 
			0x2000,
			0x2000,
			0x2005,
			0x200A,
			0x20A0,
			0x20AA,
			0x20FF,
			0x2000,
		};  
		
		for(int x=2;x<=8;x++) // long line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=20, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(0xAA));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(0xAA));
			CHECK_EQ(pixmap,expect);
		}
	
		for(int x=2;x<=8;x++) // short line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=1, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(0xAA));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(0xAA));
			CHECK_EQ(pixmap,expect);
		}

		// empty rect:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			
			xor_rect<CD>(pixmap.px+width, width, 5, 0,  4, flood_filled_color<CD>(0xAA));
			xor_rect<CD>(pixmap.px+width, width, 5, 8, -1, flood_filled_color<CD>(0xAA));
			xor_rect<CD>(pixmap.px+width, width, 8, -2, 4, flood_filled_color<CD>(0xAA));
			xor_rect<CD>(pixmap.px+width, width, 8, 8,  0, flood_filled_color<CD>(0xAA));
			CHECK_EQ(pixmap,expect);
		}
	}
	
	// 16bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_16bpp;

		constexpr int width = 32, height = 8;
		const uint16 q[] = { 
			0x2000,
			0x2000,
			0x2005,
			0x200A,
			0x20A0,
			0x20AA,
			0x20FF,
			0x2000,
		};  
		
		for(int x=1;x<=2;x++) // long line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=5, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(0xA55A));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(0xA55A));
			CHECK_EQ(pixmap,expect);
		}
	
		for(int x=2;x<=8;x++) // short line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=1, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(0xA55A));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(0xA55A));
			CHECK_EQ(pixmap,expect);
		}

		// empty rect:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			
			xor_rect<CD>(pixmap.px+width, width, 5, 0,  4, flood_filled_color<CD>(0xA55A));
			xor_rect<CD>(pixmap.px+width, width, 5, 8, -1, flood_filled_color<CD>(0xA55A));
			xor_rect<CD>(pixmap.px+width, width, 8, -2, 4, flood_filled_color<CD>(0xA55A));
			xor_rect<CD>(pixmap.px+width, width, 8, 8,  0, flood_filled_color<CD>(0xA55A));
			CHECK_EQ(pixmap,expect);
		}
	}

	// odd row offset:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_1bpp;

		constexpr int width = 11, height = 8;
		const uint16 q[] = { 
			0x0B00,
			0x0B00,
			0x0B05,
			0x0B0A,
			0x0BA0,
			0x0BAA,
			0x0BFF,
			0x0B00,
		};  
		
		for(int x=8;x<=16;x++) // long line:
		{			
			Buffer<width, height> pixmap(q);
			Buffer<width, height> expect(q);
			const int w=6*8, h=6;
			
			xor_rect<CD>    (pixmap.px+width, width, x, w, h, flood_filled_color<CD>(1));
			xor_rect_ref<CD>(expect.px+width, width, x, w, h, flood_filled_color<CD>(1));
			CHECK_EQ(pixmap,expect);
		}
	}	
}

TEST_CASE("BitBlit: copy_rect, copy_rect_of_bits, copy_rect_of_bytes")
{
	const uint8 q[256] = { 
		123, 54, 23, 01, 55, 201, 255, 67, 
		24, 75, 233, 10, 77, 88, 213, 244, 
		0x41, 0x62, 0xC7, 0x37, 0x30, 0x46, 0x62, 0xBB, 
		158, 64, 32, 233, 98, 90, 240, 43, 
		0x80, 0xc0, 0xe0, 0xf0, 15, 7, 3, 1, 
		0xc0, 23, 43, 67, 88, 76, 45, 233, 
		213, 80, 77, 65, 43, 21, 10, 98, 
		89, 00, 255, 65, 43, 210, 58, 0xf, 		
		124, 210, 214, 188, 7, 62, 235, 54,   
		71, 254, 229, 133, 214, 88, 69, 182,  
		198, 25, 123, 125, 63, 27, 16, 248,   
		71, 35, 121, 58, 111, 155, 117, 143,  
		128, 0, 78, 143, 149, 171, 187, 144,  
		240, 129, 161, 112, 40, 228, 111, 159,
		140, 251, 97, 109, 134, 198, 76, 38,  
		55, 13, 230, 121, 81, 29, 108, 178,   
		124, 210, 139, 46, 117, 6, 89, 107,
		93, 145, 75, 116, 164, 70, 219, 46,
		203, 27, 107, 190, 99, 100, 42, 254,
		66, 133, 214, 253, 160, 4, 78, 166,
		164, 40, 56, 216, 213, 82, 135, 155,
		14, 34, 17, 228, 205, 226, 176, 124,
		8, 141, 220, 204, 68, 9, 208, 95,
		6, 247, 128, 249, 168, 21, 111, 183,		
		74, 50, 76, 199, 92, 68, 117, 32,
		191, 164, 211, 241, 58, 68, 102, 137,
		130, 78, 231, 206, 30, 165, 181, 131,
		78, 238, 160, 25, 72, 26, 214, 246,
		244, 57, 226, 116, 110, 87, 109, 240,
		181, 66, 200, 255, 9, 160, 221, 96,
		244, 78, 109, 83, 24, 250, 132, 137,
		252, 129, 0, 46, 219, 255, 44, 119,		
	};  
	
	// 1bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_1bpp;

		constexpr int width = 28, height = 9;
		constexpr int width2 = 27;

		for(int zx=8; zx<=32; zx+=2) // copy left:		
		{			
			for(int qx=zx; qx<=zx+32; qx+=3)
			{
				for(int w=0,h=5; w<=13*8; w+=11)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				

					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2,width2, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2,width2, zx, expect2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}
		for(int zx=8;zx<=32;zx+=3) // copy down:		
		{			
			for(int qx=zx-8;qx<=zx+32;qx+=5)
			{
				for(int w=0,h=5; w<=12*8; w+=9)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2,width2, zx, pixmap2.px+width2*2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2,width2, zx, expect2.px+width2*2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		
		for(int zx=8; zx<=32; zx+=2) // copy right:		
		{			
			for(int qx=zx-8; qx<zx; qx++)
			{
				for(int w=0,h=5; w<=11*8; w+=9)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
										
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				

					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					//if(zx==16&&qx==9&&w==9)
					//	LOL
					
					copy_rect<CD>    (pixmap2.px+width2,width2, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2,width2, zx, expect2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}
		for(int zx=8;zx<=32;zx+=3) // copy up:		
		{			
			for(int qx=zx-8;qx<=zx+8;qx+=5)
			{
				for(int w=0,h=5; w<=13*8; w+=13)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2*2,width2, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2*2,width2, zx, expect2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		

		for(int zx=8;zx<=32;zx+=3) // mixed stride:		
		{						
			for(int qx=zx-8;qx<=zx+8;qx+=2)
			{
				for(int w=1,h=7; w<=13*8; w+=17) // copy down, z=even, q=odd
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					Buffer<width2, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*8; w+=19) // copy up, z=even, q=odd
				{
					Buffer<width2, height> pixmap2(q);
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*8; w+=23) // copy down, z=odd, q=even
				{
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);
					Buffer<width, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*8; w+=27) // copy up, z=odd, q=even
				{
					Buffer<width, height> pixmap2(q);
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
			}			
		}		
	}
	
	// 2bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_2bpp;

		constexpr int width = 28, height = 9;
		constexpr int width2 = 27;

		for(int zx=4;zx<=16;zx+=2) // copy down:		
		{			
			for(int qx=zx-4;qx<=zx+16;qx+=3)
			{
				for(int w=0,h=5; w<=12*4; w+=5)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2,width2, zx, pixmap2.px+width2*2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2,width2, zx, expect2.px+width2*2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		
		for(int zx=4;zx<=16;zx+=2) // copy up:		
		{			
			for(int qx=zx-4;qx<=zx+4;qx+=3)
			{
				for(int w=0,h=5; w<=13*4; w+=9)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2*2,width2, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2*2,width2, zx, expect2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		

		for(int zx=4;zx<=16;zx+=2) // mixed stride:		
		{						
			for(int qx=zx-4;qx<=zx+4;qx+=1)
			{
				for(int w=1,h=7; w<=13*4; w+=9) // copy down, z=even, q=odd
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					Buffer<width2, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*4; w+=10) // copy up, z=even, q=odd
				{
					Buffer<width2, height> pixmap2(q);
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*4+1; w+=13) // copy down, z=odd, q=even
				{
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);
					Buffer<width, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*4; w+=11) // copy up, z=odd, q=even
				{
					Buffer<width, height> pixmap2(q);
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
			}			
		}		
	}
	
	// 4bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_4bpp;

		constexpr int width = 28, height = 9;
		constexpr int width2 = 27;

		for(int zx=2;zx<=8;zx+=1) // copy down:		
		{			
			for(int qx=zx-2;qx<=zx+8;qx+=2)
			{
				for(int w=0,h=5; w<=12*2; w+=3)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2,width2, zx, pixmap2.px+width2*2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2,width2, zx, expect2.px+width2*2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		
		for(int zx=2;zx<=8;zx+=2) // copy up:		
		{			
			for(int qx=zx-2;qx<=zx+2;qx+=1)
			{
				for(int w=0,h=5; w<=13*2; w+=5)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2*2,width2, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2*2,width2, zx, expect2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		

		for(int zx=2;zx<=8;zx+=2) // mixed stride:		
		{						
			for(int qx=zx-2;qx<=zx+2;qx+=1)
			{
				for(int w=1,h=7; w<=13*2; w+=3) // copy down, z=even, q=odd
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					Buffer<width2, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*2; w+=5) // copy up, z=even, q=odd
				{
					Buffer<width2, height> pixmap2(q);
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13*2; w+=6) // copy down, z=odd, q=even
				{
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);
					Buffer<width, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=14*2+1; w+=7) // copy up, z=odd, q=even
				{
					Buffer<width, height> pixmap2(q);
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
			}			
		}		
	}
	
	// 8bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_8bpp;

		constexpr int width = 28, height = 9;
		constexpr int width2 = 27;

		for(int zx=2;zx<=8;zx+=1) // copy down:		
		{			
			for(int qx=zx-2;qx<=zx+4;qx+=2)
			{
				for(int w=0,h=5; w<=12; w+=3)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2,width2, zx, pixmap2.px+width2*2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2,width2, zx, expect2.px+width2*2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		
		for(int zx=2;zx<=8;zx+=1) // copy up:		
		{			
			for(int qx=zx-2;qx<=zx+2;qx+=1)
			{
				for(int w=1,h=5; w<=13; w+=3)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2*2,width2, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2*2,width2, zx, expect2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		

		for(int zx=2;zx<=8;zx+=2) // mixed stride:		
		{						
			for(int qx=zx-2;qx<=zx+2;qx+=1)
			{
				for(int w=1,h=7; w<=13; w+=3) // copy down, z=even, q=odd
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					Buffer<width2, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=16; w+=5) // copy up, z=even, q=odd
				{
					Buffer<width2, height> pixmap2(q);
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=13; w+=6) // copy down, z=odd, q=even
				{
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);
					Buffer<width, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=14+1; w+=7) // copy up, z=odd, q=even
				{
					Buffer<width, height> pixmap2(q);
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
			}			
		}		
	}
	
	// 16bpp:
	{
		constexpr ColorDepth CD = kio::Graphics::colordepth_16bpp;

		constexpr int width = 28, height = 9;
		constexpr int width2 = 26;

		for(int zx=1;zx<=5;zx+=1) // copy down:		
		{			
			for(int qx=zx-1;qx<=zx+2;qx+=1)
			{
				for(int w=0,h=5; w<=6; w+=1)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2,width2, zx, pixmap2.px+width2*2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2,width2, zx, expect2.px+width2*2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		
		for(int zx=1;zx<=5;zx+=1) // copy up:		
		{			
			for(int qx=zx-1;qx<=zx+1;qx+=1)
			{
				for(int w=1,h=5; w<=7; w+=2)
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap.px+width*2,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, expect.px+width*2,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
					
					Buffer<width2, height> pixmap2(q);
					Buffer<width2, height> expect2(q);
					//pixmap2.px[0]=expect2.px[0]=flip(uint8(zx));
					//pixmap2.px[1]=expect2.px[1]=flip(uint8(qx));
					//pixmap2.px[2]=expect2.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap2.px+width2*2,width2, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect2.px+width2*2,width2, zx, expect2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap2,expect2);				
				}
			}			
		}		

		for(int zx=1;zx<=4;zx+=1) // mixed stride:		
		{						
			for(int qx=zx-1;qx<=zx+1;qx+=1)
			{
				for(int w=1,h=7; w<=7; w+=3) // copy down, z=even, q=odd
				{
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);
					Buffer<width2, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=7; w+=3) // copy up, z=even, q=odd
				{
					Buffer<width2, height> pixmap2(q);
					Buffer<width, height> pixmap(q);
					Buffer<width, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					copy_rect_ref<CD>(expect.px+width,width, zx, pixmap2.px+width2,width2, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=7; w+=3) // copy down, z=odd, q=even
				{
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);
					Buffer<width, height> pixmap2(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
				for(int w=1,h=7; w<=7; w+=3) // copy up, z=odd, q=even
				{
					Buffer<width, height> pixmap2(q);
					Buffer<width2, height> pixmap(q);
					Buffer<width2, height> expect(q);					
					//pixmap.px[0]=expect.px[0]=flip(uint8(zx));
					//pixmap.px[1]=expect.px[1]=flip(uint8(qx));
					//pixmap.px[2]=expect.px[2]=flip(uint8(w));
					
					copy_rect<CD>    (pixmap.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					copy_rect_ref<CD>(expect.px+width2,width2, zx, pixmap2.px+width,width, qx, w, h);
					CHECK_EQ(pixmap,expect);				
				}
			}			
		}		
	}
}

TEST_CASE("BitBlit: copy_row_as_1bpp")
{
	//
}

TEST_CASE("BitBlit: copy_rect_as_bitmap")
{
	//
}


#if 0 
TEST_CASE("")
{
	REQUIRE(1 == 1);
	CHECK(1 == 1);
	SUBCASE("") {}
}
#endif

/*
































*/
