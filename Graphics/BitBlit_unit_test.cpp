// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "BitBlit.h"
#include "basic_math.h"
#include "doctest.h"
#include "stb/stb_image.h"
#include <memory>

using namespace kio::Graphics::bitblit;
using namespace kio::Graphics;
using namespace kio;


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

TEST_CASE("BitBlit: clear_row_of_bits_with_mask")
{
	//
}

TEST_CASE("BitBlit: clear_rect_of_bits")
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

TEST_CASE("BitBlit: get_pixel")
{
	//
}

TEST_CASE("BitBlit: set_pixel")
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

TEST_CASE("BitBlit: attr_draw_hline")
{
	//
}

TEST_CASE("BitBlit: attr_clear_rect")
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

TEST_CASE("BitBlit: compare_row")
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
