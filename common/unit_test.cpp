// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "doctest.h"
#include "kilipili_common.h"

using namespace kio;

TEST_CASE("basic_math: min")
{
	static_assert(min(1, 2) == 1);
	static_assert(min(2, 1) == 1);
	static_assert(min(-1, 2) == -1);
	static_assert(min(1u, 2u) == 1);
	static_assert(min(1ll, 2ll) == 1);
	static_assert(min(1, 2, 3) == 1);
	static_assert(min(3, 2, 1) == 1);
}

TEST_CASE("basic_math: min")
{
	static_assert(max(1, 2) == 2);
	static_assert(max(2, 1) == 2);
	static_assert(max(-1, 2) == 2);
	static_assert(max(1u, 2u) == 2);
	static_assert(max(1ll, 2ll) == 2);
	static_assert(max(1, 2, 3) == 3);
	static_assert(max(3, 2, 1) == 3);
}

TEST_CASE("basic_math: sign, abs")
{
	static_assert(sign(2) == 1);
	static_assert(sign(0) == 0);
	static_assert(sign(-2) == -1);
	static_assert(sign(2ll) == 1);
	static_assert(sign(2ull) == 1);	 // no warning?
	static_assert(kio::abs(0) == 0); // stdlib has a non-constexpr abs(int)
	static_assert(kio::abs(2) == 2);
	static_assert(kio::abs(-2) == 2);
	static_assert(abs(0ll) == 0);
	static_assert(abs(2ll) == 2);
	static_assert(abs(-2ll) == 2);
	static_assert(abs(2ull) == 2); // no warning?
}

TEST_CASE("basic_math: minmax, limit")
{
	static_assert(minmax(4, 5, 6) == 5);
	static_assert(minmax(4, 3, 6) == 4);
	static_assert(minmax(4, 7, 6) == 6);

	int n = 5;
	limit(4, n, 6);
	CHECK(n == 5);
	limit(3, n, 4);
	CHECK(n == 4);
	limit(7, n, 9);
	CHECK(n == 7);
	limit(5, n, 5);
	CHECK(n == 5);
	limit(7, n, 7);
	CHECK(n == 7);
}

TEST_CASE("basic_text: hex char value")
{
	static_assert(is_hex_digit('0'));
	static_assert(is_hex_digit('9'));
	static_assert(is_hex_digit('A'));
	static_assert(is_hex_digit('a'));
	static_assert(is_hex_digit('F'));
	static_assert(is_hex_digit('f'));
	static_assert(!is_hex_digit('0' - 1));
	static_assert(!is_hex_digit('9' + 1));
	static_assert(!is_hex_digit('A' - 1));
	static_assert(!is_hex_digit('a' - 1));
	static_assert(!is_hex_digit('F' + 1));
	static_assert(!is_hex_digit('f' + 1));

	static_assert(hex_digit_value('0') == 0);
	static_assert(hex_digit_value('9') == 9);
	static_assert(hex_digit_value('a') == 10);
	static_assert(hex_digit_value('A') == 10);
	static_assert(hex_digit_value('F') == 15);
	static_assert(hex_digit_value('Z') == 35);
	static_assert(hex_digit_value('z') == 35);
	static_assert(hex_digit_value('z' + 1) == 36);
}

TEST_CASE("cdefs: filenamefrompath")
{
	// though the function could be marked as constexpr it finally isn't:
	// this is a pitty because then the full path is still in all the panic() macros. shit.
	CHECK(strcmp(filenamefrompath(""), "") == 0);
	CHECK(strcmp(filenamefrompath("foo.bar"), "foo.bar") == 0);
	CHECK(strcmp(filenamefrompath("boo/foo.bar"), "foo.bar") == 0);
	CHECK(strcmp(filenamefrompath("/pub/dev/foo.bar"), "foo.bar") == 0);
	CHECK(strcmp(filenamefrompath(__FILE__), "unit_tests.cpp") == 0);
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
