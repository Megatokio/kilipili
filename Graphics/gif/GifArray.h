// Copyright (c) 2007 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

//	template class GifArray
//	-----------------------
//
//	Manage a dynamically allocated array of type T
//
//	-	delete[] data when GifArray is destroyed
//	-	index validation in operator[]
//	-	resizing
//
//	Restrictions:
//
//	-	max. number of elements in array = max(int)
//		byte-size of array is not restricted!
//
//	-	data type T must have 'flat' default ctor, dtor, and copy ctor:
//
//		-	default ctor:	data is cleared to 0
//		-	destructor:		no action
//		-	copy ctor:		flat copy


#pragma once
#include "cdefs.h"
#include "standard_types.h"
#include <string.h>


namespace kio::Graphics
{

template<class T>
class GifArray
{
public:
	GifArray() noexcept { init(); }
	GifArray(const GifArray& q) throws { init(q); }
	GifArray(const GifArray* q) throws { init(*q); }
	GifArray(const T* q, int n) throws { init(q, n); }
	GifArray(int n) throws { init(n); }
	~GifArray() noexcept { kill(); }

	GifArray& operator=(const GifArray& q) throws
	{
		if (this != &q)
		{
			kill();
			init(q);
		}
		return *this;
	}

	GifArray& operator=(T q) throws
	{
		kill();
		init(&q, 1);
		return *this;
	}


	/* Access data members:
		Size()		get item count
		Data()		get array base address
		operator[i]	access item i. checks index during development.
	*/
	int		 Count() const noexcept { return count; }
	T*		 Data() noexcept { return array; }
	const T* Data() const noexcept { return array; }
	T&		 operator[](int i) noexcept
	{
		assert(uint(i) < uint(count));
		return array[i];
	}
	const T& operator[](int i) const noexcept
	{
		assert(uint(i) < uint(count));
		return array[i];
	}


	/* Resize and clear:
		Shrink		shrink if new_count < count
		Grow		grow   if new_count > count
		Purge		purge data and resize to 0
		Clear		overwrite array with 0s.
		Clear(T)	overwrite array with Ts.
	*/
	void Shrink(int new_count) noexcept;
	void Grow(int new_count) throws /*bad alloc*/;
	void Purge() noexcept
	{
		kill();
		init();
	}
	void Clear() noexcept { clear(array, count); }
	void Clear(T q) noexcept
	{
		T* p = array;
		T* e = p + count;
		while (p < e) *p++ = q;
	}

	/* Concatenate:
		Append		append user-supplied data to this array. if bad_alloc is thrown, then GifArray remained unmodified.
		operator+=	append other array to my array. if bad_alloc is thrown, then GifArray remained unmodified.
		operator=	return concatenation of this array and other array. throws bad_alloc if malloc fails.
	*/
	GifArray& Append(const T* q, int n) throws /*bad alloc*/;
	GifArray& operator+=(const GifArray& q) throws /*bad alloc*/ { return Append(q.array, q.count); }
	GifArray operator+(const GifArray& q) const throws /*bad alloc*/ { return GifArray(this).Append(q.array, q.count); }


private:
	static size_t sz(size_t n) noexcept { return n * sizeof(T); }				  // convert item count to memory size
	static void	  copy(T* z, const T* q, int n) noexcept { memcpy(z, q, sz(n)); } // flat copy n items
	static void	  clear(T* z, int n) noexcept { memset(z, 0, sz(n)); }			  // clear n items with 0
	static T*	  malloc(int n) throws { return n ? new T[n] : nullptr; }		  // allocate memory for n items
	static T*	  newcopy(const T* q, int n) throws
	{
		if (!n) return 0;
		T* z = new T[n];
		copy(z, q, n);
		return z;
	}
	static T* newcleared(int n) throws
	{
		if (!n) return 0;
		T* z = new T[n];
		clear(z, n);
		return z;
	}
	static void release(T* array) noexcept
	{
		delete[] array;
		array = 0;
	}
	void init() noexcept // init empty array
	{
		array = 0;
		count = 0;
	}
	void init(int n) throws // alloc and clear array
	{
		array = 0;
		count = n;
		array = newcleared(n);
	}
	void init(const T* q, int n) throws // init with copy of caller-supplied data
	{
		array = 0;
		count = n;
		array = newcopy(q, n);
	}
	void init(const GifArray& q) throws { init(q.array, q.count); } // init with copy
	void kill() noexcept { release(array); }						// deallocate allocateded data

protected:
	T*	array; // -> array[0].	array *may* be NULL for empty array.
	int count; // item count.	count==0 for empty array.
};


template<class T>
GifArray<T>& GifArray<T>::Append(const T* q, int n) throws
{
	/*	Append caller-supplied items to my array.
		does nothing if item count == 0.
		can be called to append subarray of own array!
		if bad_alloc is thrown, then this GifArray remained unmodified.
	*/
	if (n > 0)
	{
		T*	old_array = array;
		int old_count = count;
		array		  = malloc(old_count + n);
		count		  = old_count + n;

		copy(array, old_array, old_count); // copy own data
		copy(array + old_count, q, n);	   // copy other data. may be subarray of my own array!
		release(old_array);				   // => release own data after copying other data!
	}
	return *this;
}


template<class T> /* verified 2006-06-09 kio */
void GifArray<T>::Shrink(int new_count) noexcept
{
	/*	Shrink GifArray:
		does nothing if requested new_count >= current count
		purges ( deallocates ) data if new_count == 0.
		silently handles new_count < 0 as new_count == 0.
		silently keeps the old data if data reallocation fails.
	*/
	if (new_count >= count) return; // won't shrink
	if (new_count <= 0)
	{
		Purge();
		return;
	} // empty array

	count = new_count;
	try // try to reallocate data
	{
		T* old_array = array;
		array		 = newcopy(old_array, new_count);
		release(old_array);
	}
	catch (Error)
	{} // but just don't do it if reallocation fails
}


template<class T>
void GifArray<T>::Grow(int new_count) throws
{
	/*	Grow GifArray:
		does nothing if requested new_count <= current count.
		if bad_alloc is thrown, then GifArray remained unmodified.
	*/
	if (new_count <= count) return;

	int old_count = count;
	T*	old_array = array;

	array = malloc(new_count);
	count = new_count;

	copy(array, old_array, old_count);
	clear(array + old_count, new_count - old_count);

	release(old_array);
}

} // namespace kio::Graphics

/*



































*/
