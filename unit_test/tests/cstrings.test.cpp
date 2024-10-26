// Copyright (c) 2018 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#undef NDEBUG
#define SAFETY	 2
#define LOGLEVEL 1
#include "cstrings.h"
#include "Array.h"
#include "cstrings.h"
#include "doctest.h"
//#include "ucs1.h"
//#include "unix/FD.h"
//#include "utf8.h"


using namespace kio;


TEST_CASE("cstrings: emptystr")
{
	CHECK(eq(emptystr, "")); //
}

TEST_CASE("cstrings: is_space")
{
	CHECK(is_space(' '));
	char c = 0;
	do {
		CHECK(is_space(c) == (c <= ' ' && c > 0));
	}
	while (++c);
}

TEST_CASE("cstrings: is_uppercase")
{
	CHECK(is_uppercase('A'));
	char c = 0;
	do {
		CHECK(is_uppercase(c) == (c >= 'A' && c <= 'Z'));
	}
	while (++c);
}

TEST_CASE("cstrings: is_lowercase")
{
	CHECK(is_lowercase('a'));
	char c = 0;
	do {
		CHECK(is_lowercase(c) == (c >= 'a' && c <= 'z'));
	}
	while (++c);
}

TEST_CASE("cstrings: is_letter")
{
	CHECK(is_letter('a'));
	char c = 0;
	do {
		CHECK(is_letter(c) == (to_upper(c) >= 'A' && to_upper(c) <= 'Z'));
	}
	while (++c);
}

TEST_CASE("cstrings: to_lower")
{
	CHECK(to_lower('A') == 'a');
	char c = 0;
	do {
		CHECK_EQ(to_lower(c), ((c >= 'A' && c <= 'Z') ? c + 32 : c));
	}
	while (++c);
}

TEST_CASE("cstrings: to_upper")
{
	CHECK_EQ(to_upper('a'), 'A');
	char c = 0;
	do {
		CHECK_EQ(to_upper(c), ((c >= 'a' && c <= 'z') ? c - 32 : c));
	}
	while (++c);
}

TEST_CASE("cstrings: is_bin_digit")
{
	CHECK(is_bin_digit('0'));
	char c = 0;
	do {
		CHECK_EQ(is_bin_digit(c), (c == '0' || c == '1'));
	}
	while (++c);
}

TEST_CASE("cstrings: is_oct_digit")
{
	CHECK(is_oct_digit('0'));
	char c = 0;
	do {
		CHECK_EQ(is_oct_digit(c), (c >= '0' && c <= '7'));
	}
	while (++c);
}

TEST_CASE("cstrings: is_decimal_digit")
{
	CHECK(is_decimal_digit('0'));
	char c = 0;
	do {
		CHECK_EQ(is_decimal_digit(c), (c >= '0' && c <= '9'));
	}
	while (++c);
}

TEST_CASE("cstrings: is_hex_digit")
{
	CHECK(is_hex_digit('0'));
	char c = 0;
	do {
		CHECK_EQ(is_hex_digit(c), ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')));
	}
	while (++c);
}

TEST_CASE("cstrings: no_xxx_digit")
{
	for (char c = 77; ++c != 77;)
	{
		CHECK_NE(is_bin_digit(c), no_bin_digit(c));
		CHECK_NE(is_decimal_digit(c), no_dec_digit(c));
		CHECK_NE(is_hex_digit(c), no_hex_digit(c));
		CHECK_NE(is_oct_digit(c), no_oct_digit(c));
	}
}

TEST_CASE("cstrings: dec_digit_value")
{
	CHECK(dec_digit_value(('0')) == 0); //
}

TEST_CASE("cstrings: hex_digit_value")
{
	CHECK_EQ(hex_digit_value(('0')), 0);
	CHECK_EQ(hex_digit_value(('9')), 9);
	CHECK_EQ(hex_digit_value(('A')), 10);
	CHECK_EQ(hex_digit_value(('a')), 10);
	CHECK_EQ(hex_digit_value(('Z')), 35);
	CHECK_EQ(hex_digit_value(('z')), 35);
	CHECK_GE(hex_digit_value(('0' - 1)), 36);
	CHECK_GE(hex_digit_value(('9' + 1)), 36);
	CHECK_GE(hex_digit_value(('a' - 1)), 36);
	CHECK_GE(hex_digit_value(('z' + 1)), 36);
	CHECK_GE(hex_digit_value(('A' - 1)), 36);
	CHECK_GE(hex_digit_value(('Z' + 1)), 36);
	char c = 0;
	do {
		CHECK_EQ((hex_digit_value(c) >= 36), (no_dec_digit(c) && !is_letter(c)));
	}
	while (++c);
}

TEST_CASE("cstrings: hexchar")
{
	CHECK_EQ(hexchar(0), '0');
	CHECK_EQ(hexchar(9), '9');
	CHECK_EQ(hexchar(10), 'A');
	CHECK_EQ(hexchar(15), 'F');
	CHECK_EQ(hexchar(16), '0');
}

TEST_CASE("cstrings: strlen")
{
	CHECK_EQ(kio::strlen("123"), 3); //
}

TEST_CASE("cstrings: lt")
{
	CHECK(lt("123", "23")); //
}

TEST_CASE("cstrings: gt")
{
	CHECK(gt("234", "1234")); //
}

TEST_CASE("cstrings: lcgt")
{
	CHECK(lcgt("big", "Apple")); //
}

TEST_CASE("cstrings: eq")
{
	CHECK(eq("123", "123"));	   //
	CHECK_FALSE(eq("123", "124")); //
}

TEST_CASE("cstrings: ne")
{
	CHECK_FALSE(ne("123", "123")); //
	CHECK(ne("123", "124"));	   //
}

TEST_CASE("cstrings: find(str)")
{
	char s[] = "asdfghJKlJKL";
	CHECK(find(s, "JK") == s + 6);
}

TEST_CASE("cstrings: rfind(str)")
{
	char s[] = "asdfghJKlJKL";
	CHECK(rfind(s, "JK") == s + 9);
}

TEST_CASE("cstrings: startswith")
{
	CHECK(startswith("Asdfg", "Asd")); //
}

TEST_CASE("cstrings: endswith")
{
	CHECK(endswith("Asdfg", "fg")); //
}

TEST_CASE("cstrings: contains")
{
	CHECK(contains("Asdfg", "sd")); //
}

TEST_CASE("cstrings: isupperstr")
{
	CHECK(isupperstr("ANTON_01;")); //
}

TEST_CASE("cstrings: islowerstr")
{
	CHECK(islowerstr("anton_01;")); //
}

TEST_CASE("cstrings: newstr")
{
	str s = newstr(20);
	CHECK(s[20] == 0);
	delete[] s;
}

TEST_CASE("cstrings: newcopy")
{
	str s = newcopy("123");
	CHECK(eq(s, "123"));
	delete[] s;
}

TEST_CASE("cstrings: tempstr")
{
	TempMem z;
	str		s1 = tempstr(20);
	CHECK(s1[20] == 0);
	str s2 = tempstr(1); //CHECK(s2 == s1-2);	// not required but expected
}

TEST_CASE("cstrings: tempstr")
{
	TempMem z;
	str		s1 = tempstr(20);
	CHECK(s1[20] == 0);
	TempMem z2;
	//cstr s2 = xtempstr(1); CHECK(s2 == s1-2);	// not required but expected
	cstr s2 = xdupstr("x");
}

TEST_CASE("cstrings: spacestr")
{
	CHECK(eq(spacestr(5, 'x'), "xxxxx")); //
}

TEST_CASE("cstrings: spaces")
{
	CHECK(eq(spaces(5), "     "));
	CHECK_EQ(spaces(5), spaces(10) + 5); // not required but expected: short strings are in static const
}

TEST_CASE("cstrings: whitestr")
{
	CHECK(eq(whitestr("\tFoo Bar\n"), "\t       \n")); //
}

TEST_CASE("cstrings: dupstr")
{
	static const char s1[4] = "123";
	CHECK(eq(s1, dupstr(s1)));
	CHECK(s1 != dupstr(s1));
}

TEST_CASE("cstrings: xdupstr")
{
	static const char s1[4] = "123";
	const char*		  s2;
	{
		TempMem z;
		s2 = dupstr(s1);
		s2 = xdupstr(s2);
		z.purge();
		(void)spacestr(66);
	}
	CHECK(eq(s1, s2));
	CHECK(s1 != s2);
}

TEST_CASE("cstrings: substr")
{
	static const cstr s = "12345678";
	CHECK(eq(substr(s + 2, s + 5), "345"));
}

TEST_CASE("cstrings: mulstr")
{
	CHECK(eq(mulstr("123", 4), "123123123123")); //
}

TEST_CASE("cstrings: catstr")
{
	CHECK(eq(catstr("123", "45"), "12345"));
	CHECK(eq(catstr("A", "n", "ton"), "Anton"));
	CHECK(eq(catstr("A", "n", "ton", "ov"), "Antonov"));
	CHECK(eq(catstr("A", "n", "ton", "ov", "_"), "Antonov_"));
	CHECK(eq(catstr("A", "n", "ton", "ov", " ", "78"), "Antonov 78"));
}

TEST_CASE("cstrings: midstr")
{
	CHECK(eq(midstr("Antonov", 2, 3), "ton"));
	CHECK(eq(midstr("Antonov", 2), "tonov"));
}

TEST_CASE("cstrings: leftstr")
{
	CHECK(eq(leftstr("Antonov", 3), "Ant")); //
}

TEST_CASE("cstrings: rightstr")
{
	CHECK(eq(rightstr("Antonov", 3), "nov")); //
}

TEST_CASE("cstrings: lastchar")
{
	CHECK(lastchar("123") == '3'); //
}

TEST_CASE("cstrings: toupper")
{
	str s = dupstr("AntoNov 123");
	toupper(s);
	CHECK(eq(s, "ANTONOV 123"));
}

TEST_CASE("cstrings: tolower")
{
	str s = dupstr("AntoNov 123");
	tolower(s);
	CHECK(eq(s, "antonov 123"));
}

TEST_CASE("cstrings: upperstr")
{
	CHECK(eq(upperstr("AnToNoV 123;"), "ANTONOV 123;")); //
}

TEST_CASE("cstrings: lowerstr")
{
	CHECK(eq(lowerstr("AnToNoV 123;"), "antonov 123;")); //
}

TEST_CASE("cstrings: replacedstr(char)")
{
	CHECK(eq(replacedstr("Beet", 'e', 'o'), "Boot")); //
}

TEST_CASE("cstrings: replacedstr(str)")
{
	CHECK(eq(replacedstr("minFooxyz", "Foo", "Bar"), "minBarxyz"));
	CHECK(eq(replacedstr("abcdef", "abc", "ABC"), "ABCdef"));
	CHECK(eq(replacedstr("abcdef", "def", "DEF"), "abcDEF"));
	CHECK(eq(replacedstr("abcdef", "abd", "fff"), "abcdef"));
	CHECK(eq(replacedstr("abcabcabcdef", "abc", "ABC"), "ABCABCABCdef"));
	CHECK(eq(replacedstr("111111111", "111", "2"), "222"));
	CHECK(eq(replacedstr("abcdef", "bcde", "X"), "aXf"));
	CHECK(eq(replacedstr("xyyyz", "y", "123"), "x123123123z"));
	CHECK(eq(replacedstr("", "f00", "xx"), ""));
	CHECK(eq(replacedstr("abcdef", "bc", "bc"), "abcdef"));
	CHECK(eq(replacedstr("abcdef", "BC", "bc"), "abcdef"));
	CHECK(eq(replacedstr("foooo", "foo", "f"), "foo"));
	CHECK(eq(replacedstr("abcdef", "cd", "xyz"), "abxyzef"));
}

TEST_CASE("cstrings: escapedstr")
{
	CHECK(eq(escapedstr("123\tabc\n"), "123\\tabc\\n")); //
}

TEST_CASE("cstrings: unescapedstr")
{
	CHECK(eq(unescapedstr("123\\tabc\\n"), "123\tabc\n")); //
}

TEST_CASE("cstrings: quotedstr")
{
	CHECK(eq(quotedstr("123\tabc\n"), "\"123\\tabc\\n\"")); //
}

TEST_CASE("cstrings: unquotedstr")
{
	CHECK(eq(unquotedstr("\"123\\tabc\\n\""), "123\tabc\n")); //
}

TEST_CASE("cstrings: tohtmlstr")
{
	CHECK(eq(tohtmlstr("a<int&>\n"), "a&lt;int&amp;&gt;<br>")); //
}

TEST_CASE("cstrings: fromhtmlstr")
{
	CHECK(eq(fromhtmlstr("a&lt;int&amp;&gt;<br>"), "a<int&>\n"));
	CHECK(eq(fromhtmlstr("foo<br>a&lt;int&amp;&gt;<br>"), "foo\na<int&>\n"));
}

TEST_CASE("cstrings: toutf8str")
{
	static const uchar s[] = {'a', 0xC4, 0xFF, 0};
	CHECK(kio::strlen(toutf8str(cstr(s))) == 5);
	CHECK(eq(toutf8str(cstr(s)), "aÄÿ"));
}

TEST_CASE("cstrings: fromutf8str")
{
	static const uchar s[] = {'a', 0xC4, 0xFF, 0};
	CHECK(eq(fromutf8str("aÄÿ"), cstr(s)));
}

TEST_CASE("cstrings: unhexstr")
{
	static const uchar s[] = {'a', 0xC4, 0xFF, ' ', '\n', 0};
	CHECK(eq(unhexstr("61C4ff200A"), cstr(s)));
}

//TEST_CASE("cstrings: "){
//	str uuencodedstr(cstr)
//}

//TEST_CASE("cstrings: "){
//	str uudecodedstr(cstr)
//}

TEST_CASE("cstrings: base64str")
{
	CHECK(eq(base64str("Die Fo01[]"), "RGllIEZvMDFbXQ=="));
	CHECK(eq(base64str("oDie Fo01[]"), "b0RpZSBGbzAxW10="));
	CHECK(eq(base64str("o Die Fo01[]"), "byBEaWUgRm8wMVtd"));
}

TEST_CASE("cstrings: unbase64str")
{
	CHECK(eq(unbase64str("byBEaWUgRm8wMVtd"), "o Die Fo01[]"));
	CHECK(eq(unbase64str("b0RpZSBGbzAxW10="), "oDie Fo01[]"));
	CHECK(eq(unbase64str("RGllIEZvMDFbXQ=="), "Die Fo01[]"));
}

TEST_CASE("cstrings: croppedstr")
{
	CHECK(eq(croppedstr("\t hallo\n"), "hallo")); //
}

TEST_CASE("cstrings: detabstr")
{
	CHECK(eq(detabstr(" \thallo\t\n", 4), "    hallo   \n")); //
}

TEST_CASE("cstrings: usingstr")
{
	CHECK(eq(usingstr("foo%s", "bar"), "foobar")); //
}

TEST_CASE("cstrings: tostr")
{
	CHECK(eq(tostr(1.5f), "1.5"));
	CHECK(eq(tostr(1.5), "1.5"));
	CHECK(eq(tostr(1.5L), "1.5"));
	CHECK(eq(tostr(uint64(12345)), "12345"));
	CHECK(eq(tostr(int64(-12345)), "-12345"));
	CHECK(eq(tostr(uint32(12345)), "12345"));
	CHECK(eq(tostr(int32(-12345)), "-12345"));
	CHECK(eq(tostr("foo"), "\"foo\""));
}

TEST_CASE("cstrings: binstr")
{
	CHECK(eq(binstr(0x2345), "01000101"));
	CHECK(eq(binstr(0x2345, "abcdefghijkl", "ABCDEFGHIJKL"), "abCDeFghiJkL"));
	CHECK(eq(binstr(0x2345, "0000.0000.0000", "1111.1111.1111"), "0011.0100.0101"));
	// 2019-10-06: binstr failed if b0str and b1str started with same char:
	CHECK(eq(binstr(0x5a, "0b00000000", "0b11111111"), "0b01011010"));
}

TEST_CASE("cstrings: hexstr")
{
	CHECK(eq(hexstr(0x2345, 8), "00002345"));
	CHECK(eq(hexstr(0x2345L, 8), "00002345"));
	CHECK(eq(hexstr(0xC3456789ABCDEF01L, 16), "C3456789ABCDEF01"));
	CHECK(eq(hexstr(0x2345LL, 8), "00002345"));
	CHECK(eq(hexstr(0xC3456789ABCDEF01LL, 16), "C3456789ABCDEF01"));
	CHECK(eq(hexstr(" 0Ab"), "20304162"));
}

TEST_CASE("cstrings: hexstr")
{
	{
		char n = 0x12;
		CHECK(eq(hexstr(n, 2), "12"));
	}
	{
		uchar n = 0x12;
		CHECK(eq(hexstr(n, 2), "12"));
	}
	{
		schar n = 0x12;
		CHECK(eq(hexstr(n, 2), "12"));
	}
	{
		short n = 0x123;
		CHECK(eq(hexstr(n, 4), "0123"));
	}
	{
		ushort n = 0x123;
		CHECK(eq(hexstr(n, 4), "0123"));
	}
	{
		int n = 0x123456;
		CHECK(eq(hexstr(n, 6), "123456"));
	}
	{
		uint n = 0x123456;
		CHECK(eq(hexstr(n, 6), "123456"));
	}
#if ULONG_WIDTH > 4
	{
		long n = 0x123456789;
		CHECK(eq(hexstr(n, 10), "0123456789"));
	}
	{
		ulong n = 0x123456789;
		CHECK(eq(hexstr(n, 10), "0123456789"));
	}
#else
	{
		long n = 0x12345678;
		CHECK(eq(hexstr(n, 10), "012345678"));
	}
	{
		ulong n = 0x12345678;
		CHECK(eq(hexstr(n, 10), "012345678"));
	}
#endif
	{
		llong n = 0x123456789;
		CHECK(eq(hexstr(n, 10), "0123456789"));
	}
	{
		ullong n = 0x123456789;
		CHECK(eq(hexstr(n, 10), "0123456789"));
	}

	{
		int8 n = 0x12;
		CHECK(eq(hexstr(n, 2), "12"));
	}
	{
		uint8 n = 0x12;
		CHECK(eq(hexstr(n, 2), "12"));
	}
	{
		int16 n = 0x123;
		CHECK(eq(hexstr(n, 4), "0123"));
	}
	{
		uint16 n = 0x123;
		CHECK(eq(hexstr(n, 4), "0123"));
	}
	{
		int32 n = 0x123456;
		CHECK(eq(hexstr(n, 6), "123456"));
	}
	{
		uint32 n = 0x123456;
		CHECK(eq(hexstr(n, 6), "123456"));
	}
	{
		int64 n = 0x123456789;
		CHECK(eq(hexstr(n, 10), "0123456789"));
	}
	{
		uint64 n = 0x123456789;
		CHECK(eq(hexstr(n, 10), "0123456789"));
	}

	{
		enum Foo : ushort { a, b, c, foo = 0x123 };
		CHECK(eq(hexstr(foo, 4), "0123"));
	}
	{
		enum Foo : uint { a, b, c, foo = 0x123 };
		CHECK(eq(hexstr(foo, 4), "0123"));
	}
	{
		enum Foo : ulong { a, b, c, foo = 0x123 };
		CHECK(eq(hexstr(foo, 4), "0123"));
	}
}

TEST_CASE("cstrings: charstr")
{
	CHECK(eq(charstr('c'), "c"));
	CHECK(eq(charstr('A', 'B'), "AB"));
	CHECK(eq(charstr('A', 'B', 'C'), "ABC"));
	CHECK(eq(charstr('A', 'B', 'C', 'D'), "ABCD"));
	CHECK(eq(charstr('A', 'B', 'C', 'D', 'E'), "ABCDE"));
}

TEST_CASE("cstrings: datestr, datetimestr")
{
	time_t m	 = 60;
	time_t h	 = 60 * m;
	time_t d	 = 24 * h;
	time_t y	 = 365 * d;
	time_t y1970 = 0;
	time_t y1980 = y1970 + 10 * y + 2 * d;
	time_t y1990 = y1980 + 10 * y + 3 * d;
	time_t y2000 = y1990 + 10 * y + 2 * d;
	time_t y2010 = y2000 + 10 * y + 3 * d;
	time_t jan = 0, feb = jan + 31 * d, mrz = feb + 28 * d, apr = mrz + 31 * d, mai = apr + 30 * d, jun = mai + 31 * d,
		   jul = jun + 30 * d, aug = jul + 31 * d, sep = aug + 31 * d, okt = sep + 30 * d, nov = okt + 31 * d,
		   dez = nov + 30 * d;

	// These tests fail if your timezone ist not CET/CEST:
	CHECK(eq(datestr(31 * d), "1970-02-01"));
	CHECK(eq(datestr(31 * d - 1), "1970-02-01"));
	CHECK(eq(datestr(31 * d - 1 * h), "1970-02-01")); // CET is 1h ahead
	CHECK(eq(datestr(31 * d - 2 * h - 1), "1970-01-31"));
	CHECK(eq(timestr(31 * d + 45 * m + 21), "01:45:21"));
	CHECK(eq(datetimestr(y1970 + feb + 45 * m + 21), "1970-02-01 01:45:21"));
	CHECK(eq(datetimestr(y1990 + feb + 45 * m + 21), "1990-02-01 01:45:21"));
	CHECK(eq(datetimestr(y1990 + feb + 45 * m + 21), "1990-02-01 01:45:21"));
	CHECK(eq(datetimestr(y2000 + feb + 45 * m + 21), "2000-02-01 01:45:21"));
	CHECK(eq(datetimestr(y2000 + feb + 28 * d + 45 * m + 21), "2000-02-29 01:45:21"));
	CHECK(eq(datetimestr(y2000 + jun + 1 * d + 28 * d - 1 * h + 5 * m + 21), "2000-06-29 01:05:21"));
	CHECK(eq(datetimestr(y2010 + dez + 23 * d - 1 * h + 18 * h + 30 * m + 0), "2010-12-24 18:30:00"));
	CHECK(eq(datetimestr(y2010 + jan - 1 * h - 1), "2009-12-31 23:59:59"));
}

TEST_CASE("cstrings: dateval")
{
	// This test fails if your timezone ist not CET:
	CHECK(dateval("1970-02-01 01:45:21") == 31 * 24 * 60 * 60 + 45 * 60 + 21);

	CHECK(dateval("19991231") == dateval("1999-12-31 0:0:0"));
	CHECK(dateval("99-12-31") == dateval("1999-12-31 0:0"));
	CHECK(dateval("20-4-7") == dateval("2020-04-07 0"));
	CHECK(dateval("'2020-04-12 12:34:56'") == dateval("2020-04-12 12:34:56"));
	CHECK(dateval("day=2020-4-12, time=12:34:56") == dateval("2020-04-12 12:34:56 // foo comment"));

	for (int i = 0; i < 99; i++)
	{
		time_t n = random();
		CHECK(dateval(datetimestr(n)) == n);
	}
}

TEST_CASE("cstrings: durationstr")
{
	time_t m = 60;
	time_t h = 60 * m;
	time_t d = 24 * h;
	CHECK(eq(durationstr(123), "123 sec."));
	CHECK(eq(durationstr(1230), "20m:30s"));
	CHECK(eq(durationstr(3 * h + 5 * m + 33), "3h:05m:33s"));
	CHECK(eq(durationstr(2 * d + 11 * h + 15 * m + 33), "2d:11h:15m"));
	CHECK(eq(durationstr(1230.1), "20m:30s"));
	CHECK(eq(durationstr(123.145), "123.145 sec."));
	CHECK(eq(durationstr(66.1), "66.100 sec."));
}

TEST_CASE("cstrings: split")
{
	char	   s1[] = "Die Kuh lief um den Teich herum zum Wasser.\n";
	Array<str> array;
	split(array, s1, ' ');
	CHECK(array.count() == 9);
	CHECK(eq(array[0], "Die"));
	CHECK(eq(array[8], "Wasser.\n"));

	char s2[] = "Die Kuh\nlief um\rden Teich\n\rherum zum\r\nWasser.\n";
	split(array, s2);
	CHECK(array.count() == 5);
	CHECK(eq(array[0], "Die Kuh"));
	CHECK(eq(array[4], "Wasser."));

	char s3[] = "";
	split(array, s3, int(0));
	CHECK(array.count() == 0);
}

TEST_CASE("cstrings: strcpy")
{
	str s = dupstr("123");
	strcpy(s, "abc", 0);
	CHECK(eq(s, "123"));
	strcpy(s, "abc", 1);
	CHECK(eq(s, ""));
	strcpy(s, "abc", 3);
	CHECK(eq(s, "ab"));
	strcpy(s, "ABC", 4);
	CHECK(eq(s, "ABC"));
	strcpy(s, "", 99);
	CHECK(eq(s, ""));
	strcpy(s, nullptr, 99);
	CHECK(eq(s, ""));
}

TEST_CASE("cstrings: strcat")
{
	str s = tempstr(99);
	strcpy(s, "123", 99);
	CHECK(eq(s, "123"));
	strcat(s, "456", 99);
	CHECK(eq(s, "123456"));
	s[8] = 'x';
	s[9] = 0;
	strcat(s, "789", 8);
	CHECK(eq(s, "1234567"));
}

#if UCS1
TEST_CASE("cstrings: ")
{
	CHECK(ucs1::is_uppercase('A'));
	CHECK(ucs1::is_uppercase(0xc1));
	CHECK(!ucs1::is_uppercase(0));
	ucs1char c = 0;
	do {
		CHECK(ucs1::is_uppercase(c) == ((c >= 'A' && c <= 'Z') || (c >= 0xc0 && c <= 0xde && c != 0xd7)));
	}
	while (++c);
}

TEST_CASE("cstrings: ")
{
	CHECK(ucs1::is_lowercase('a'));
	CHECK(ucs1::is_lowercase(0xe1));
	ucs1char c = 0;
	do {
		CHECK(
			ucs1::is_lowercase(c) == ((c >= 'a' && c <= 'z') || (c >= 0xdf && c <= 0xff && c != 0xf7)) ||
			c == 0xb5 /*µ*/);
	}
	while (++c);
}
#endif

#if UTF8
TEST_CASE("cstrings: ")
{
	errno = 0;
	for (int i = 0; i <= 255; i++)
	{
		CHECK(utf8::is_fup(char(i)) == (i >= 0x80 && i < 0xC0));
		CHECK(utf8::no_fup(char(i)) != (i >= 0x80 && i < 0xC0));
	}
	CHECK(utf8::charcount("\t1hGfr&%<'") == 10);
	CHECK(utf8::charcount("ß1≥€•@∆º¶§") == 10);
	CHECK(utf8::max_csz("\t1hGfr&%<'") == 1);
	CHECK(utf8::max_csz("ß2≥€•@∆º¶§") == 2);

	ucs4char c1 = 0x09999;
	ucs4char c2 = 0x10000;
	CHECK(utf8::max_csz(catstr("\t1hGf∆º¶§", utf8::to_utf8str(&c1, 1), "x")) == 2);
	CHECK(utf8::max_csz(catstr("\t1hGf∆º¶§", utf8::to_utf8str(&c2, 1), "x")) == 4);

	CHECK(utf8::fits_in_ucs1("\t1hGfr&%<'"));
	CHECK(!utf8::fits_in_ucs1("ß≤4€•@∆º¶§"));
	CHECK(utf8::fits_in_ucs2("ß3≥€•@∆º¶§"));
	CHECK(!utf8::fits_in_ucs2(catstr("\t1hGfr&%<'", utf8::to_utf8str(&c2, 1))));
	CHECK(errno == 0);
}

TEST_CASE("cstrings: ")
{
	static const char s1[] = "\t1hGfr&%<'";
	static const char s2[] = "ß≤5€•@∆º¶§";

	errno = 0;
	uint8 bu1[20], *e1 = utf8::utf8_to_ucs1(s1, bu1);
	CHECK(utf8::utf8strlen(bu1, uint(e1 - bu1)) == NELEM(s1) - 1);
	CHECK(errno == 0);
	uint16 bu2[20], *e2 = utf8::utf8_to_ucs2(s2, bu2);
	CHECK(utf8::utf8strlen(bu2, uint(e2 - bu2)) == NELEM(s2) - 1);
	CHECK(errno == 0);
	uint32 bu3[20], *e3 = utf8::utf8_to_ucs4(s2, bu3);
	CHECK(utf8::utf8strlen(bu3, uint(e3 - bu3)) == NELEM(s2) - 1);
	CHECK(errno == 0);
}

TEST_CASE("cstrings: ")
{
	errno = 0;
	uint32 bu[10], bu2[10];
	for (uint i = 0; i < NELEM(bu); i++) { bu[i] = uint32(random()) + uint32(random()) * 0x10000u; }
	uint n = utf8::utf8strlen(bu, 10);
	str	 s = tempstr(n);
	ptr	 e = utf8::ucs4_to_utf8(bu, 10, s);
	CHECK(*e == 0);
	CHECK(e - s == n);
	CHECK(utf8::charcount(s) == 10);
	ssize_t len = utf8::utf8_to_ucs4(s, bu2) - bu2;
	CHECK(len == 10);
	CHECK(memcmp(bu, bu2, sizeof(bu)) == 0);
	CHECK(errno == 0);
}

TEST_CASE("cstrings: ")
{ // ucs4_to_utf8(), utf8_to_ucs4()
	char bu[20];
	errno = 0;
	for (int nbits = 0; nbits <= 32; nbits++) // max num of bits
	{
		for (int i = 0; i < 200; i++)
		{
			uint32 n[2], m[2];
			n[0] = uint32(random()) + uint32(random()) * 0x10000;
			if (nbits < 32) n[0] &= RMASK(nbits);
			n[1] = uint32(random()) + uint32(random()) * 0x10000;
			if (nbits < 32) n[1] &= RMASK(nbits);

			ptr e = utf8::ucs4_to_utf8(n, 2, bu);
			CHECK(*e == 0);
			CHECK(utf8::charcount(bu) == 2);
			CHECK(utf8::utf8_to_ucs4(bu, m) == m + 2);
			CHECK(memcmp(n, m, sizeof(n)) == 0);
		}
	}
	CHECK(errno == 0);
}

TEST_CASE("cstrings: ")
{ // ucs2_to_utf8(), utf8_to_ucs2()
	char bu[20];
	errno = 0;
	for (int nbits = 0; nbits <= 16; nbits++) // max num of bits
	{
		for (int i = 0; i < 100; i++)
		{
			uint16 n[2], m[2];
			n[0] = uint16(uint(random()) & RMASK(nbits));
			n[1] = uint16(uint(random()) & RMASK(nbits));

			ptr e = utf8::ucs2_to_utf8(n, 2, bu);
			CHECK(*e == 0);
			CHECK(utf8::charcount(bu) == 2);
			CHECK(utf8::utf8_to_ucs2(bu, m) == m + 2);
			CHECK(memcmp(n, m, sizeof(n)) == 0);
		}
	}
	CHECK(errno == 0);
}

TEST_CASE("cstrings: ")
{ // ucs1_to_utf8(), utf8_to_ucs1()
	char bu[20];
	errno = 0;
	for (int nbits = 0; nbits <= 8; nbits++) // max num of bits
	{
		for (int i = 0; i < 100; i++)
		{
			uint8 n[2], m[2];
			n[0] = uint8(uint(random()) & RMASK(nbits));
			n[1] = uint8(uint(random()) & RMASK(nbits));

			ptr e = utf8::ucs1_to_utf8(n, 2, bu);
			CHECK(*e == 0);
			CHECK(utf8::charcount(bu) == 2);
			CHECK(utf8::utf8_to_ucs1(bu, m) == m + 2);
			CHECK(memcmp(n, m, sizeof(n)) == 0);
		}
	}
	CHECK(errno == 0);
}

TEST_CASE("cstrings: ")
{
	CHECK(eq(utf8::fromhtmlstr("a&lt;int&amp;&gt;<br>"), "a<int&>\n"));
	CHECK(eq(utf8::fromhtmlstr("foo<br>a&lt;int&amp;&gt;<br>"), "foo\na<int&>\n"));
}

TEST_CASE("cstrings: ") { CHECK(eq(utf8::detabstr(" \thällö\t\n", 4), "    hällö   \n")); }
#endif
