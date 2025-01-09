// Copyright (c) 2018 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "tempmem.h"
//#include "hash/sdbm_hash.h"
//#include "main.h"
//#include "unix/FD.h"
#include "Xoshiro128.h"
#include "basic_math.h"
#include "cstrings.h"
#include "doctest.h"
#include "sdbm_hash.h"
#include <cstddef>
#include <cstring>
#include <pthread.h>


using namespace kio;

static Xoshiro128 rng(__DATE__ "123456"); // use compile date as seed, need 16 bytes
static uint		  random(uint n) { return n * (rng.next() >> 16) >> 16; }

static void alloc_some_bytes(uint n = 99)
{
	for (uint i = 0; i < n; i++) (void)tempstr(random(999));
}


static constexpr int  N			= 2000;
static constexpr uint max_align = sizeof(ptr);

TEST_CASE("cstrings: basic alloc")
{
	purge_tempmem();
	(void)tempstr(0);
	(void)tempstr(8);
	CHECK_NE(size_t(tempstr(79)) % max_align, 0); // not required but expected
	CHECK_NE(size_t(tempstr(79)) % max_align, 0); // not required but expected
	CHECK_EQ(size_t(tempmem(80)) % max_align, 0); // required
	(void)tempstr(12345);
	purge_tempmem();
	(void)tempstr(8);
}

TEST_CASE("tempmem: burn-in")
{
	TempMem outerpool;
	cstr	t1 = "Hello world!";
	cstr	t2;

	{
		TempMem tempmempool;
		t2 = "Have a nice day!";

		ptr	 list1[N];
		ptr	 list2[N];
		uint size[N];

		for (uint i = 0; i < N; i++)
		{
			uint n	 = min(random(0x1fff), random(0x1fff));
			list1[i] = tempmem(n);
			list2[i] = new char[n];
			size[i]	 = n;
			while (n--) list1[i][n] = char(random(256));
			memcpy(list2[i], list1[i], size[i]);
		}

		for (uint i = 0; i < N; i++)
		{
			CHECK(memcmp(list1[i], list2[i], size[i]) == 0);
			delete[] list2[i];
		}

		for (uint i = 0; i < N; i++)
		{
			uint n = min(random(0x1fff), random(0x1fff));
			str	 a = tempstr(n);
			str	 b = tempmem(n);
			CHECK(a[n] == 0);
			CHECK(size_t(b) % sizeof(ptr) == 0);
		}

		t2 = xdupstr(t2);
		tempmempool.purge();
		memset(tempmem(2000), 0, 2000);

		for (uint i = 0; i < N; i++)
		{
			uint n = min(random(0x1fff), random(0x1fff));
			str	 a = tempstr(n);
			str	 b = tempmem(n);
			cstr c = xdupstr(a);
			str	 d = xtempmem(n);
			CHECK(a[n] == 0);
			CHECK(size_t(b) % max_align == 0);
			CHECK(memcmp(a, c, kio::strlen(a) + 1) == 0);
			CHECK(size_t(d) % max_align == 0);
		}
	}

	CHECK(eq(t1, "Hello world!"));
	CHECK(eq(t2, "Have a nice day!"));
}

TEST_CASE("tempmem: burn-in #2")
{
	ptr	 list1[N];
	ptr	 list2[N];
	uint size[N];
	uint hash[N];

	{
		TempMem z;
		for (uint i = 0; i < N; i++)
		{
			uint n	 = min(random(0x1fff), random(0x1fff));
			list1[i] = tempmem(n);
			list2[i] = xtempmem(n);
			size[i]	 = n;
			while (n--) list1[i][n] = char(random(256));
			hash[i] = sdbm_hash(list1[i], size[i]);
			memcpy(list2[i], list1[i], size[i]);
		}
		alloc_some_bytes();
		for (uint i = 0; i < N; i++)
		{
			CHECK(memcmp(list1[i], list2[i], size[i]) == 0); //
		}
		alloc_some_bytes();
		purge_tempmem();
		alloc_some_bytes();
	}

	alloc_some_bytes();
	for (uint i = 0; i < N; i++)
	{
		CHECK(sdbm_hash(list2[i], size[i]) == hash[i]); //
	}
}


/*


































*/
