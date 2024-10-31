// Copyright (c) 2008 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "tempmem.h"
#include "cdefs.h"
#include <cstring>
#include <pico/sync.h>

#ifndef TEMPMEM_SIZE0
  #define TEMPMEM_SIZE0 1000
#endif
#ifndef TEMPMEM_SIZE1
  #define TEMPMEM_SIZE1 320
#endif


// this file provides cyclic tempmem buffers on the RP2040 (Raspberry Pico).


namespace kio
{

static char null	 = 0;
str			emptystr = &null;


template<uint SZ>
struct TPool
{
	TPool* prev = nullptr;
	uint16 size;
	uint16 avail;
	char   data[SZ] = {};

	char* alloc(uint cnt) noexcept // unaligned, uncleared
	{
		assert_le(cnt, size);
		avail = uint16(avail >= cnt ? avail - cnt : size - cnt);
		return data + avail;
	}

	void  purge() noexcept { avail = size; }
	char* tempstr(uint len)
	{
		str s  = alloc(len + 1);
		s[len] = 0;
		return s;
	}

	ptr tempmem(uint size)
	{
		ptr	 p = alloc(size);
		uint d = size_t(p) & 3;
		assert_ge(avail, d);
		avail -= d;
		return p - d;
	}

	str dupstr(cstr s)
	{
		if unlikely (!s) return nullptr;
		if unlikely (*s == 0) return emptystr;

		uint n = strlen(s) + 1;
		str	 z = alloc(n);
		memcpy(z, s, n);
		return z;
	}
};

static TPool<TEMPMEM_SIZE0> pool0 {.size = TEMPMEM_SIZE0, .avail = TEMPMEM_SIZE0};
static TPool<TEMPMEM_SIZE1> pool1 {.size = TEMPMEM_SIZE1, .avail = TEMPMEM_SIZE1};

using Pool = TPool<4>;

static Pool* pools[2] = {reinterpret_cast<Pool*>(&pool0), reinterpret_cast<Pool*>(&pool1)};


Pool* newPool(uint size, Pool* prev)
{
	assert(size == uint16(size));

	void* p = malloc(sizeof(Pool) - sizeof(Pool::data) + size);
	if unlikely (!p) throw OUT_OF_MEMORY;

	Pool* pp  = reinterpret_cast<Pool*>(p);
	pp->prev  = prev;
	pp->size  = uint16(size);
	pp->avail = uint16(size);
	return pp;
}

TempMem::TempMem(uint size)
{
	// create a new local tempmem pool
	// the TempMem object itself contains no data.
	// the local pool with all required data is 'static pools[core]'

	uint core	= get_core_num();
	pools[core] = newPool(size, pools[core]);
}

TempMem::~TempMem() noexcept
{
	// dispose of the current pool for this core
	// and replace it with the current previous one:

	uint  core	= get_core_num();
	Pool* pool	= pools[core];
	pools[core] = pool->prev;
	free(pool);
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

	uint len = strlen(s) + 1;
	str	 z	 = new char[len];
	memcpy(z, s, len);
	return z;
}

void purge_tempmem() noexcept { pools[get_core_num()]->purge(); }

char* tempstr(uint len) { return pools[get_core_num()]->tempstr(len); }

ptr tempmem(uint size) { return pools[get_core_num()]->tempmem(size); }

str dupstr(cstr s) { return pools[get_core_num()]->dupstr(s); }

cstr xdupstr(cstr s)
{
	Pool* pool = pools[get_core_num()];
	assert(pool->prev);
	return pool->prev->dupstr(s);
}

str xtempstr(uint len)
{
	Pool* pool = pools[get_core_num()];
	assert(pool->prev);
	return pool->prev->tempstr(len);
}

ptr xtempmem(uint size)
{
	Pool* pool = pools[get_core_num()];
	assert(pool->prev);
	return pool->prev->tempmem(size);
}

} // namespace kio

/*














































*/
