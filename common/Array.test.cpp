// Copyright (c) 2014 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "Array.h"
#include "doctest.h"

using namespace kio;

static bool foo_lt(int a, int b) { return (a ^ 3) < (b ^ 3); }


TEST_CASE("Array: sort")
{
	{
		int	 array1[] = {6, 4, 9, 2, 1, 0, 4, 8, 7, 6, 2, 1, 9, 9, 1};
		uint size = sizeof(array1), count = NELEM(array1);
		int	 array2[]  = {6, 4, 9, 2, 1, 0, 4, 8, 7, 6, 2, 1, 9, 9, 1};
		int	 array3[]  = {6, 4, 9, 2, 1, 0, 4, 8, 7, 6, 2, 1, 9, 9, 1};
		int	 array4[]  = {6, 4, 9, 2, 1, 0, 4, 8, 7, 6, 2, 1, 9, 9, 1};
		int	 sorted[]  = {0, 1, 1, 1, 2, 2, 4, 4, 6, 6, 7, 8, 9, 9, 9};
		int	 rsorted[] = {9, 9, 9, 8, 7, 6, 6, 4, 4, 2, 2, 1, 1, 1, 0};
		sort(&array1[0], &array1[count]);
		CHECK(memcmp(array1, sorted, size) == 0);
		rsort(&array2[0], &array2[count]);
		CHECK(memcmp(array2, rsorted, size) == 0);
		sort(&array3[0], &array3[count], lt);
		CHECK(memcmp(array3, sorted, size) == 0);
		sort(&array4[0], &array4[count], gt);
		CHECK(memcmp(array4, rsorted, size) == 0);
	}

	{
		const char q[]		 = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!',
								10,	 'D', 'i', 'e', ' ', 'K', 'u', 'h', '.', 10,  0};
		const char sorted[]	 = {10,	 10,  32,  32,	33,	 46,  68,  72,	75,	 87,  100, 101,
								101, 104, 105, 108, 108, 108, 111, 111, 114, 117, 0};
		const char rsorted[] = {117, 114, 111, 111, 108, 108, 108, 105, 104, 101, 101, 100,
								87,	 75,  72,  68,	46,	 33,  32,  32,	10,	 10,  0};
		str		   s;
		s = dupstr(q);
		sort(s, strchr(s, 0));
		CHECK(eq(s, sorted));
		s = dupstr(q);
		rsort(s, strchr(s, 0));
		CHECK(eq(s, rsorted));
		s = dupstr(q);
		sort(s, strchr(s, 0), lt);
		CHECK(eq(s, sorted));
		s = dupstr(q);
		sort(s, strchr(s, 0), gt);
		CHECK(eq(s, rsorted));
	}

	// sort str and cstr

	{
		cstr  q[]  = {"Anton", "\tfoo", nullptr, "\t", "anton", "Antonov"};
		uint  size = sizeof(q), count = NELEM(q);
		cstr  sorted[]	= {nullptr, "\t", "\tfoo", "Anton", "Antonov", "anton"};
		cstr  rsorted[] = {"anton", "Antonov", "Anton", "\tfoo", "\t", nullptr};
		cstr* a;
		a = new cstr[count];
		memcpy(a, q, size);
		sort(a, a + count);
		CHECK(memcmp(a, sorted, size) == 0);
		a = new cstr[count];
		memcpy(a, q, size);
		rsort(a, a + count);
		CHECK(memcmp(a, rsorted, size) == 0);
		a = new cstr[count];
		memcpy(a, q, size);
		sort(a, a + count, lt);
		CHECK(memcmp(a, sorted, size) == 0);
		a = new cstr[count];
		memcpy(a, q, size);
		sort(a, a + count, gt);
		CHECK(memcmp(a, rsorted, size) == 0);
	}

	{
		cstr q[]  = {"Anton", "\tfoo", nullptr, "\t", "anton", "Antonov"};
		uint size = sizeof(q), count = NELEM(q);
		cstr sorted[]  = {nullptr, "\t", "\tfoo", "Anton", "Antonov", "anton"};
		cstr rsorted[] = {"anton", "Antonov", "Anton", "\tfoo", "\t", nullptr};
		str* a;
		a = new str[count];
		memcpy(a, q, size);
		sort(a, a + count);
		CHECK(memcmp(a, sorted, size) == 0);
		a = new str[count];
		memcpy(a, q, size);
		rsort(a, a + count);
		CHECK(memcmp(a, rsorted, size) == 0);
		a = new str[count];
		memcpy(a, q, size);
		sort(a, a + count, lt);
		CHECK(memcmp(a, sorted, size) == 0);
		a = new str[count];
		memcpy(a, q, size);
		sort(a, a + count, gt);
		CHECK(memcmp(a, rsorted, size) == 0);
	}
}

TEST_CASE("Array: test1")
{
	{
		Array<int> array;
		CHECK(array.count() == 0);
	}

	{
		const char* s = "abc";
		Array<char> a(s, 4);
		CHECK(a[0] == 'a');
		CHECK(a[3] == 0);
	}

	{
		Array<int> a;
		CHECK(a.getData() == nullptr);
		a << 1 << 2;
		CHECK(a.count() == 2);
		CHECK(a.getData() == &a[0]);
		const Array<int> b(a);
		CHECK(b.count() == 2);
		CHECK(b.getData() == &b[0]);
		CHECK(a == b);
		CHECK(a.getData() != b.getData());
	}

	{
		Array<uint16> a;
		a << 42 << 7 << 99;
		CHECK(a.first() == 42);
		CHECK(a.last() == 99);
		const Array<uint16> b(a);
		CHECK(b.first() == 42);
		CHECK(b.last() == 99);
		CHECK(&b.first() != &a.first());
		Array<float> c;
		c << 100.25f;
		CHECK(c.first() == 100.25f);
		CHECK(&c.first() == &c.last());
	}

	/*{
	Array<int> a;
	a.first()++;
	CHECK(InternalError)*/

	{
		Array<int> a;
		a.resize(2);
		CHECK(a.count() == 2);
		CHECK(a[0] == 0);
		a[0] = 44;
		CHECK(a[1] == 0);
		a[1]++;
		a.resize(10);
		CHECK(a.count() == 10);
		CHECK(a[0] == 44);
		CHECK(a[1] == 1);
		CHECK(a[2] == 0);
		a[2] = 99;
		CHECK(a[9] == 0);
		a.resize(3);
		CHECK(a.count() == 3);
		CHECK(a[0] == 44);
		CHECK(a[1] == 1);
		CHECK(a[2] == 99);
	}

	{
		Array<int> a;
		a << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8;
		CHECK(a.copyofrange(2, 99) == Array<int>() << 3 << 4 << 5 << 6 << 7 << 8);
		CHECK(a.copyofrange(0, 3) == Array<int>() << 1 << 2 << 3);
	}

	{
		Array<int> a;
		a << 1 << 2 << 3;
		CHECK(Array<int>() != a);
		CHECK(a != Array<int>() << 1 << 2 << 4);
		CHECK(a != Array<int>() << 1 << 2);
	}

	{
		Array<int> a;
		a << 1 << 2 << 3 << 4;
		static const int s[] = {5, 6, 7, 8, 9};
		a.insertat(2, s, 5);
		CHECK(a.count() == 9);
		CHECK(a == Array<int>() << 1 << 2 << 5 << 6 << 7 << 8 << 9 << 3 << 4);
	}

	{
		Array<int> a;
		a << 1 << 2 << 3 << 4;
		Array<int> s;
		s << 5 << 6 << 7 << 8 << 9;
		a.insertat(2, s);
		CHECK(a.count() == 9);
		CHECK(a == Array<int>() << 1 << 2 << 5 << 6 << 7 << 8 << 9 << 3 << 4);
	}

	{
		Array<int> a;
		a << 1 << 3 << 5 << 7;
		CHECK(a == Array<int>() << 1 << 3 << 5 << 7);
		a.insertsorted(4);
		CHECK(a == Array<int>() << 1 << 3 << 4 << 5 << 7);
		a.insertsorted(4);
		CHECK(a == Array<int>() << 1 << 3 << 4 << 4 << 5 << 7);
	}

	{
		Array<int> a;
		a.grow() = 22;
		CHECK(a == Array<int>() << 22);

		a.grow(8);
		CHECK(a == Array<int>() << 22 << 0 << 0 << 0 << 0 << 0 << 0 << 0);

		a[3] = 33;
		a.shrink(4);
		CHECK(a == Array<int>() << 22 << 0 << 0 << 33);

		a.purge();
		CHECK(a.count() == 0);

		a.append(11);
		CHECK(a.count() == 1);
		CHECK(a[0] == 11);

		int* aa = &a[0];
		a.grow(2, 44);
		CHECK(a == Array<int>() << 11 << 0);
		int* bb = &a[0];
		CHECK(aa != bb);

		a.grow(8);
		CHECK(a.count() == 8);
		CHECK(a[0] == 11);
		CHECK(a[1] == 0);
		CHECK(&a[0] == bb);

		a.append(77);
		CHECK(a.count() == 9);
		CHECK(a[8] == 77);
		CHECK(&a[0] == bb);

		a.grow(5, 20);
		CHECK(a.count() == 9);
		CHECK(&a[0] == bb);

		a.grow(20, 44);
		CHECK(a.count() == 20);
		CHECK(&a[0] == bb);

		a.grow(0, 55);
		CHECK(a.count() == 20);
		CHECK(a[0] == 11);
		//CHECK(&a[0] != bb); // expected not required
	}

	Array<int> array;

	{
		array.append(3);
		array.append(6);
		CHECK(array.count() == 2);
		CHECK(array.first() == 3);
		CHECK(array.last() == 6);
	}

	{
		array << 9 << 12;
		CHECK(array.count() == 4);
		CHECK(array[2] == 9);
		CHECK(array[3] == 12);
	}

	{
		array.insertat(2, 5);
		CHECK(array == Array<int>() << 3 << 6 << 5 << 9 << 12);
	}

	{
		array.removeat(1);
		CHECK(array == Array<int>() << 3 << 5 << 9 << 12);
	}

	{
		array.removeat(0);
		CHECK(array == Array<int>() << 5 << 9 << 12);
	}

	{
		array.removeat(2);
		CHECK(array == Array<int>() << 5 << 9);
	}

	{
		array.insertat(0, 6);
		CHECK(array == Array<int>() << 6 << 5 << 9);
	}

	{
		array.insertat(3, 7);
		CHECK(array == Array<int>() << 6 << 5 << 9 << 7);
	}

	/*{ array[4]; CHECK(InternalError)*/

	/*{ array[uint(-1)]; CHECK(InternalError)*/

	{
		array[3]++;
		CHECK(array == Array<int>() << 6 << 5 << 9 << 8);
	}

	{
		Array<int> array2(array);
		CHECK(array == array2);
		CHECK(array2 == Array<int>() << 6 << 5 << 9 << 8);
	}

	Array<int> array2(std::move(array));

	{
		CHECK(array.count() == 0);
		CHECK(array2 == Array<int>() << 6 << 5 << 9 << 8);
	}

	{
		array = array2;
		CHECK(array2 == array);
		CHECK(array == Array<int>() << 6 << 5 << 9 << 8);
	}

	{
		array.purge();
		array = std::move(array2);
		CHECK(array2.count() == 0);
		CHECK(array == Array<int>() << 6 << 5 << 9 << 8);
	}

	{
		array.purge();
		CHECK(array.count() == 0);
	}

	{
		array.revert();
		CHECK(array.count() == 0);
	}

	{
		array.append(0);
		array.revert();
		CHECK(array.count() == 1);
		CHECK(array[0] == 0);
	}

	{
		array << 1 << 2 << 3 << 4 << 5 << 6 << 7;
		array.revert();
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		array.rol();
		CHECK(array == Array<int>() << 6 << 5 << 4 << 3 << 2 << 1 << 0 << 7);
	}

	{
		array.ror();
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		array.rol(1, 1);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		array.rol(2, 0);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		array.ror(1, 1);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		array.ror(2, 0);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		array.ror(2, 8);
		CHECK(array == Array<int>() << 7 << 6 << 0 << 5 << 4 << 3 << 2 << 1);
	}

	{
		array.rol(2, 8);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		array.rol(2, 9);
		CHECK(array == Array<int>() << 7 << 6 << 4 << 3 << 2 << 1 << 0 << 5);
	}

	{
		array.ror(2, 9);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1 << 0);
	}

	{
		int n = array.pop();
		CHECK(n == 0);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2 << 1);
	}

	{
		array.drop();
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 3 << 2);
	}

	{
		array.removerange(2, 4);
		CHECK(array == Array<int>() << 7 << 6 << 3 << 2);
	}

	{
		array.insertrange(2, 4);
		CHECK(array == Array<int>() << 7 << 6 << 0 << 0 << 3 << 2);
	}

	{
		array.removerange(4, 99);
		CHECK(array == Array<int>() << 7 << 6 << 0 << 0);
	}

	{
		array[2]	 = 5;
		array.last() = 4;
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4);
	}

	{
		array2 = Array<int>() << 1 << 2 << 3;
		array.append(array2);
		CHECK(array2.count() == 3);
		CHECK(array == Array<int>() << 7 << 6 << 5 << 4 << 1 << 2 << 3);
	}

	{
		array.shrink(2);
		CHECK(array == Array<int>() << 7 << 6);
	}

	{
		array.append(Array<int>(array));
		CHECK(array == Array<int>() << 7 << 6 << 7 << 6);
	}

	{
		array.purge();
		array.sort();
		CHECK(array.count() == 0);
	}

	{
		array << 1;
		array.sort();
		CHECK(array.count() == 1);
		CHECK(array[0] == 1);
	}

	{
		array << 7 << 5 << 9 << 3 << 8 << 5 << 0;
		array.sort();
		CHECK(array == Array<int>() << 0 << 1 << 3 << 5 << 5 << 7 << 8 << 9);
	}

	{
		array.rsort();
		CHECK(array == Array<int>() << 9 << 8 << 7 << 5 << 5 << 3 << 1 << 0);
	}

	{
		array.sort(7, 99);
		CHECK(array == Array<int>() << 9 << 8 << 7 << 5 << 5 << 3 << 1 << 0);
	}

	{
		array.sort(4, 8);
		CHECK(array == Array<int>() << 9 << 8 << 7 << 5 << 0 << 1 << 3 << 5);
	}

	{
		array.sort(foo_lt);
		CHECK(array == Array<int>() << 3 << 1 << 0 << 7 << 5 << 5 << 9 << 8);
	}

	{
		array.rsort(2, 7);
		CHECK(array == Array<int>() << 3 << 1 << 9 << 7 << 5 << 5 << 0 << 8);
	}

	{
		array.sort(2, 99, lt);
		CHECK(array == Array<int>() << 3 << 1 << 0 << 5 << 5 << 7 << 8 << 9);
	}

	{
		Array<uint> a;
		a << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7;
		uint b[8][8];
		memset(b, 0, sizeof(b));
		uint n, l;

		for (n = 0, l = 0; l < 10000 && n < 64; l++)
		{
			a.shuffle();
			for (uint i = 0; i < 8; i++)
			{
				uint z = a[i];
				if (++b[z][i] == 1) n++;
			}
		}

		if (l >= 10000)
		{
			printf("  shuffle: %i permutations not seen after %u runs:\n", 64 - n, l);
			for (int z = 0; z < 8; z++)
				for (int i = 0; i < 8; i++)
					if (b[z][i] == 0) printf("    number %i at index %i\n", z, i);
		}

		CHECK(l < 10000);
		printf("  shuffle: all permutations seen after %u runs\n", l);
		a.sort();
		CHECK(a == Array<uint>() << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7);
	}

	//	{
	//		Array<int> a;
	//		a << 123 << -99 << 0 << 2127741132 << -2127745532;
	//		FD fd;
	//		fd.open_tempfile();
	//		a.print(fd, "•");
	//		fd.write_char('X');
	//		fd.rewind_file();
	//		cstr s = fd.read_str();
	//		CHECK(eq("•Array[5]", s));
	//		for (uint i = 0; i < a.count(); i++)
	//		{
	//			s = fd.read_str();
	//			CHECK(eq(s, usingstr("  •[%2u] %s", i, tostr(a[i]))));
	//		}
	//		CHECK(fd.read_char() == 'X');
	//	}

	//	{
	//		int		   m = Array<int>::MAGIC * 0x10001;
	//		Array<int> a;
	//		a << m << 123 << -99 << 0 << 2127741132 << -2127745532 << m;
	//		Array<int> b;
	//		b << 1;
	//		FD fd;
	//		fd.open_tempfile();
	//		a.serialize(fd);
	//		fd.write_char('X');
	//		fd.rewind_file();
	//		b.deserialize(fd);
	//		CHECK(a == b);
	//		CHECK(fd.read_char() == 'X');
	//	}
}

TEST_CASE("Array: test2")
{
	{
		Array<int> a(uint(0), 5);
		a << 1 << 2 << 3;
		Array<int> b(uint(0), 6);
		b << 4 << 5 << 6 << 8;
		Array<int> a2(a);
		Array<int> b2(b);
		std::swap(a, b);
		CHECK(a.count() == 4);
		CHECK(a == b2);
		CHECK(b == a2);
	}

	Array<cstr> array = std::move(
		Array<cstr>() << "11"
					  << "22"
					  << "33");

	//	FD fd;
	//	{
	//		fd.open_tempfile();
	//		array.serialize(fd);
	//	}
	//	{
	//		array.shrink(1);
	//		fd.rewind_file();
	//		array.deserialize(fd);
	//		CHECK(
	//			array == Array<cstr>() << "11"
	//								   << "22"
	//								   << "33");
	//	}

	{
		array.appendifnew("44");
		CHECK(array.count() == 4);
	}

	{
		array.appendifnew("22");
		CHECK(array.count() == 4);
	}

	{
		array.removeitem("33");
		CHECK(array.count() == 3);
	}

	{
		array.removeat(1);
		CHECK(
			array == Array<cstr>() << "11"
								   << "44");
	}

	{
		array << "foo"
			  << "bar";
		CHECK(
			array == Array<cstr>() << "11"
								   << "44"
								   << "foo"
								   << "bar");
		array.remove(1);
		CHECK(
			array == Array<cstr>() << "11"
								   << "foo"
								   << "bar");
		array.remove("foo");
		CHECK(
			array == Array<cstr>() << "11"
								   << "bar");
	}
}
