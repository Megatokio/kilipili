#pragma once
// Copyright (c) 2010 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "basic_math.h"
#include "cdefs.h"
#include "cstrings.h"
#include "relational_operators.h"
#include "sort.h"
#include "standard_types.h"
#include "template_helpers.h"
#include <memory>
#include <stdexcept>
#include <string.h>
#include <type_traits>

namespace kio
{

#define ref_or_value(T) select_type<std::is_scalar_v<T>, T, const T&>


/**	General Array 

	· new items are initialized with their ctor; scalar items are initialized with zero.
	· operator[] etc. abort on failed index check!
	· item's eq(), ne(), lt() should be implemented for operator==() and sort() etc.
	· specializations for Array<str> and Array<cstr> with allocation in TempMem.
*/


template<typename T>
class Array
{
protected:
	uint max  = 0;
	uint cnt  = 0;
	T*	 data = nullptr;

	static T*	_malloc(uint n) throws;
	static void _free(T* data) noexcept;
	static void _move_up(T* z, const T* q, uint n) noexcept;
	static void _move_dn(T* z, const T* q, uint n) noexcept;
	static void _init_with_copy(T* z, const T* q, uint n);
	static void _init_with_move(T* z, T* q, uint n) noexcept;
	static void _init(T* a, uint n) noexcept;
	static void _kill(T* a, uint n) noexcept;
	void		_growmax(uint newmax) throws;
	void		_shrinkmax(uint newmax) throws;
	static void _init(T& a) noexcept;
	static void _kill(T& a) noexcept { a.~T(); }


public:
	// see https://stackoverflow.com/questions/11562/how-to-overload-stdswap
	static void swap(Array& a, Array& b) noexcept
	{
		std::swap(a.cnt, b.cnt);
		std::swap(a.max, b.max);
		std::swap(a.data, b.data);
	}

	~Array() noexcept
	{
		_kill(data, cnt);
		_free(data);
	}
	Array() noexcept = default;

	explicit Array(uint cnt, uint max = 0) throws;
	Array(const T* q, uint n) throws;
	Array(const Array& q) throws : Array(q.data, q.cnt) {}
	Array(Array&& q) noexcept : max(q.max), cnt(q.cnt), data(q.data)
	{
		q.max  = 0;
		q.cnt  = 0;
		q.data = nullptr;
	}

	Array& operator=(const Array& q) throws { return operator=(std::move(Array(q))); }
	Array& operator=(Array&& q) noexcept
	{
		swap(*this, q);
		return *this;
	}

	Array copyofrange(uint a, uint e) const throws;

	// access data members:
	const uint& count() const noexcept { return cnt; }
	const T*	getData() const noexcept { return data; }
	T*			getData() noexcept { return data; }

	const T& operator[](uint i) const noexcept
	{
		assert_lt(i, cnt);
		return data[i];
	}
	T& operator[](uint i) noexcept
	{
		assert_lt(i, cnt);
		return data[i];
	}
	const T& operator[](int i) const noexcept
	{
		assert_lt(uint(i), cnt);
		return data[i];
	}
	T& operator[](int i) noexcept
	{
		assert_lt(uint(i), cnt);
		return data[i];
	}
	const T& first() const noexcept
	{
		assert(cnt);
		return data[0];
	}
	T& first() noexcept
	{
		assert(cnt);
		return data[0];
	}
	const T& last() const noexcept
	{
		assert(cnt);
		return data[cnt - 1];
	}
	T& last() noexcept
	{
		assert(cnt);
		return data[cnt - 1];
	}

	bool operator==(const Array& q) const noexcept; // uses ne()
	bool operator!=(const Array& q) const noexcept; // uses ne()
	bool operator<(const Array& q) const noexcept;	// uses eq() and lt()
	bool operator>(const Array& q) const noexcept;	// uses eq() and gt()
	bool operator>=(const Array& q) const noexcept { return !operator<(q); }
	bool operator<=(const Array& q) const noexcept { return !operator>(q); }

	uint indexof(ref_or_value(T) item) const noexcept; // compare using '==' except str/cstr: 'eq'
	bool contains(ref_or_value(T) item) const noexcept { return indexof(item) != ~0u; } // uses indexof()

	// resize:
	void growmax(uint newmax) throws { _growmax(newmax); }
	T&	 grow() throws
	{
		_growmax(cnt + 1);
		_init(data[cnt]);
		return data[cnt++];
	}
	void grow(uint cnt, uint max) throws;
	void grow(uint newcnt) throws;
	void shrink(uint newcnt) noexcept;
	void resize(uint newcnt) throws { grow(newcnt), shrink(newcnt); }

	void drop() noexcept
	{
		assert(cnt);
		data[--cnt].~T();
	}
	T pop() noexcept
	{
		assert(cnt);
		return std::move(data[--cnt]);
	}
	void purge() noexcept
	{
		_kill(data, cnt);
		_free(data);
		max = cnt = 0;
		data	  = nullptr;
	}
	T& append(T q) throws
	{
		_growmax(cnt + 1);
		new (&data[cnt]) T(std::move(q));
		return data[cnt++];
	}
	void appendifnew(T q) throws // uses indexof()
	{
		if (!contains(q)) append(std::move(q));
	}
	Array& operator<<(T q) throws
	{
		append(std::move(q));
		return *this;
	}
	void append(const T* q, uint n) throws
	{
		_growmax(cnt + n);
		_init_with_copy(data + cnt, q, n);
		cnt += n;
	}
	void append(const Array& q) throws
	{
		// don't call append(q.data,n) => appending to itself works!
		_growmax(cnt + q.cnt);
		_init_with_copy(data + cnt, q.data, q.cnt);
		cnt += q.cnt;
	}

	void removeitem(ref_or_value(T) item, bool fast = 0) noexcept;
	void removeat(uint idx, bool fast = 0) noexcept;
	void removerange(uint a, uint e) noexcept;

	// define overloaded remove() for item and idx:
	//	 void remove (item, bool);
	//	 void remove (idx, bool);
	// for fundamental types this is ambiguous and cannot even be defined for T=uint
	// we cannot make the definitions optional, based on type T
	//   -> make unusable definitions for fundamental types!
	//   -> make usable definitions for non_fundamental types!
	struct Foo1
	{
		Foo1() = delete;
	};
	struct Foo2
	{
		Foo2() = delete;
	};
	void remove(select_type<std::is_fundamental_v<T>, Foo1, ref_or_value(T)> item, bool fast = 0) noexcept
	{
		removeitem(item, fast);
	}
	void remove(select_type<std::is_fundamental_v<T>, Foo2, uint> idx, bool fast = 0) noexcept { removeat(idx, fast); }

	void insertat(uint idx, T) throws;
	void insertat(uint idx, const T* q, uint n) throws;
	void insertat(uint idx, const Array&) throws;
	void insertrange(uint a, uint e) throws;
	void insertsorted(T) throws; // uses lt()

	void revert(uint a, uint e) noexcept;  // revert items in range [a..[e
	void rol(uint a, uint e) noexcept;	   // roll left  range  [a..[e
	void ror(uint a, uint e) noexcept;	   // roll right range  [a..[e
	void shuffle(uint a, uint e) noexcept; // shuffle range [a..[e
	void sort(uint a, uint e) noexcept
	{
		if (e > cnt) e = cnt;
		if (a < e) kio::sort(data + a, data + e);
	}
	void rsort(uint a, uint e) noexcept
	{
		if (e > cnt) e = cnt;
		if (a < e) kio::rsort(data + a, data + e);
	}
	void sort(uint a, uint e, compare_fu(T) lt) noexcept
	{
		if (e > cnt) e = cnt;
		if (a < e) kio::sort(data + a, data + e, lt);
	}

	void revert() noexcept { revert(0, cnt); }
	void rol() noexcept { rol(0, cnt); }
	void ror() noexcept { ror(0, cnt); }
	void shuffle() noexcept { shuffle(0, cnt); }
	void sort() noexcept
	{
		if (cnt) kio::sort(data, data + cnt);
	} // uses lt()
	void rsort() noexcept
	{
		if (cnt) kio::rsort(data, data + cnt);
	} // uses gt()
	void sort(compare_fu(T) lt) noexcept
	{
		if (cnt) kio::sort(data, data + cnt, lt);
	}

	void swap(uint i, uint j) noexcept
	{
		assert(i < cnt && j < cnt);
		std::swap(data[i], data[j]);
	}
};


// -----------------------------------------------------------------------
//					  I M P L E M E N T A T I O N S
// -----------------------------------------------------------------------

template<typename T>
void Array<T>::_init(T* a, uint n) noexcept
{
	if constexpr (std::is_scalar_v<T>) memset(a, 0, n * sizeof(T)); // arith, enum, ptr
	else
		while (n--) { new (a) T; }
}

template<typename T>
void Array<T>::_init(T& a) noexcept
{
	if constexpr (std::is_scalar_v<T>) a = T(0); // arith, enum, ptr
	else new (&a) T;
}

template<typename T>
void Array<T>::_init_with_copy(T* z, const T* q, uint n)
{
	// initialize data from some other source

	while (n--) { new (z++) T(*q++); }
}

template<typename T>
void Array<T>::_init_with_move(T* z, T* q, uint n) noexcept
{
	// initialize data from some other source

	while (n--) { new (z++) T(std::move(*q++)); }
}

template<typename T>
void Array<T>::_kill(T* a, uint n) noexcept
{
	while (n--) { a[n].~T(); }
}

template<typename T>
T* Array<T>::_malloc(uint n) throws // static
{
	// allocate uninitialized memory

	if (T* rval = reinterpret_cast<T*>(malloc(n * sizeof(T)))) return rval;
	else throw OUT_OF_MEMORY;
}

template<typename T>
void Array<T>::_free(T* data) noexcept // static
{
	// deallocate uninitialized memory

	free(data);
}

template<typename T>
void Array<T>::_move_dn(T* z, const T* q, uint n) noexcept // static
{
	// move data from higher or non-overlapping source

	while (n--) { *z++ = std::move(*q++); }
}

template<typename T>
void Array<T>::_move_up(T* z, const T* q, uint n) noexcept // static
{
	// move data from lower or non-overlapping source

	while (n--) { z[n] = std::move(q[n]); }
}


template<typename T>
void Array<T>::_growmax(uint newmax) throws
{
	// grow data[]
	// grow only

	if (newmax > max)
	{
		if constexpr (std::is_trivially_move_constructible_v<T>)
		{
			if (T* newdata = reinterpret_cast<T*>(realloc(data, newmax * sizeof(T))))
			{
				data = newdata;
				max	 = newmax;
			}
			else throw OUT_OF_MEMORY;
		}
		else
		{
			newmax += newmax / 8 + 4;
			T* newdata = _malloc(newmax);
			_init_with_move(newdata, data, cnt); // if T has custom copy_ctor
			_kill(data, cnt);					 // if T has custom dtor
			_free(data);
			data = newdata;
			max	 = newmax;
		}
	}
}

template<typename T>
void Array<T>::_shrinkmax(uint newmax) throws
{
	// shrink data[]
	// update max

	assert(newmax >= cnt);

	if (newmax < max)
	{
		void* p = realloc(data, newmax * sizeof(T));
		assert(p == data);
		max = newmax;
	}
}

template<typename T>
Array<T>::Array(uint cnt, uint max) throws : Array()
{
	if (max < cnt) max = cnt;
	data	  = _malloc(max);
	this->max = max;
	_init(data, cnt);
	this->cnt = cnt;
}

template<typename T>
Array<T>::Array(const T* q, uint n) throws
{
	data = _malloc(n);
	max	 = n;
	_init_with_copy(data, q, n);
	cnt = n;
}

template<typename T>
Array<T> Array<T>::copyofrange(uint a, uint e) const throws
{
	// create a copy of a range of data of this

	if (e > cnt) e = cnt;
	if (a < e) return Array(data + a, e - a);
	else return Array();
}

template<typename T>
bool Array<T>::operator==(const Array<T>& q) const noexcept
{
	// compare arrays
	// used ne()

	if (cnt != q.cnt) return false;
	for (uint i = cnt; i--;)
	{
		if (ne(data[i], q.data[i])) return false;
	}
	return true;
}

template<typename T>
bool Array<T>::operator!=(const Array<T>& q) const noexcept
{
	// compare arrays
	// used ne()

	if (cnt != q.cnt) return true;
	for (uint i = cnt; i--;)
	{
		if (ne(data[i], q.data[i])) return true;
	}
	return false;
}

template<typename T>
bool Array<T>::operator<(const Array& q) const noexcept
{
	// compare arrays
	// uses eq() and lt()

	uint end = min(cnt, q.cnt);
	uint i	 = 0;
	while (i < end && eq(data[i], q.data[i])) { i++; }
	if (i < end) return lt(data[i], q.data[i]);
	else return cnt < q.cnt;
}

template<typename T>
bool Array<T>::operator>(const Array& q) const noexcept
{
	// compare arrays
	// uses eq() and gt()

	uint end = min(cnt, q.cnt);
	uint i	 = 0;
	while (i < end && eq(data[i], q.data[i])) { i++; }
	if (i < end) return gt(data[i], q.data[i]);
	else return cnt > q.cnt;
}

template<typename T>
uint Array<T>::indexof(ref_or_value(T) item) const noexcept
{
	// find first occurance
	// using == (find pointers by identity)
	// or return ~0u

	for (uint i = 0; i < cnt; i++)
	{
		if (data[i] == item) return i;
	}
	return ~0u;
}

template<>
inline uint Array<cstr>::indexof(cstr item) const noexcept
{
	// specializations for str and cstr
	// use eq() --> compare string contents

	for (uint i = 0; i < cnt; i++)
	{
		if (eq(data[i], item)) return i;
	}
	return ~0u;
}

template<>
inline uint Array<str>::indexof(str item) const noexcept
{
	// specializations for str and cstr
	// use eq() --> compare string contents

	for (uint i = 0; i < cnt; i++)
	{
		if (eq(data[i], item)) return i;
	}
	return ~0u;
}

template<typename T>
void Array<T>::grow(uint newcnt, uint newmax) throws
{
	// grow data[]
	// grow only
	//
	// newmax > max: grow data[]
	// newcnt > cnt: grow cnt and clear new items

	assert(newmax >= newcnt);

	_growmax(newmax);
	while (cnt < newcnt) { _init(data[cnt++]); }
}

template<typename T>
void Array<T>::grow(uint newcnt) throws
{
	// grow data[]
	// grow only

	grow(newcnt, newcnt);
}

template<typename T>
void Array<T>::shrink(uint newcnt) noexcept
{
	// shrink data[]
	// does nothing if new count ≥ current count
	// does not relocate data[]

	while (cnt > newcnt) data[--cnt].~T();
	_shrinkmax(cnt);
}

template<typename T>
void Array<T>::removeat(uint idx, bool fast) noexcept
{
	// remove item at index
	// idx < cnt

	assert_lt(idx, cnt);

	if (idx < --cnt)
	{
		if (fast) { data[idx] = std::move(data[cnt]); }
		else { _move_dn(data + idx, data + idx + 1, cnt - idx); }
	}

	data[cnt].~T();
}

template<typename T>
void Array<T>::removeitem(ref_or_value(T) item, bool fast) noexcept
{
	// find first occurance and remove it
	// uses indexof()
	// => uses == (find pointers by identity)
	//    uses eq() for str and cstr

	uint idx = indexof(item);
	if (idx != ~0u) removeat(idx, fast);
}

template<typename T>
void Array<T>::removerange(uint a, uint e) noexcept
{
	// remove range of data

	if (e > cnt) e = cnt;
	if (a >= e) return;

	uint n = e - a;
	_move_dn(data + a, data + e, cnt - e);
	_kill(data + cnt - n, n);
	cnt -= n;
}

template<typename T>
void Array<T>::insertat(uint idx, T t) throws
{
	// insert item at index
	// idx ≤ cnt

	assert(idx <= cnt);

	_growmax(cnt + 1);
	_init(data[cnt]);
	_move_up(data + idx + 1, data + idx, cnt - idx);
	cnt++;
	data[idx] = std::move(t);
}

template<typename T>
void Array<T>::insertsorted(T q) throws
{
	uint i;
	for (i = cnt; i > 0 && lt(q, data[i - 1]); i--) {}
	insertat(i, std::move(q));
}

template<typename T>
void Array<T>::insertat(uint idx, const T* q, uint n) throws
{
	// insert source array at index
	// idx ≤ cnt

	assert(idx <= cnt);
	if (n == 0) return;

	_growmax(cnt + n);
	_init(data + cnt, n);
	_move_up(data + idx + n, data + idx, cnt - idx);
	cnt += n;
	_kill(data + idx, n);
	_init_with_copy(data + idx, q, n);
}

template<typename T>
void Array<T>::insertat(uint idx, const Array& q) throws
{
	// insert source array at index
	// idx ≤ cnt

	assert(this != &q);

	insertat(idx, q.data, q.cnt);
}

template<typename T>
void Array<T>::insertrange(uint a, uint e) throws
{
	// insert space into data
	// a ≤ cnt

	assert(a <= cnt);
	if (a >= e) return;

	uint n = e - a;
	_growmax(cnt + n);
	_init(data + cnt, n);
	_move_up(data + e, data + a, cnt - a);
	_kill(data + a, n);
	_init(data + a, n);
	cnt += n;
}

template<typename T>
void Array<T>::revert(uint a, uint e) noexcept
{
	// revert order of items in data[]

	if (e > cnt) e = cnt;
	if (a >= e) return;

	T* pa = data + a;
	T* pe = data + e - 1;

	while (pa < pe) { std::swap(*pa++, *pe--); }
}

template<typename T>
void Array<T>::rol(uint a, uint e) noexcept
{
	// roll left range [a..[e

	if (e > cnt) e = cnt;
	if (a >= e) return;

	T z(std::move(data[a]));
	_move_dn(&data[a], &data[a + 1], e - a - 1);
	data[e - 1] = std::move(z);
}

template<typename T>
void Array<T>::ror(uint a, uint e) noexcept
{
	// roll right range [a..[e

	if (e > cnt) e = cnt;
	if (a >= e) return;

	T z(std::move(data[e - 1]));
	_move_up(&data[a + 1], &data[a], e - a - 1);
	data[a] = std::move(z);
}

template<typename T>
void Array<T>::shuffle(uint a, uint e) noexcept
{
	// shuffle data in range [a..[e

	if (e > cnt) e = cnt;
	if (a >= e) return;

	T*	 p = data + a;
	uint n = e - a;

	for (uint i = 0; i < n; i++) { std::swap(p[i], p[random() % n]); }
}

} // namespace kio


// 1-line description of array for debugging and logging:

template<typename T>
inline str tostr(const kio::Array<T>& array)
{
	return usingstr("Array<T>[%u]", array.count());
}

inline str tostr(const kio::Array<cstr>& array) { return kio::usingstr("Array<cstr>[%u]", array.count()); }
inline str tostr(const kio::Array<str>& array) { return kio::usingstr("Array<str>[%u]", array.count()); }


/* 
























*/
