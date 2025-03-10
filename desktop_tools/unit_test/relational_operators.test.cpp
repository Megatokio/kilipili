// Copyright (c) 2019 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Array.h"
//#include "RCObject.h"
#include "RCPtr.h"
//#include "unix/FD.h"
#include "doctest.h"


using namespace kio;


TEST_CASE("rel_op: int")
{
	static const int n[] = {-2, -1, 0, +1, +2};
	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			int a = n[i];
			int b = n[j];
			CHECK_EQ(eq(a, b), (a == b));
			CHECK_EQ(ne(a, b), (a != b));
			CHECK_EQ(lt(a, b), (a < b));
			CHECK_EQ(gt(a, b), (a > b));
			CHECK_EQ(le(a, b), (a <= b));
			CHECK_EQ(ge(a, b), (a >= b));
		}
}

TEST_CASE("rel_op: uint")
{
	static const uint n[] = {0, +1, +2, ~0u};
	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			uint a = n[i];
			uint b = n[j];
			CHECK_EQ(eq(a, b), (a == b));
			CHECK_EQ(ne(a, b), (a != b));
			CHECK_EQ(lt(a, b), (a < b));
			CHECK_EQ(gt(a, b), (a > b));
			CHECK_EQ(le(a, b), (a <= b));
			CHECK_EQ(ge(a, b), (a >= b));
		}
}

TEST_CASE("rel_op: signed char")
{
	static const signed char n[] = {-2, -1, 0, +1, +2};
	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			signed char a = n[i];
			signed char b = n[j];
			CHECK_EQ(eq(a, b), (a == b));
			CHECK_EQ(ne(a, b), (a != b));
			CHECK_EQ(lt(a, b), (a < b));
			CHECK_EQ(gt(a, b), (a > b));
			CHECK_EQ(le(a, b), (a <= b));
			CHECK_EQ(ge(a, b), (a >= b));
		}
}

TEST_CASE("rel_op: uint64")
{
	static const uint64 n[] = {0, +1, +2, ~0u};
	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			uint64 a = n[i];
			uint64 b = n[j];
			CHECK_EQ(eq(a, b), (a == b));
			CHECK_EQ(ne(a, b), (a != b));
			CHECK_EQ(lt(a, b), (a < b));
			CHECK_EQ(gt(a, b), (a > b));
			CHECK_EQ(le(a, b), (a <= b));
			CHECK_EQ(ge(a, b), (a >= b));
		}
}

TEST_CASE("rel_op: float")
{
	static const float n[] = {-3.3e-13f, 2.2e-3f, 2.2e+3f, 0.0f};
	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			float a = n[i];
			float b = n[j];
			CHECK_EQ(eq(a, b), (a == b));
			CHECK_EQ(ne(a, b), (a != b));
			CHECK_EQ(lt(a, b), (a < b));
			CHECK_EQ(gt(a, b), (a > b));
			CHECK_EQ(le(a, b), (a <= b));
			CHECK_EQ(ge(a, b), (a >= b));
		}
}

TEST_CASE("rel_op: double")
{
	static const double n[] = {-3.3e-13, 2.2e-3, 2.2e+3, 0.0};
	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			double a = n[i];
			double b = n[j];
			CHECK_EQ(eq(a, b), (a == b));
			CHECK_EQ(ne(a, b), (a != b));
			CHECK_EQ(lt(a, b), (a < b));
			CHECK_EQ(gt(a, b), (a > b));
			CHECK_EQ(le(a, b), (a <= b));
			CHECK_EQ(ge(a, b), (a >= b));
		}
}

TEST_CASE("rel_op: long double")
{
	static const long double n[] = {-3.3e-13l, 2.2e-3l, 2.2e+3l, 0.0l};
	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			long double a = n[i];
			long double b = n[j];
			CHECK_EQ(eq(a, b), (a == b));
			CHECK_EQ(ne(a, b), (a != b));
			CHECK_EQ(lt(a, b), (a < b));
			CHECK_EQ(gt(a, b), (a > b));
			CHECK_EQ(le(a, b), (a <= b));
			CHECK_EQ(ge(a, b), (a >= b));
		}
}

TEST_CASE("rel_op: cstr")
{
	cstr a = "1.1e33l", b = "Anton", c = "anton", e = nullptr;
	CHECK(eq(b, "Anton"));
	CHECK(ne(b, c));
	CHECK(gt(b, a));
	CHECK(lt(a, b));
	CHECK(gt(b, ""));
	CHECK(lt("", b));
	CHECK(eq("", e));
	CHECK(ne(e, b));
	CHECK(gt(b, e));
	CHECK(lt(e, b));
	CHECK(!gt("", e));
	CHECK(!lt("", e));
	CHECK(!gt(e, ""));
	CHECK(!lt(e, ""));
	CHECK(!gt(e, e));
	CHECK(!lt(e, e));
}

TEST_CASE("rel_op: str, cstr")
{
	cstr a = "Anton", b = "anton";
	str	 c = dupstr(b);
	CHECK(eq(b, c));
	CHECK(eq(c, b));
	CHECK(ne(a, c));
	CHECK(ne(c, a));
	CHECK(gt(c, a));
	CHECK(!gt(a, c));
	CHECK(lt(a, c));
	CHECK(!lt(c, a));
	CHECK(gt(c, ""));
	CHECK(lt("", c));
}

TEST_CASE("rel_op: class")
{
	class Foo
	{
	public:
		int n;
		Foo(int n) : n(n) {}
		bool operator==(const Foo& b) const { return n == b.n; }
		bool operator!=(const Foo& b) const { return n != b.n; }
		bool operator<(const Foo& b) const { return n < b.n; }
		bool operator>(const Foo& b) const { return n > b.n; }
		bool operator<=(const Foo& b) const { return n <= b.n; }
		bool operator>=(const Foo& b) const { return n >= b.n; }
	};

	static Foo n[] = {-2, -1, 0, 2, 3};

	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			const Foo& a = n[i];
			Foo&	   b = n[j];
			CHECK_EQ((a == b), (a.n == b.n));
			CHECK_EQ((a != b), (a.n != b.n));
			CHECK_EQ((a < b), (a.n < b.n));
			CHECK_EQ((a > b), (a.n > b.n));
			CHECK_EQ((a <= b), (a.n <= b.n));
			CHECK_EQ((a >= b), (a.n >= b.n));
		}

	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			const Foo& a = n[i];
			Foo&	   b = n[j];
			CHECK_EQ(eq(a, b), (a.n == b.n));
			CHECK_EQ(ne(a, b), (a.n != b.n));
			CHECK_EQ(lt(a, b), (a.n < b.n));
			CHECK_EQ(gt(a, b), (a.n > b.n));
			CHECK_EQ(le(a, b), (a.n <= b.n));
			CHECK_EQ(ge(a, b), (a.n >= b.n));
		}

	for (uint i = 0; i < NELEM(n); i++)
		for (uint j = 0; j < NELEM(n); j++)
		{
			const Foo* a = &n[i];
			Foo*	   b = &n[j];
			CHECK_EQ(eq(a, b), (a->n == b->n));
			CHECK_EQ(ne(a, b), (a->n != b->n));
			CHECK_EQ(lt(a, b), (a->n < b->n));
			CHECK_EQ(gt(a, b), (a->n > b->n));
			CHECK_EQ(le(a, b), (a->n <= b->n));
			CHECK_EQ(ge(a, b), (a->n >= b->n));
		}
}

TEST_CASE("rel_op: RCPtr")
{
	class Foo : public RCObject
	{
	public:
		int n;
		Foo(int n) : n(n) {}
		bool operator==(const Foo& b) const { return n == b.n; }
		bool operator!=(const Foo& b) const { return n != b.n; }
		bool operator<(const Foo& b) const { return n < b.n; }
		bool operator>(const Foo& b) const { return n > b.n; }
		bool operator<=(const Foo& b) const { return n <= b.n; }
		bool operator>=(const Foo& b) const { return n >= b.n; }
	};

	RCPtr<Foo> p[] = {new Foo(-2), new Foo(-1), new Foo(0), new Foo(2), new Foo(3), new Foo(-2)};

	for (uint i = 0; i < NELEM(p); i++)
		for (uint j = 0; j < NELEM(p); j++)
		{
			RCPtr<Foo> a = p[i];
			RCPtr<Foo> b = p[j];
			CHECK_EQ((a == b), (a.ptr() == b.ptr()));
			CHECK_EQ((a != b), (a.ptr() != b.ptr()));
			CHECK_EQ((a < b), (a.ptr() < b.ptr()));
			CHECK_EQ((a > b), (a.ptr() > b.ptr()));
			CHECK_EQ((a <= b), (a.ptr() <= b.ptr()));
			CHECK_EQ((a >= b), (a.ptr() >= b.ptr()));
		}

	for (uint i = 0; i < NELEM(p); i++)
		for (uint j = 0; j < NELEM(p); j++)
		{
			RCPtr<Foo> a = p[i];
			RCPtr<Foo> b = p[j];
			CHECK_EQ((*a == *b), (a->n == b->n));
			CHECK_EQ((*a != *b), (a->n != b->n));
			CHECK_EQ((*a < *b), (a->n < b->n));
			CHECK_EQ((*a > *b), (a->n > b->n));
			CHECK_EQ((*a <= *b), (a->n <= b->n));
			CHECK_EQ((*a >= *b), (a->n >= b->n));
		}

	for (uint i = 0; i < NELEM(p); i++)
		for (uint j = 0; j < NELEM(p); j++)
		{
			RCPtr<Foo> a = p[i];
			RCPtr<Foo> b = p[j];
			CHECK_EQ(eq(a, b), (a->n == b->n));
			CHECK_EQ(ne(a, b), (a->n != b->n));
			CHECK_EQ(lt(a, b), (a->n < b->n));
			CHECK_EQ(gt(a, b), (a->n > b->n));
			CHECK_EQ(le(a, b), (a->n <= b->n));
			CHECK_EQ(ge(a, b), (a->n >= b->n));
		}
}
