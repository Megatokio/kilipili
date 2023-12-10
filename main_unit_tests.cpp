// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
//
#include "cdefs.h"


void panic(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	exit(2);
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
