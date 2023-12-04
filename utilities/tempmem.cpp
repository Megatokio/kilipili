// Copyright (c) 2008 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "tempmem.h"
#include "basic_math.h"
#include "kilipili_cdefs.h"
#include <cstring>
#include <pico/platform.h>

// this file provides tempmem buffers on the RP2040 (Raspberry Pico).

namespace kio
{

struct Block
{
	Block* prev;
	uint16 size;
	uint16 used;
	char   data[4];
};

Block* newBlock(uint size, Block* prev)
{
	assert(size == uint16(size));

	uint* p = uintptr(malloc(sizeof(Block) - 4 + size));
	if unlikely (!p) throw OUT_OF_MEMORY;

	Block* pm = reinterpret_cast<Block*>(p);
	pm->prev  = prev;
	pm->size  = uint16(size);
	pm->used  = 0;
	return pm;
}

struct Pool
{
	Pool*  prev;
	Block* data;

	ptr	 alloc(uint size);
	void purge();
};

void Pool::purge()
{
	while (Block* block = data)
	{
		data = block->prev;
		free(block);
	}
}

ptr Pool::alloc(uint size)
{
	if unlikely (!data) // first allocation
		data = newBlock(max(size, 100u), nullptr);

	uint used = data->used;

	if unlikely (used + size > data->size)
	{
		void* p = realloc(data, sizeof(Block) - 4 + used); // shrink to fit
		assert(p == data);								   // shrinking never moves

		uint newsize = max(size, min(uint(data->size) * 2, 3200u));
		data		 = newBlock(newsize, data);
		used		 = 0;
	}

	data->used = uint16(used + size);
	return &data->data[used];
}


static char null	 = 0;
str			emptystr = &null;
static Pool pools[2];


TempMem::TempMem(uint size)
{
	// create a new local tempmem pool
	// the TempMem object itself contains no data.
	// the local pool with all required data is 'static pools[core]'

	Pool& pool = pools[get_core_num()];
	pool.prev  = new Pool(pool);
	pool.data  = nullptr;
	if (size) pool.data = newBlock(size, nullptr);
}

TempMem::~TempMem()
{
	// dispose of the current pool for this core
	// and replace it with the current previous one:

	Pool& pool = pools[get_core_num()];
	pool.purge();
	Pool* prev = pool.prev;
	pool	   = *prev;
	delete prev;
}

void purge_tempmem() noexcept
{
	pools[get_core_num()].purge(); //
}

str newstr(uint len) noexcept
{
	// allocate char[]
	// => deallocate with delete[]
	// presets terminating 0

	str z  = new char[len + 1];
	z[len] = 0;
	return z;
}

str newcopy(cstr s) noexcept
{
	// allocate char[]
	// => deallocate with delete[]
	// returns NULL if source string is NULL

	if unlikely (!s) return nullptr;

	uint len = strlen(s) + 1;
	str	 z	 = new char[len];
	memcpy(z, s, len);
	return z;
}

char* tempstr(uint len) noexcept
{
	// Allocate a cstring
	// in the thread's current tempmem pool
	// the returned string is not aligned and may start on an odd address

	Pool& pool = pools[get_core_num()];

	char* s = pool.alloc(len + 1);
	s[len]	= 0;
	return s;
}

str dupstr(cstr s) noexcept
{
	// Create copy of string in the current tempmem pool

	if unlikely (!s) return nullptr;
	if unlikely (*s == 0) return emptystr;

	Pool& pool = pools[get_core_num()];

	uint  len = strlen(s);
	char* z	  = pool.alloc(len + 1);
	memcpy(z, s, len + 1);
	return z;
}

cstr xdupstr(cstr s) noexcept
{
	// Copy string into the surrounding tempmem pool

	Pool& pool = pools[get_core_num()];

	if (size_t(s - pool.data->data) >= pool.data->size) return s; // not in this pool!

	uint len = strlen(s);
	str	 z	 = pool.prev->alloc(len + 1);
	memcpy(z, s, len + 1);
	return z;
}

} // namespace kio

/*








































*/
