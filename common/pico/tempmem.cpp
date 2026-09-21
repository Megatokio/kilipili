// Copyright (c) 2008 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "tempmem.h"
#include "cdefs.h"
#include <cstring>
#include <new>
#include <pico/sync.h>


namespace kilipili
{

char emptystr[1] = {'\0'};

// initial pool sizes and size increments:
static constexpr uint size0 = 800; // core 0
static constexpr uint size1 = 100; // core 1

template<uint SZ = 8>
struct TPool
{
	TPool(uint16 sz, TPool* prev) noexcept : prev(prev), size(sz), free(sz) {}
	ptr	 current_position() noexcept { return &data[free]; }
	bool contains(cptr s) noexcept { return size_t(s - data) <= size; }

	TPool* prev;
	uint16 size;
	uint16 free;
	char   data[SZ];
};

using Pool = TPool<8>;

static union
{
	Pool  pool0 {size0, nullptr};
	uchar xxx[size0 + sizeof(TPool<0>)];
};
static union
{
	Pool  pool1 {size1, nullptr};
	uchar yyy[size1 + sizeof(TPool<0>)];
};

static Pool* pools[2] = {&pool0, &pool1};						  // current pools
static ptr	 saves[2] = {&pool0.data[size0], &pool1.data[size1]}; // TempMemSave


// ***********************************************************************


static void restore_tempmem(ptr* const new_save = nullptr) noexcept
{
	// purge tempmem up to the save position:

	uint core = get_core_num();
	ptr	 save = saves[core];
	if (new_save) saves[core] = *new_save;
	Pool*& pool = pools[core];

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
	uint		   core			  = get_core_num();
	Pool*&		   pool			  = pools[core];
	constexpr uint alignment_mask = sizeof(ptr) - 1;

	if (sz > pool->free)
	{
		uint len = core ? size1 : size0;
		if (sz * 2 > len) len = (sz + alignment_mask) & ~alignment_mask;

		Pool* p = reinterpret_cast<Pool*>(malloc(sizeof(TPool<0>) + len));
		if (p == nullptr) throw OUT_OF_MEMORY;
		pool = new (p) Pool(uint16(len), pool);
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

	uint len = uint(strlen(s)) + 1;
	str	 z	 = new char[len];
	memcpy(z, s, len);
	return z;
}

void purge_tempmem() noexcept
{
	// reset tempmem to the last save position:

	restore_tempmem();
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

	uint core	= get_core_num();
	old_save	= saves[core];
	saves[core] = pools[core]->current_position();
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

str TempMemSave::xdupstr(cstr s)
{
	// purge tempmem and copy the (presumably temp) string before the save position
	// so that the string survives when this TempMemSave is destroyed.

	if unlikely (s == nullptr) return nullptr;
	if unlikely (s[0] == 0) return emptystr;

	uint   core	 = get_core_num();
	Pool*& pool	 = pools[core];
	ptr&   save	 = saves[core];
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

	assert(size_t(save - pool->data) == uint16(save - pool->data));
	pool->free = uint16(save - pool->data);

	size_t slen = strlen(s) + 1;
	save		= alloc_tempmem(slen, false); // this does not write anything into data[]
	memmove(save, s, slen);					  // s may be -unprotected- in the current pool
	free(qpool);
	return save;
}

} // namespace kilipili


/*














































*/
