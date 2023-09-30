// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Pixmap_wAttr.h"
#include "basic_math.h"
#include "doctest.h"
#include "stb/stb_image.h"
#include <memory>

using namespace kio::Graphics;
using namespace kio;

// clang-format off
#define ALL_PIXMAPS Bitmap, Pixmap_i2, Pixmap_i4, Pixmap_i8, Pixmap_rgb
#define ALL_PIXMAPa1\
	Pixmap<colormode_a1w1_i4>, \
	Pixmap<colormode_a1w1_i8>, \
	Pixmap<colormode_a1w1_rgb>, \
	Pixmap<colormode_a1w2_i4>, \
	Pixmap<colormode_a1w2_i8>, \
	Pixmap<colormode_a1w2_rgb>, \
	Pixmap<colormode_a1w4_i4>, \
	Pixmap<colormode_a1w4_i8>, \
	Pixmap<colormode_a1w4_rgb>, \
	Pixmap<colormode_a1w8_i4>, \
	Pixmap<colormode_a1w8_i8>, \
	Pixmap<colormode_a1w8_rgb> 
#define ALL_PIXMAPa2\
	Pixmap<colormode_a2w1_i4>, \
	Pixmap<colormode_a2w1_i8>, \
	Pixmap<colormode_a2w1_rgb>, \
	Pixmap<colormode_a2w2_i4>, \
	Pixmap<colormode_a2w2_i8>, \
	Pixmap<colormode_a2w2_rgb>, \
	Pixmap<colormode_a2w4_i4>, \
	Pixmap<colormode_a2w4_i8>, \
	Pixmap<colormode_a2w4_rgb>, \
	Pixmap<colormode_a2w8_i4>, \
	Pixmap<colormode_a2w8_i8>, \
	Pixmap<colormode_a2w8_rgb>
// clang-format on


template<typename PM>
void print_metrics(const PM& pm, cstr ident = "", cstr msg = "new Pixmap")
{
	printf("%s%s: %i*%i, %s\n", ident, msg, pm.width, pm.height, tostr(PM::colormode));
	printf("%s  attrheight = %i\n", ident, pm.attrheight);
	printf("%s  allocated  = %s\n", ident, pm.allocated ? "yes" : "no");
	printf("%s  bits_per_pixel = %i\n", ident, pm.bits_per_pixel);
	printf("%s  bits_per_color = %i\n", ident, pm.bits_per_color);
	printf("%s  row_offset = %i\n", ident, pm.row_offset);
	if constexpr (is_attribute_mode(PM::colormode))
	{
		print_metrics(pm.as_super(), "  ", "pixels");
		print_metrics(pm.attributes, "  ", "attributes");
	}
}

template<typename PM>
bool is_clear(const PM& pm, uint color)
{
	return true;
	constexpr ColorDepth CD = PM::colordepth;

	if (is_direct_color(PM::colormode) && pm.row_offset << 3 == pm.width << CD)
	{
		color = flood_filled_color<CD>(color);
		if (memcmp(pm.pixmap, &color, uint(min(pm.row_offset * pm.height, 4))) != 0) return false;
		return memcmp(pm.pixmap, pm.pixmap + 4, uint(pm.row_offset * pm.height - 4)) == 0;
	}
	else
	{
		color &= pixelmask<CD>;
		for (int y = 0; y < pm.height; y++)
			for (int x = 0; x < pm.width; x++)
				if (pm.get_color(x, y) != color) return false;
		return true;
	}
}

TEST_CASE_TEMPLATE("Pixmap(Size)", T, ALL_PIXMAPS)
{
	constexpr ColorDepth CD				= T::CD;
	static constexpr int bits_per_pixel = 1 << CD;

	for (int width = 80; width <= 81; width++)
		for (int height = 40; height <= 41; height++)
		{
			T		pm(Size(width, height), attrheight_12px);
			Canvas& canvas = pm;

			CHECK_EQ(T::calc_row_offset(width), (width * bits_per_pixel + 7) / 8);

			CHECK_EQ(canvas.size, Size(width, height));
			CHECK_EQ(canvas.colormode, T::colormode);
			CHECK_EQ(canvas.attrheight, attrheight_none);
			CHECK_EQ(canvas.allocated, true);

			CHECK_EQ(pm.row_offset, T::calc_row_offset(width));
			CHECK_NE(pm.pixmap, nullptr);
		}
}

TEST_CASE_TEMPLATE("Pixmap_wAttr(Size) constructor", T, ALL_PIXMAPa1, ALL_PIXMAPa2)
{
	constexpr ColorMode	 CM				 = T::colormode;
	constexpr ColorDepth CD				 = T::CD;
	constexpr AttrMode	 AM				 = T::AM;
	constexpr AttrWidth	 AW				 = T::AW;
	static constexpr int bits_per_color	 = 1 << CD;				// 1 .. 16 bits per color in attributes[]
	static constexpr int bits_per_pixel	 = 1 << AM;				// 1 .. 2  bits per pixel in pixmap[]
	static constexpr int colors_per_attr = 1 << bits_per_pixel; // 2 .. 4
	static constexpr int pixel_per_attr	 = 1 << AW;

	static_assert(CD == get_colordepth(CM));
	static_assert(AM == get_attrmode(CM));
	static_assert(AW == get_attrwidth(CM));
	static_assert(CM == calc_colormode(AM, AW, CD));

	for (int width = 80; width <= 81; width++)
		for (int height = 40; height <= 41; height++)
		{
			T		pm(Size(width, height), attrheight_12px);
			Canvas* canvas = &pm;

			CHECK_EQ(canvas->size, Size(width, height));
			CHECK_EQ(canvas->colormode, CM);
			CHECK_EQ(canvas->attrheight, attrheight_12px);
			CHECK_EQ(canvas->allocated, true);

			CHECK_EQ(pm.row_offset, (width * bits_per_pixel + 7) / 8);
			CHECK_NE(pm.pixmap, nullptr);

			CHECK_EQ(T::calc_row_offset(width), (width * bits_per_pixel + 7) / 8);
			CHECK_EQ(T::calc_attr_width(width), (width + pixel_per_attr - 1) / pixel_per_attr * colors_per_attr);
			CHECK_EQ(T::calc_attr_row_offset(width), (T::calc_attr_width(width) * bits_per_color + 7) / 8);
			CHECK_EQ(T::calc_attr_height(height, pm.attrheight), (height + pm.attrheight - 1) / pm.attrheight);

			CHECK_EQ(pm.row_offset, T::calc_row_offset(width));
			CHECK_EQ(pm.attributes.width, T::calc_attr_width(width));
			CHECK_EQ(pm.attributes.height, T::calc_attr_height(height, pm.attrheight));
			CHECK_EQ(pm.attributes.colormode, calc_colormode(attrmode_none, attrwidth_none, CD));
			CHECK_EQ(pm.attributes.attrheight, attrheight_none);
			CHECK_EQ(pm.attributes.allocated, true);
			CHECK_EQ(pm.attributes.row_offset, T::calc_attr_row_offset(width));
			CHECK_NE(pm.attributes.pixmap, nullptr);
		}
}

TEST_CASE_TEMPLATE("Pixmap(Size,pixels[]) constructor", T, ALL_PIXMAPS)
{
	for (int width = 80; width <= 81; width++)
		for (int height = 40; height <= 41; height++)
		{
			const int				 row_offset = T::calc_row_offset(width) + 4;
			std::unique_ptr<uchar[]> pixels(new uchar[uint(width * row_offset)]);
			T						 pm(Size(width, height), pixels.get(), row_offset);
			Canvas&					 canvas = pm;

			CHECK_EQ(canvas.size, Size(width, height));
			CHECK_EQ(canvas.colormode, T::colormode);
			CHECK_EQ(canvas.attrheight, attrheight_none);
			CHECK_EQ(canvas.allocated, false);

			CHECK_EQ(pm.row_offset, row_offset);
			CHECK_EQ(pm.pixmap, pixels.get());
		}
}

TEST_CASE_TEMPLATE("Pixmap(Size,pixels[],attr[]) constructor", T, ALL_PIXMAPa1, ALL_PIXMAPa2)
{
	for (int width = 80; width <= 81; width++)
		for (int height = 40; height <= 41; height++)
		{
			const AttrHeight		 attrheight		 = attrheight_8px;
			const int				 row_offset		 = T::calc_row_offset(width) + 4;
			const int				 attr_row_offset = T::calc_attr_row_offset(width) + 4;
			std::unique_ptr<uchar[]> pixels(new uchar[uint(height * row_offset)]);
			std::unique_ptr<uchar[]> attributes(new uchar[uint(height * attr_row_offset)]);
			T		pm(Size(width, height), pixels.get(), row_offset, attributes.get(), attr_row_offset, attrheight);
			Canvas& canvas = pm;

			CHECK_EQ(canvas.size, Size(width, height));
			CHECK_EQ(canvas.colormode, T::colormode);
			CHECK_EQ(canvas.attrheight, attrheight);
			CHECK_EQ(canvas.allocated, false);

			CHECK_EQ(pm.row_offset, row_offset);
			CHECK_EQ(pm.pixmap, pixels.get());
		}
}

TEST_CASE_TEMPLATE("Pixmap::clear()", T, ALL_PIXMAPS, ALL_PIXMAPa1, ALL_PIXMAPa2)
{
	T pm1 {80, 40, attrheight_8px};
	pm1.clear(0, 0);
	CHECK(is_clear(pm1, 0));

	T pm2 {81, 40, attrheight_8px};
	pm2.clear(0, 0);
	CHECK(is_clear(pm2, 0));

	T pm3 {80, 41, attrheight_8px};
	pm3.clear(0, 0);
	CHECK(is_clear(pm3, 0));

	T pm4 {40, 80, attrheight_8px};
	pm4.clear(0, 0);
	CHECK(is_clear(pm4, 0));

	T pm5 {41, 80, attrheight_8px};
	pm5.clear(0, 0);
	CHECK(is_clear(pm5, 0));

	T pm6 {40, 81, attrheight_8px};
	pm6.clear(0, 0);
	CHECK(is_clear(pm6, 0));
}

TEST_CASE_TEMPLATE("Pixmap(Pixmap,Rect) constructor", T, ALL_PIXMAPa1, ALL_PIXMAPa2, ALL_PIXMAPS)
{
	for (int width = 128; width < 129; width++)
	{
		constexpr int	 BPP	= T::bits_per_pixel;
		const int		 height = 100;
		const AttrHeight ah		= attrheight_8px;
		const int		 x0 = 8, y0 = ah;
		const int		 w = 100, h = 80;

		T pm1 {width, height, ah};
		T pm2 {pm1, x0, y0, w, h};

		CHECK_EQ(pm2.width, w);
		CHECK_EQ(pm2.height, h);
		CHECK_EQ(pm2.colormode, pm1.colormode);
		CHECK_EQ(pm2.attrheight, pm1.attrheight);
		CHECK_EQ(pm2.allocated, false);
		CHECK_EQ(pm2.pixmap, pm1.pixmap + y0 * pm1.row_offset + (x0 / 8 * BPP));
		CHECK_EQ(pm2.row_offset, pm1.row_offset);
		CHECK_EQ(pm2.Canvas::colormode, pm1.colormode);

		pm1.clear(0);
		CHECK_EQ(is_clear(pm2, 0), true);

		for (int x = 0, y = 0; x < w && y < h; x += w / 11, y += h / 11)
		{
			pm2.set_pixel(x, y, 1);
			CHECK_EQ(pm1.get_color(x0 + x, y0 + y), 1);
			pm1.setPixel(x0 + x, y0 + y, 0);
			CHECK_EQ(pm2.get_color(x, y), 0);
		}

		CHECK_EQ(is_clear(pm1, 0), true);
	}
}

TEST_CASE_TEMPLATE("Pixmap::operator==() direct color", T, ALL_PIXMAPS)
{
	const int		 width = 55, height = 77;
	const AttrHeight ah = attrheight_8px;
	const int		 x0 = 8, y0 = ah;

	T pm1 {x0 + width + 1, y0 + height + 1, ah};
	T pm2 {pm1, x0, y0, width, height};
	T pm3 {width, height, ah};

	for (uint ink = 0; ink <= 1; ink++)
	{
		pm1.clear(ink, ink);
		pm3.clear(ink, ink);
		CHECK_EQ(pm3, pm2);

		pm2.set_pixel(0, 0, !ink, !ink);
		CHECK(pm3 != pm2);
		pm2.set_pixel(0, 0, ink, ink);
		CHECK(pm3 == pm2);

		pm2.set_pixel(width - 1, height - 1, !ink, !ink);
		CHECK(pm3 != pm2);
		pm2.set_pixel(width - 1, height - 1, ink, ink);
		CHECK(pm3 == pm2);

		pm1.clear(!ink, !ink);
		pm2.clear(ink, ink);
		CHECK(pm3 == pm3);
		CHECK(pm2 == pm2);
		CHECK(pm3 == pm2);
	}
}

TEST_CASE_TEMPLATE("Pixmap::operator==() attribute modes", T, ALL_PIXMAPa1, ALL_PIXMAPa2)
{
	const int		 width = 55, height = 77;
	const AttrHeight ah = attrheight_8px;
	const int		 x0 = 8, y0 = ah;

	T pm1 {x0 + width + 1, y0 + height + 1, ah};
	T pm2 {pm1, x0, y0, width, height}; // window in pm1
	T pm3 {width, height, ah};

	uint num_inks = T::colors_per_attr;

	// clear() == fillRect()
	// --> sets only colors for ink in attr[] but operator==() compares whole attr[]
	// --> extra clear needed:
	// TODO: maybe we should clear the pixmaps in ctor?
	for (uint ink = 0; ink < num_inks; ink++)
	{
		pm1.clear(0, ink);
		pm3.clear(0, ink);
	}
	CHECK_EQ(pm2, pm3);

	for (uint ink = 0; ink < num_inks; ink++)
	{
		pm1.clear(0, ink);
		pm3.clear(0, ink);
		CHECK_EQ(pm2, pm3);

		pm2.set_pixel(0, 0, 1, ink);
		CHECK_NE(pm3, pm2);
		pm2.set_pixel(0, 0, 0, ink);
		CHECK_EQ(pm3, pm2);

		pm2.set_pixel(width - 1, height - 1, 1, ink);
		CHECK_EQ(pm2, pm3);
		pm2.set_pixel(width - 1, height - 1, 0, ink);
		CHECK_EQ(pm2, pm3);

		pm1.clear(1, ink);
		pm2.clear(0, ink);
		CHECK_EQ(pm2, pm3);
	}
}

TEST_CASE_TEMPLATE("Pixmap::set_pixel(), get_pixel()", T, ALL_PIXMAPS)
{
	T pm {200, 100};
	pm.clear(0, 0);

	Point pt[] = {{10, 12}, {20, 12}, {10, 24}, {111, 99}};

	for (uint i = 0; i < NELEM(pt); i++)
	{
		uint ink;
		CHECK_UNARY(pm.getPixel(pt[i], &ink) == 0 && ink == 0);
		pm.setPixel(pt[i], 1);
		CHECK_UNARY(pm.getPixel(pt[i], &ink) == 1 && ink == 1);
		pm.setPixel(pt[i], 0);
		CHECK_UNARY(pm.getPixel(pt[i], &ink) == 0 && ink == 0);
	}
	CHECK(is_clear(pm, 0));
}

TEST_CASE_TEMPLATE("Pixmap::draw_hline_to()", T, ALL_PIXMAPS)
{
	// TODO
}

TEST_CASE_TEMPLATE("Pixmap::draw_vline_to()", T, ALL_PIXMAPS)
{
	// TODO
}

TEST_CASE_TEMPLATE("Pixmap::fillRect()", T, ALL_PIXMAPS)
{
	// TODO
}

TEST_CASE_TEMPLATE("Pixmap::xorRect()", T, ALL_PIXMAPS)
{
	// TODO
}

TEST_CASE_TEMPLATE("Pixmap::copyRect()", T, ALL_PIXMAPS)
{
	// TODO
}

TEST_CASE_TEMPLATE("Pixmap::drawBmp()", T, ALL_PIXMAPS)
{
	// TODO
}

TEST_CASE_TEMPLATE("Pixmap::drawChar()", T, ALL_PIXMAPS)
{
	// TODO
}


#if 0 
// c&p template:

TEST_CASE("")
{
	REQUIRE(1 == 1);
	CHECK(1 == 1);
	SUBCASE("") {}
}

#endif


/*








































*/
