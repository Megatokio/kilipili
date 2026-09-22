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

namespace kilipili
{

char emptystr[1] = {'\0'};

using poolsize_t = select_type<sizeof(ptr) == 8, uint32, uint16>;

struct Pool
{
	Pool(poolsize_t sz, Pool* prev) noexcept : prev(prev), size(sz), free(sz) {}
	ptr	 current_position() noexcept { return &data[free]; }
	bool contains(cptr s) noexcept { return size_t(s - data) <= size; }

	Pool*	   prev;
	poolsize_t size;
	poolsize_t free;
	char	   data[8];
};

template<poolsize_t SZ>
struct PoolSZ : public Pool
{
	PoolSZ() noexcept : Pool(SZ, nullptr) {}
	char more_data[SZ - sizeof(data)];
};


#if defined MAKE_TOOLS

static constexpr poolsize_t std_pool_size = 8000;
static thread_local Pool*	current_pool  = new PoolSZ<std_pool_size>();
static thread_local ptr		save_position = current_pool->data + std_pool_size;

#else

// initial pool sizes and size increments:
static constexpr poolsize_t size0 = 800; // core 0
static constexpr poolsize_t size1 = 100; // core 1

static PoolSZ<size0> pool0;
static PoolSZ<size1> pool1;

static Pool* pools[2] = {&pool0, &pool1};						  // current pools
static ptr	 saves[2] = {&pool0.data[size0], &pool1.data[size1]}; // last save position

  #define current_pool	pools[get_core_num()]
  #define save_position saves[get_core_num()]
  #define std_pool_size (get_core_num() ? size1 : size0)

#endif


// ***********************************************************************


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

	assert(size_t(save - pool->data) == uint16(save - pool->data));
	pool->free = uint16(save - pool->data);
}

static ptr alloc_tempmem(uint sz, bool aligned)
{
	Pool*&		   pool			  = current_pool;
	constexpr uint alignment_mask = sizeof(ptr) - 1;

	if (sz > pool->free)
	{
		uint len = std_pool_size;
		if (sz * 2 > len) len = (sz + alignment_mask) & ~alignment_mask;

		assert(poolsize_t(len) == len);
		Pool* p = reinterpret_cast<Pool*>(malloc(sizeof(Pool) - sizeof(Pool::data) + len));
		if (p == nullptr) throw OUT_OF_MEMORY;
		pool = new (p) Pool(poolsize_t(len), pool);
	}

	pool->free -= sz;
	if (aligned) pool->free &= ~alignment_mask;
	return pool->current_position();
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

str xdupstr(cstr s)
{
	// purge tempmem and copy the (presumably temp) string before the save position
	// so that the string survives when this TempMemSave is destroyed.

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
	pool->free = poolsize_t(save - pool->data);

	uint slen = uint(strlen(s) + 1);
	save	  = alloc_tempmem(poolsize_t(slen), false); // this does not write anything into data[]
	memmove(save, s, slen);								// s may be -unprotected- in the current pool
	free(qpool);
	return save;
}

} // namespace kilipili


/*














































*/
