// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "basic_math.h"
#include "kilipili_cdefs.h"
#include "standard_types.h"
#include "template_helpers.h"
#include <string.h>


namespace kio
{

template<typename T>
constexpr int find_sorted_insertion_point(const T data[], int a, int e, T x, bool(lt)(const T&, const T&))
{
	if (a >= e) return a;
	if (lt(x, data[a])) return a;

	if (lt(x, data[e - 1]))
	{
		while (a < e - 1)
		{
			int m = (a + e) >> 1;
			if (lt(x, data[m])) e = m;
			else a = m;
		}
	}
	return e;
}

template<typename T>
constexpr int find_sorted_insertion_point(const T data[], int a, int e, T x)
{
	// find insertion point for element x in sorted array data[a … [e.
	// 'same values' will be inserted behind the 'same value'.
	// the template uses
	//	 lt(T,T) if defined
	// else for pointers
	//	 *T < *T  or  lt(*T,*T) if defined
	// else T < T.

	if constexpr (has_operator_lt<T>)
		return find_sorted_insertion_point(data, a, e, x, [](const T& a, const T& b) { return lt(a, b); });
	if constexpr (has_operator_star<T>)
		if constexpr (has_operator_lt<typename std::remove_pointer_t<T>>)
			return find_sorted_insertion_point(data, a, e, x, [](const T& a, const T& b) { return lt(*a, *b); });
		else return find_sorted_insertion_point(data, a, e, x, [](const T& a, const T& b) { return *a < *b; });
	else
	{
		if (a >= e) return a;
		if (x < data[a]) return a;

		if (x < data[e - 1])
		{
			while (a < e - 1)
			{
				int m = (a + e) >> 1;
				if (x < data[m]) e = m;
				else a = m;
			}
		}

		return e;
	}
}


static_assert(find_sorted_insertion_point("abcdefghijk", 0, 0, 'a') == 0);
static_assert(find_sorted_insertion_point("bcdefghijk", 0, 10, 'a') == 0);
static_assert(find_sorted_insertion_point("abcdefghijk", 0, 10, 'a') == 1);
static_assert(find_sorted_insertion_point("abcdefghijk", 1, 10, 'b') == 2);
static_assert(find_sorted_insertion_point("abcdeghijk", 0, 10, 'f') == 5);
static_assert(find_sorted_insertion_point("abcdefghijk", 1, 10, 'f') == 6);
static_assert(find_sorted_insertion_point("abcdefghijk", 0, 9, 'f') == 6);
static_assert(find_sorted_insertion_point("abcdefghijk", 1, 7, 'c') == 3);
static_assert(find_sorted_insertion_point("abbbbfghijk", 0, 10, 'b') == 5);
static_assert(find_sorted_insertion_point("abcdefghij", 1, 10, 'j') == 10);
static_assert(find_sorted_insertion_point("abcdefghij", 0, 10, 'k') == 10);
static_assert(find_sorted_insertion_point("abcdefghijk", 1, 7, 'a') == 1);
static_assert(find_sorted_insertion_point(&"abcdefghijk"[4], -4, 0, 'a') == -3);
static_assert(find_sorted_insertion_point(&"abcdefghijk"[10], 10, 1, 'r') == 10);


inline uint32 hash(cstr key)
{
	// note: this is the sdbm algorithm:
	// hash(i) = hash(i-1) * 65599 + str[i];
	// see: http://www.cse.yorku.ca/~oz/hash.html
	// https://programmers.stackexchange.com/questions/49550

	uint hash = 0;
	while (int c = *key++) { hash = (hash << 6) + (hash << 16) - hash + uint(c); }
	return hash;
}

template<typename T>
inline uint32 hash(T v)
{
	return v;
}

constexpr char DUPLICATE_KEY[] = "duplicate key";


/* ————————————————————————————————————————————————————————————————————————
   fixed-size hash table
   usable if you know ahead how large it needs to be
*/
template<typename VALUE, uint BITS, typename KEY = cstr>
class FixedHashMap
{
	// table should not be filled to more than 80%.
	// ATTN: if filled to 100% then find(key) for n.ex. key will never return!

	struct KV
	{
		KEY	  key;
		VALUE value;
	};

	static constexpr uint size = 1 << BITS;
	static constexpr uint mask = size - 1;

	KV table[size] = {0};

	FixedHashMap() noexcept {}
	~FixedHashMap() noexcept { free(table); }

	/* 
		find key or slot for new key
	*/
	KV& find(KEY key) const noexcept
	{
		for (uint h = hash(key);; h++)
		{
			KV& kv = table[h & mask];
			if (kv.key == key || !kv.key) return kv;
		}
	}

	/* 
		add or set
	*/
	void set(KEY key, VALUE value) noexcept
	{
		KV& kv	 = find(key);
		kv.value = value;
		kv.key	 = key;
	}

	/*	
		add or throw
	*/
	void add(KEY key, VALUE value) throws
	{
		KV& kv = find(key);
		if unlikely (kv.key) throw DUPLICATE_KEY;
		kv.key	 = key;
		kv.value = value;
	}

	/* 
		remove if exists
	*/
	void remove(KEY key) noexcept
	{
		KV& kv = find(key);
		if (!kv.key) return;
		kv.key = KEY(0);

		// check all following keys whether their slot was relocated from a lower index:
		for (uint i = &kv - table;;)
		{
			KV& a = table[++i & mask];
			if (!a.key) break;			 // no forced relocation across a gap!
			KV& b = find(a.key);		 // where should it go?
			if (!b.key) std::swap(b, a); // move it there
		}
	}

	/* 
		find key and return ref to value
		returns ref to void value if key not found
	*/
	VALUE&		 operator[](KEY key) noexcept { return find(key).value; }
	const VALUE& operator[](KEY key) const noexcept { return find(key).value; }
	VALUE&		 get(KEY key) noexcept { return find(key).value; }
	const VALUE& get(KEY key) const noexcept { return find(key).value; }

	/*
		find key and return value or return default value if key not found
	*/
	VALUE get(KEY key, VALUE dflt) const noexcept
	{
		KV& kv = find(key).value;
		return kv.key ? kv.value : dflt;
	}
};


/* ————————————————————————————————————————————————————————————————————————
   universal hash table
   automatically grows as needed
*/
template<typename KEY, typename VALUE>
class HashMap
{
	struct KV
	{
		KEY	  key;
		VALUE value;
	};

	uint mask  = 0;
	uint max   = 0;
	uint cnt   = 0;
	KV*	 table = nullptr;

public:
	HashMap() noexcept {}
	~HashMap() noexcept { free(table); }

	uint count() const noexcept { return cnt; }

	void purge() noexcept
	{
		mask = max = cnt = 0;
		free(table);
		table = nullptr;
	}

	/*
		grow table 
		new size must be 2^N
	*/
	void grow(uint newsize) throws
	{
		uint oldsize = table ? mask + 1 : 0;

		assert((newsize & (newsize - 1)) == 0); // 2^N
		assert(newsize > oldsize);

		KV* newtable = reinterpret_cast<KV*>(realloc(table, newsize * sizeof(KV)));
		if (newtable)
		{
			table = newtable;
			memset(table + oldsize, 0, (newsize - oldsize) * sizeof(KV));
			mask = newsize - 1;
			max	 = newsize / 2 + newsize / 4;

			// relocate all entries which should have gone into the upper half.
			// relocate all entries which subsequently should have gone into a thereby freed slot.
			// after the first free slot is passed all subsequent slots fall into their correct position.

			uint i0 = 0;
			while (table[i0].key) i0++; // find first free slot

			for (uint i = i0; i < oldsize + i0; i++)
			{
				KV& a = table[i & (mask >> 1)];
				if (!a.key) continue; // empty slot
				KV& b = find(a.key);
				if (b.key) continue; // found (in lower half)
				std::swap(a, b);	 // not found (upper or lower half)
			}

#ifdef DEBUG
			uint n = 0;
			for (uint i = 0; i < newsize; i++)
			{
				if (!table[i].key) continue;
				assert(&find(table[i].key) == &table[i]);
				n++;
			}
			assert(n == cnt);
#endif
		}
		else if (cnt == mask) throw OUT_OF_MEMORY; // 1 slot *must* remain free!
	}

	/* 
		find existing key or slot for new key
	*/
	KV& find(KEY key) const noexcept
	{
		for (uint h = hash(key);; h++)
		{
			KV& kv = table[h & mask];
			if (kv.key == key || !kv.key) return kv;
		}
	}

	/* 
		add or set
	*/
	void set(KEY key, VALUE value) throws
	{
		if (cnt >= max) grow((mask + 1) * 2);
		KV& kv	 = find(key);
		kv.value = value;
		if (kv.key) return; // key exists
		kv.key = key;		// add new key
		cnt++;
	}

	/* 
		add or throw
	*/
	void add(KEY key, VALUE value) throws
	{
		if (cnt >= max) grow((mask + 1) * 2);
		KV& kv = find(key);
		if unlikely (kv.key) throw DUPLICATE_KEY;
		kv.key	 = key;
		kv.value = value;
		cnt++;
	}

	/* 
		remove if exists
	*/
	void remove(const KEY& key) noexcept
	{
		KV& kv = find(key);
		if (!kv.key) return; // wasn't in table
		kv.key = KEY(0);
		cnt--;

		// check all following keys whether their slot was shifted from a lower index:
		for (uint i = &kv - table;;)
		{
			KV& a = table[++i & mask];
			if (!a.key) break;			 // no shift across a gap!
			KV& b = find(a.key);		 // where should it go?
			if (!b.key) std::swap(b, a); // move it there
		}
	}

	/* 
		find key and return ref to value
		return ref to void value if key not found
	*/
	VALUE&		 operator[](const KEY& key) noexcept { return find(key).value; }
	const VALUE& operator[](const KEY& key) const noexcept { return find(key).value; }
	VALUE&		 get(const KEY& key) noexcept { return find(key).value; }
	const VALUE& get(const KEY& key) const noexcept { return find(key).value; }

	/*
		find key and return value 
		return default value if key not found
	*/
	VALUE get(const KEY& key, VALUE dflt) const noexcept
	{
		KV& kv = find(key);
		return kv.key ? kv.value : dflt;
	}
};


/* ————————————————————————————————————————————————————————————————————————
   bucket sort helpers
*/
template<typename T>
constexpr uint ordinal(const T& n, uint total_bits, uint mask)
{
	// default implementation for integer types:

	uint ss = sizeof(T) * 8 - total_bits;
	return (n >> ss) & mask;
}

constexpr uint ordinal(cstr s, uint total_bits, uint mask)
{
	// implementation for cstrings:

	if (!s) return 0;

	while (*s && total_bits > 8)
	{
		s++;
		total_bits -= 8;
	}

	uint ss = sizeof(char) * 8 - total_bits;
	return (*s >> ss) & mask;
}

template<typename T>
void sort3(T* a, T* e, bool(lt)(const T&, const T&))
{
	// sort up to 3 items directly

	if (e - a <= 1) return;

	if (e - a == 3)
	{
		int i = lt(a[0], a[1]);
		if (lt(a[2], a[i])) { std::swap(a[i], a[2]); }
	}

	if (lt(a[1], a[0])) { std::swap(a[0], a[1]); }
}

template<typename T>
void sort3(T* a, T* e)
{
	// sort up to 3 items directly

	if constexpr (has_operator_lt<T>) return sort3(a, e, [](const T& a, const T& b) { return lt(a, b); });
	if constexpr (!has_operator_star<T>) return sort3(a, e, [](const T& a, const T& b) { return a < b; });
	if constexpr (has_operator_lt<typename std::remove_pointer_t<T>>)
		return sort3(a, e, [](const T& a, const T& b) { return lt(*a, *b); });
	else return sort3(a, e, [](const T& a, const T& b) { return *a < *b; });
}

template<typename T>
bool all_equal(T* a, T* e)
{
	// test whether all elements in range [a .. [e are equal

	using xT = typename std::remove_pointer_t<T>;

	T* p = a + 1;

	// clang-format off
	if constexpr      (has_operator_eq<T>)    while (p < e && eq(*a, *p)) p++;
	else if constexpr (!has_operator_star<T>) while (p < e && *a == *p) p++;
	else if constexpr (has_operator_eq<xT>)   while (p < e && eq(**a, **p)) p++;
	else                                      while (p < e && **a == **p) p++;
	// clang-format on

	return p == e;
}


/* ————————————————————————————————————————————————————————————————————————
   bucket sort
   type T must implement:
   - ordinal(T, uint total_bits, uint mask)   
   - either all_equal() or eq() or operator==()
   - either sort3() or lt() or operator<()
*/
template<typename T, typename INDEX = uint, uint BITS = min(8u, sizeof(INDEX) * 2)>
void bucket_sort(T* a, T* e)
{
	static_assert(BITS == 2 || BITS == 4 || BITS == 8);

	constexpr uint num_buckets = 1 << BITS;
	constexpr uint mask		   = num_buckets - 1;

	if (e - a <= 3) { return sort3(a, e); }

	struct Bucket
	{
		T* a;
		T* e;
	};

	struct PushedBucket
	{
		T*	 a;
		T*	 e;
		uint total_bits;
		uint size() { return e - a; }
	};

	auto push_bucket = [](PushedBucket buckets[], uint& num_buckets, T* a, T* e, uint total_bits) //
	{
		// sort 0 .. 3 items directly:

		if (e - a <= 3) { return sort3(a, e); }

		// check whether all items are the same because this would lead to infinite recursion:

		if (all_equal(a, e)) return;

		// insert into bucket stack acc. to size:
		// smaller buckets must be popped and sorted first to avoid overflow of stack!

		uint n = e - a;
		uint i = num_buckets++;
		while (i > 0 && n > buckets[i - 1].size)
		{
			buckets[i] = buckets[i - 1];
			i--;
		}
		buckets[i].a		  = a;
		buckets[i].e		  = e;
		buckets[i].total_bits = total_bits;
	};


	uint total_bits = BITS;

	constexpr uint max_pushed_buckets = 4 * num_buckets;
	uint		   num_pushed_buckets = 0;
	PushedBucket   pushed_buckets[max_pushed_buckets];


	for (;;)
	{
		Bucket buckets[num_buckets];
		INDEX  sizes[num_buckets];
		memset(sizes, 0, sizeof(sizes));

		// count items per bucket:

		for (T* p = a; p < e; p++) sizes[ordinal(*p, total_bits, mask)]++;

		// convert sizes to start indexes of buckets:

		T* start = a;
		for (uint i = 0; i < num_buckets; i++)
		{
			buckets[i].a = start;
			start += sizes[i];
			buckets[i].e = start;
		}

		// sort:

		for (uint idx = 0; idx < num_buckets; idx++)
		{
			for (T *p = buckets[idx].a, *end = buckets[idx].e; p < end; p++)
			{
				uint ox, o = ordinal(*p, total_bits, mask);
				while (o != idx) // while p is in the wrong bucket
				{
					T* px = buckets[o].a; // find wrongly placed px in bucket o (must exist)
					while (((ox = ordinal(*px, total_bits, mask))) == o) { px++; }
					buckets[o].a = px + 1; // skip over known correctly placed

					// store p into correct bucket in place of a wrongly placed px there
					// retrieve wrongly placed px back into p
					// resume until p goes into the current bucket
					std::swap(*p, *px);
					o = ox;
				}
			}
		}

		// push buckets on stack to sort recursively:

		start = a; // needed because buckets[].a is garbled
		for (uint idx = 0; idx < num_buckets; idx++)
		{
			T* end = buckets[idx].e;
			push_bucket(pushed_buckets, num_pushed_buckets, start, end, total_bits + BITS);
			start = end;
		}

		// pop next bucket to work on from stack:

		if unlikely (num_pushed_buckets == 0) return;

		PushedBucket& b = pushed_buckets[--num_pushed_buckets];
		a				= b.a;
		e				= b.e;
		total_bits		= b.total_bits;
	}
}


} // namespace kio
