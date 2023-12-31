// Copyright (c) 2008 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "basic_math.h"
#include "cdefs.h"
#include "string.h"
#include "tempmem.h"
#include <cstddef>

// this file provides tempmem buffers on systems supporting c++11 thread_local variables.

namespace kio
{

struct Block
{
	Block* prev;
	uint   size;
	uint   used;
	char   data[8];
};

Block* newBlock(uint size, Block* prev)
{
	uint* p = uintptr(malloc(sizeof(Block) - sizeof(Block::data) + size));
	if unlikely (!p) throw OUT_OF_MEMORY;

	Block* pm = reinterpret_cast<Block*>(p);
	pm->prev  = prev;
	pm->size  = uint16(size);
	pm->used  = 0;
	return pm;
}

struct Pool
{
	Pool*  prev = nullptr;
	Block* data = nullptr;

	ptr	 alloc(uint size);
	void purge() noexcept;
};

void Pool::purge() noexcept
{
	while (Block* block = data)
	{
		data = block->prev;
		free(block);
	}
}

ptr Pool::alloc(uint size)
{
	static constexpr uint min_block_size = 8 kB - sizeof(Block);

	if unlikely (!data) // first allocation
		data = newBlock(max(size, min_block_size), nullptr);

	uint used = data->used;

	if unlikely (used + size > data->size)
	{
		data = newBlock(max(size, min_block_size), data);
		used = 0;
	}

	data->used = uint16(used + size);
	return &data->data[used];
}


static char				 null	  = 0;
str						 emptystr = &null;
static thread_local Pool pool;


TempMem::TempMem(uint size)
{
	// create a new local tempmem pool
	// the TempMem object itself contains no data.
	// the local pool with all required data is 'static pools[core]'

	pool.prev = new Pool(pool);
	pool.data = nullptr;
	if (size) pool.data = newBlock(size, nullptr);
}

TempMem::~TempMem() noexcept
{
	// dispose of the current pool for this core
	// and replace it with the current previous one:

	pool.purge();
	Pool* prev = pool.prev;
	pool	   = *prev;
	delete prev;
}

void purge_tempmem() noexcept
{
	pool.purge(); //
}

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

char* tempstr(uint len)
{
	// Allocate a cstring
	// in the thread's current tempmem pool
	// the returned string is not aligned and may start on an odd address

	char* s = pool.alloc(len + 1);
	s[len]	= 0;
	return s;
}

ptr tempmem(uint size)
{
	if (Block* data = pool.data)
		if (uint odd = data->used & (sizeof(max_align_t) - 1)) //
			data->used += sizeof(max_align_t) - odd;

	return pool.alloc(size);
}

str dupstr(cstr s)
{
	// Create copy of string in the current tempmem pool

	if unlikely (!s) return nullptr;
	if unlikely (*s == 0) return emptystr;

	uint  len = uint(strlen(s));
	char* z	  = pool.alloc(len + 1);
	memcpy(z, s, len + 1);
	return z;
}

cstr xdupstr(cstr s)
{
	// Copy string into the surrounding tempmem pool

	assert(pool.prev);

	if unlikely (!s) return nullptr;
	if unlikely (*s == 0) return emptystr;

	uint len = uint(strlen(s));
	str	 z	 = pool.prev->alloc(len + 1);
	memcpy(z, s, len + 1);
	return z;
}

str xtempstr(uint len)
{
	assert(pool.prev);

	char* s = pool.prev->alloc(len + 1);
	s[len]	= 0;
	return s;
}

ptr xtempmem(uint size)
{
	assert(pool.prev);

	if (Block* data = pool.prev->data)
		if (uint odd = data->used & (sizeof(max_align_t) - 1)) //
			data->used += sizeof(max_align_t) - odd;

	return pool.prev->alloc(size);
}

} // namespace kio


/*






































*/
