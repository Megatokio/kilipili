// Copyright (c) 2008 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "tempmem.h"
#include "kilipili_cdefs.h"
#include <cstring>
#include <pico/platform.h>

// this file provides rolling tempmem buffers on the RP2040 (Raspberry Pico).


#ifndef TEMPMEM_POOL_SIZE_CORE0
  #define TEMPMEM_POOL_SIZE_CORE0 2000
#endif
#ifndef TEMPMEM_POOL_SIZE_CORE1
  #define TEMPMEM_POOL_SIZE_CORE1 80
#endif


namespace kio
{

static constexpr uint size0 = TEMPMEM_POOL_SIZE_CORE0;
static constexpr uint size1 = TEMPMEM_POOL_SIZE_CORE1;

alignas(ptr) static char data0[size0];
alignas(ptr) static char data1[size1];


struct Pool
{
	Pool* prev;
	char* data;
	uint  size;
	uint  free;

	char* alloc(uint size, bool aligned = false) noexcept
	{
		if (free < size)
		{
			free = this->size;
			assert(free >= size);
		}

		free -= size;
		if (aligned) free &= ~(sizeof(ptr) - 1);
		return data + free;
	}

	void purge() noexcept { free = size; }
};


static char null	 = 0;
str			emptystr = &null;
static Pool pools[2] = {Pool {nullptr, data0, size0, size0}, Pool {nullptr, data1, size1, size1}};


TempMem::TempMem(uint size)
{
	// create a new local tempmem pool
	// the TempMem object itself contains no data.
	// the local pool with all required data is 'static pools[core]'

	if (!size) size = size0;
	constexpr uint alignment = sizeof(ptr) - 1;
	size &= ~alignment;

	Pool& pool = pools[get_core_num()];

	// create copy of current pool for use as prev:
	Pool* prev = new Pool(pool);
	char* data = new char[size];
	if (!data) panic("TempMem: oomem");

	// instantiate pools[core] as the new pool:
	pool.prev = prev;
	pool.data = data;
	pool.size = size;
	pool.free = size;
}

TempMem::~TempMem()
{
	// dispose of the current pool for this core
	// and replace it with the current previous one:

	Pool& pool = pools[get_core_num()];
	Pool* prev = pool.prev;
	if (!prev) return;

	delete[] pool.data; // destroy current pool
	pool = *prev;		// move data from previous pool
	delete prev;		// dispose of the now void previos instance
}


void purge_tempmem() noexcept { pools[get_core_num()].purge(); }

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

	if (unlikely(!s)) return nullptr;

	uint len = strlen(s) + 1;
	str	 z	 = new char[len];
	memcpy(z, s, len);
	return z;
}

char* tempmem(uint size) noexcept
{
	// Allocate some memory
	// in the thread's current tempmem pool
	// the memory is aligned to pointer size.

	Pool& pool = pools[get_core_num()];
	return pool.alloc(size, true);
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

	if (unlikely(!s)) return nullptr;
	if (unlikely(*s == 0)) return emptystr;

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

	if (size_t(s - pool.data) >= pool.size) return s;

	uint len = strlen(s);
	str	 z	 = pool.prev->alloc(len + 1);
	memcpy(z, s, len + 1);
	return z;
}

} // namespace kio

/*








































*/
