// Copyright (c) 2008 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "tempmem.h"
#include "cdefs.h"
#include "template_helpers.h"
#include <cstring>
#include <new>
#if !defined MAKE_TOOLS
  #include <pico/sync.h>
#endif


extern "C" __weak_symbol bool mresize(void* mem, size_t size);


namespace kilipili
{

char emptystr[1] = {'\0'};

using poolsize_t = uint_with_size<sizeof(ptr) / 2>;

struct Pool
{
	Pool(poolsize_t sz, Pool* prev) noexcept : prev(prev), size(sz), used(0) {}
	ptr	 current_position() noexcept { return &data[used]; }
	ptr	 end() noexcept { return &data[size]; }
	bool contains(cptr s) noexcept { return size_t(s - data) <= size; }

	Pool*	   prev;
	poolsize_t size;
	poolsize_t used;
	char	   data[0];
};

template<poolsize_t SZ>
struct PoolSZ : public Pool
{
	PoolSZ() noexcept : Pool(SZ, nullptr) {}
	char more_data[SZ];
};


#if defined MAKE_TOOLS

static constexpr poolsize_t std_pool_size = 8000;
static thread_local Pool*	current_pool  = new PoolSZ<std_pool_size>();
static thread_local ptr		save_position = current_pool->data + std_pool_size;

#else

// initial pool sizes and size increments:
static constexpr poolsize_t std_pool_size = 400;

static PoolSZ<std_pool_size> pool0;
static PoolSZ<0>			 pool1;

static Pool* pools[2] = {&pool0, &pool1};				  // current pools
static ptr	 saves[2] = {&pool0.data[0], &pool1.data[0]}; // last save position

  #define current_pool	pools[get_core_num()]
  #define save_position saves[get_core_num()]

#endif


// ***********************************************************************

static constexpr uint alignment_mask = sizeof(ptr) - 1;
static constexpr uint roundup(uint n) { return (n + alignment_mask) & ~alignment_mask; }


static void restore_tempmem(ptr* const new_save) noexcept
{
	// purge tempmem up to the save position:

	ptr save = save_position;
	if (new_save) save_position = *new_save;
	Pool*& pool = current_pool;

	// while save position is not in the current_pool:
	while (!pool->contains(save))
	{
		assert(pool->prev); // save position not found?!?

		Pool* z = pool;
		pool	= pool->prev;
		free(z);
	}

	assert(size_t(save - pool->data) == poolsize_t(save - pool->data));
	pool->used = poolsize_t(save - pool->data);
}

static bool try_resize_pool(Pool* pool, uint sz)
{
	if (mresize == nullptr) return false;	 // can resize memory on heap?
	if (pool->prev == nullptr) return false; // the first block is not in heap

	size_t new_size = roundup(sz);
	if (poolsize_t(new_size) != new_size) return false;
	if (mresize(pool, sizeof(Pool) + new_size) == false) return false;

	pool->size = poolsize_t(new_size);
	return true;
}

static ptr alloc_tempmem(uint sz, bool aligned)
{
	assert(sz <= poolsize_t(~alignment_mask)); // => roundup(sz) fits in poolsize_t

	Pool*& pool = current_pool;
	if (aligned) pool->used = poolsize_t(roundup(pool->used));
	assert(pool->used <= pool->size);

	if (pool->used + sz > pool->size && try_resize_pool(pool, pool->used + sz) == false)
	{
		void* p		   = nullptr;
		uint  new_size = std_pool_size;
		if (new_size >= sz)
		{
			p = malloc(sizeof(Pool) + new_size); //
		}
		if (p == nullptr)
		{
			new_size = roundup(sz);
			assert(poolsize_t(new_size) == new_size);
			p = malloc(sizeof(Pool) + new_size);
			if (p == nullptr) throw OUT_OF_MEMORY;
		}
		pool = new (p) Pool(poolsize_t(new_size), pool);
	}

	ptr p = pool->current_position();
	pool->used += sz;
	return p;
}


// ***********************************************************************


str newstr(uint len)
{
	// allocate char[]
	// => deallocate with delete[]
	// presets terminating 0

	str z  = new char[len + 1];
	z[len] = 0;
	return z;
}

str newcopy(cstr s)
{
	// allocate char[]
	// => deallocate with delete[]
	// returns NULL if source string is NULL

	if unlikely (!s) return nullptr;

	size_t len = strlen(s) + 1;
	str	   z   = new char[len];
	memcpy(z, s, len);
	return z;
}

void purge_tempmem() noexcept
{
	// reset tempmem to the last save position:

	restore_tempmem(nullptr);
}

ptr tempstr(uint len)
{
	// Allocate a cstring in tempmem
	// the returned string is not aligned and may start on an odd address

	ptr s  = alloc_tempmem(len + 1, false);
	s[len] = 0;
	return s;
}

ptr tempmem(uint size)
{
	// allocate some memory in tempmem
	// the returned memory is aligned to pointer size

	return alloc_tempmem(size, true);
}

str dupstr(cstr s)
{
	// create copy of string in tempmem

	if unlikely (!s) return nullptr;
	if unlikely (*s == 0) return emptystr;

	uint len = uint(strlen(s));
	ptr	 z	 = alloc_tempmem(len + 1, false);
	memcpy(z, s, len + 1);
	return z;
}


// ***********************************************************************


TempMemSave::TempMemSave() noexcept
{
	// save current tempmem position for later restore:

	old_save	  = save_position;
	save_position = current_pool->current_position();
}

TempMemSave::~TempMemSave() noexcept
{
	// restore tempmem to saved position:

	restore_tempmem(&old_save);
}

void TempMemSave::purge() noexcept
{
	// reset tempmem to saved position:

	restore_tempmem(nullptr);
}

str xdupstr(cstr s, bool before_save_pos)
{
	// purge tempmem and
	// before_save_pos = true:  copy the (presumably temp) string before the save position
	//							so that the string survives when this TempMemSave is destroyed.
	// before_save_pos = false: copy the (presumably temp) string behind the save position
	//							making it the only string in the current pool

	if unlikely (s == nullptr) return nullptr;
	if unlikely (s[0] == 0) return emptystr;

	Pool*& pool	 = current_pool;
	ptr&   save	 = save_position;
	Pool*  qpool = nullptr;

	// purge pools until we reach the pool which contains the current save position.
	// keep the pool with the source string:
	while (!pool->contains(save))
	{
		assert(pool->prev); // save position not found?!?

		Pool* z = pool;
		pool	= pool->prev;
		if (z->contains(s)) qpool = z;
		else free(z);
	}

	assert(size_t(save - pool->data) == poolsize_t(save - pool->data));
	pool->used = poolsize_t(save - pool->data);

	uint slen = uint(strlen(s) + 1);
	ptr	 xs	  = alloc_tempmem(slen, false); // this does not write anything into data[]
	if (before_save_pos) save = pool->current_position();
	memmove(xs, s, slen); // s may be -unprotected- in the current pool
	free(qpool);
	return xs;
}

bool resize_tempstr(str s, uint new_len) noexcept
{
	// try to resize a string in-place:
	//		 string must be the last string in the tempmem
	//		 string must be behind the current save position
	//		 string must not contain char(0) except for the terminating null char
	// grow: the pool must have enough room for the new size or mresize() must succeed
	//
	// return true:  success
	// return false: failure

	// this function can help to reduce piling up of tempmem in some scenarios, e.g. line editor

	// example:
	//
	//	s = xdupstr(s,false); // makes s the only and thereby also last string behind the save position
	//	if(resize_tempstr(s,strlen(s)+1)==false) s=catstr(s," "); // if resize fails use catstr()

	Pool* pool	  = current_pool;
	uint  old_len = strlen(s);

	// string must be the last string:
	if (s + old_len + 1 != pool->current_position()) return false;

	// string must be behind the current save position:
	if (save_position == pool->current_position()) return false;

	// if not enough free room left in this pool then try to grow it:
	uint req_size = pool->used + new_len - old_len;
	if (req_size > pool->size && try_resize_pool(pool, req_size) == false) return false;
	assert(s + new_len + 1 <= pool->end());

	pool->used += new_len - old_len;
	s[new_len] = 0;
	return true;
}

} // namespace kilipili


/*














































*/
