// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BitBlit.h"
#include "basic_math.h"
#include "doctest.h"
#include "stb/stb_image.h"
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
	// clear_rect_of_bits(uint8* zp, int xoffs, int row_offset, int width, int height, uint32 color)
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

		clear_rect_of_bits(pixmap.px, 8*width + 32, width, 64, 2, flip(0xACACACACu));
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

		clear_rect_of_bits(pixmap.px, 8*width + x0, width, w, h, flip(0xACACACACu));
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

		clear_rect_of_bits(pixmap.px, 8*width + x0, width, w, h, flip(0x55555555u));
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

		clear_rect_of_bits(pixmap.px, 8*width + x0, width, w, h, flip(0x66666666u));
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

		clear_rect_of_bits(pixmap.px, 8*width + x0, width, w, h, flip(0x66666666u));
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

		clear_rect_of_bits(pixmap.px, 8*width + x0, width, 0, 2, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, 8*width + x0, width, 33, 0, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, 8*width + x0, width, 33, -1, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, 8*width + x0, width, -1, 2, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, 8*width + x0, width, 33, -999, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits(pixmap.px, 8*width + x0, width, -999, 2, flip(0x66666666u));
		CHECK_EQ(pixmap, expect);
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

		clear_rect_of_bits_with_mask(pixmap.px, x0, 0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
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

		clear_rect_of_bits_with_mask(pixmap.px, x0, 0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
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

		clear_rect_of_bits_with_mask(pixmap.px, x0, 0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
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

		clear_rect_of_bits_with_mask(pixmap.px+1, x0-8, 0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
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

		clear_rect_of_bits_with_mask(pixmap.px, x0, 0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
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

		clear_rect_of_bits_with_mask(pixmap.px, x0, 0, w, h, flip(0x55555555u),flip(0xf0f0f0f0u));
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

		clear_rect_of_bits_with_mask(pixmap.px, 8*width + x0, width, 0, 2, 0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, 8*width + x0, width, 33, 0,  0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, 8*width + x0, width, 33, -1, 0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, 8*width + x0, width, -1, 2,  0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);		
		clear_rect_of_bits_with_mask(pixmap.px, 8*width + x0, width, 33, -999, 0xffffffffu,0x55555555u);
		CHECK_EQ(pixmap, expect);
		clear_rect_of_bits_with_mask(pixmap.px, 8*width + x0, width, -999, 2,  0xffffffffu,0x55555555u);
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




TEST_CASE("BitBlit: clear_row: uint8*, uint16*, uint32*")
{
	//
}

TEST_CASE("BitBlit: clear_row_of_bits")
{
	//
}

TEST_CASE("BitBlit: xor_row_of_bits")
{
	//
}

TEST_CASE("BitBlit: clear_rect_of_bytes")
{
	//
}

TEST_CASE("BitBlit: xor_rect_of_bits")
{
	//
}

TEST_CASE("BitBlit: xor_rect_of_bytes")
{
	//
}

TEST_CASE("BitBlit: copy_rect_of_bits")
{
	//
}

TEST_CASE("BitBlit: copy_rect_of_bytes")
{
	//
}

TEST_CASE("BitBlit: draw_vline")
{
	//
}

TEST_CASE("BitBlit: draw_hline")
{
	//
}

TEST_CASE("BitBlit: copy_rect")
{
	//
}

TEST_CASE("BitBlit: draw_bmp_1bpp draw_bmp_2bpp draw_bmp_4bpp draw_bmp_8bpp draw_bmp_16bpp")
{
	//
}

TEST_CASE("BitBlit: draw_chr_1bpp draw_chr_2bpp draw_chr_4bpp")
{
	//
}

TEST_CASE("BitBlit: draw_bitmap")
{
	//
}

TEST_CASE("BitBlit: draw_char")
{
	//
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
