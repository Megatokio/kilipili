// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
//
#include "Graphics/graphics_types.h"
#include "common/Array.h"
#include "common/cdefs.h"


void panic(const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);
	vprintf(fmt, va);
	exit(2);
}

namespace kio
{
doctest::String toString(Array<cstr> log)
{
	if (log.count() == 0) return "{<empty>}";
	cstr s = log[0];
	for (uint i = 1; i < log.count(); i++) { s = catstr(s, ",\n", log[i]); }
	return catstr("{\n", s, "}");
}
} // namespace kio

namespace kio::Graphics
{
doctest::String toString(ColorMode cm)
{
	return tostr(cm); //
}
} // namespace kio::Graphics


#if 0 
// c&p template:

TEST_CASE("")
{
	REQUIRE(1 == 1);
	CHECK(1 == 1);
	SUBCASE("") {}
}

#endif
