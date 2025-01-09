// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Canvas.h"
#include "Pixmap.h"
#include "basic_math.h"
#include "doctest.h"
#include <memory>

using namespace kio::Graphics;
using namespace kio;


TEST_CASE("Canvas: is_inside()") { CHECK(1 == 1); }

TEST_CASE("Canvas: setPixel(), getPixel(), getColor(), getInk()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawHLine()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawVLine()") { CHECK(1 == 1); }

TEST_CASE("Canvas: fillRect()") { CHECK(1 == 1); }

TEST_CASE("Canvas: xorRect()") { CHECK(1 == 1); }

TEST_CASE("Canvas: clear()") { CHECK(1 == 1); }

TEST_CASE("Canvas: copyRect() inside Pixmap") { CHECK(1 == 1); }

TEST_CASE("Canvas: copyRect() between Pixmaps") { CHECK(1 == 1); }

TEST_CASE("Canvas: readBmp()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawBmp()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawChar()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawLine()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawRect()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawCircle()") { CHECK(1 == 1); }

TEST_CASE("Canvas: fillCircle()") { CHECK(1 == 1); }

TEST_CASE("Canvas: floodFill()") { CHECK(1 == 1); }

TEST_CASE("Canvas: drawPolygon()") { CHECK(1 == 1); }


#if 0 
TEST_CASE("")
{
	REQUIRE(1 == 1);
	CHECK(1 == 1);
	SUBCASE("") {}
}
#endif
