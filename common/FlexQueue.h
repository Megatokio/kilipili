// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/cdefs.h"
#include "standard_types.h"
#include <type_traits>
#include <utility>


namespace kilipili
{

/**
 *  The FlexQueue template class provides an automatically growing queue.
 *	If INLINE_SIZE != 0 then initial allocation is provided in the FlexQueue object itself.
 *	The current implementation does not shrink.
 *	This implementation is not thread safe, esp. when put() needs to grow the buffer.
 */
template<typename T, uint INLINE_SIZE = 0>
class FlexQueue
{
	static_assert((INLINE_SIZE & (INLINE_SIZE - 1)) == 0, "size must be a power of 2");

protected:
	static const int DEFAULT_SIZE = INLINE_SIZE ? INLINE_SIZE : 32;
	using IDX					  = uint;

	T*	 data = nullptr;
	IDX	 size = DEFAULT_SIZE;
	IDX	 mask = DEFAULT_SIZE - 1;
	IDX	 rp	  = 0; // only modified by reader
	IDX	 wp	  = 0; // only modified by writer
	char inline_data[INLINE_SIZE * sizeof(T)];

public:
	FlexQueue() { data = reinterpret_cast<T*>(INLINE_SIZE ? inline_data : malloc(DEFAULT_SIZE * sizeof(T))); }
	~FlexQueue()
	{
		while (rp != wp) { data[rp++ & mask].~T(); }
		if (ptr(data) != inline_data) ::free(data);
	}

	IDX	 avail() const noexcept { return wp - rp; }
	IDX	 free() const noexcept { return size - avail(); }
	void flush() noexcept { rp = wp; }

	void grow() throws // -> double the buffer size
	{
		assert(wp == IDX(rp + size));

		int new_size = size * 2;
		int new_mask = new_size - 1;
		T*	newdata;

		if (std::is_trivially_move_constructible_v<T> && ptr(data) != inline_data)
		{
			newdata = reinterpret_cast<T*>(realloc(data, new_size * sizeof(T)));
			if (!newdata) throw OUT_OF_MEMORY;

			memcpy(&newdata[size], &newdata[0], size * sizeof(T));
		}
		else // not std::is_trivially_move_constructible_v<T>
		{
			T* olddata = data;
			newdata	   = reinterpret_cast<T*>(malloc(new_size * sizeof(T)));
			if (!newdata) throw OUT_OF_MEMORY;

			if ((wp & mask) == (wp & new_mask)) // wp stick, rp moved to new space
			{
				for (int i = 0; i < (wp & mask); i++) { new (&newdata[i]) T(std::move(olddata[i])); }
				for (int i = wp & mask; i < size; i++) { new (&newdata[i + size]) T(std::move(olddata[i])); }
			}
			else // wp move to new space, rp stick
			{
				for (int i = 0; i < (wp & mask); i++) { new (&newdata[i + size]) T(std::move(olddata[i])); }
				for (int i = wp & mask; i < size; i++) { new (&newdata[i]) T(std::move(olddata[i])); }
			}

			if (ptr(olddata) != inline_data) ::free(olddata);
		}

		data = newdata;
		mask = new_mask;
		size = new_size;
	}

	T& peek() noexcept
	{
		assert(avail());
		return data[rp & mask];
	}
	void drop() noexcept
	{
		assert(avail());
		rp += 1;
	}
	T get() noexcept
	{
		assert(avail());
		return std::move(data[rp++ & mask]);
	}
	void put(const T& c) noexcept
	{
		if unlikely (!free()) grow();
		new (&data[wp++ & mask]) T(std::move(c));
	}
};


} // namespace kilipili

/*











































*/
