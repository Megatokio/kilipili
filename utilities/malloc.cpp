// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "malloc.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <pico.h>
#include <pico/malloc.h>

#ifndef PICO_CXX_ENABLE_EXCEPTIONS
  #define PICO_CXX_ENABLE_EXCEPTIONS false
#endif

static constexpr bool enable_exceptions = PICO_CXX_ENABLE_EXCEPTIONS;
constexpr char		  OUT_OF_MEMORY[]	= "out of memory";

void* operator new(size_t n)
{
	if (void* p = malloc(n)) return p;
	if constexpr (enable_exceptions) throw OUT_OF_MEMORY;
	else panic(OUT_OF_MEMORY);
}
void* operator new[](size_t n)
{
	if (void* p = malloc(n)) return p;
	if constexpr (enable_exceptions) throw OUT_OF_MEMORY;
	else panic(OUT_OF_MEMORY);
}

void* operator new(size_t n, const std::nothrow_t&) noexcept { return malloc(n); }

void* operator new[](size_t n, const std::nothrow_t&) noexcept { return malloc(n); }

void operator delete(void* p) noexcept { free(p); }
void operator delete[](void* p) noexcept { free(p); }

#if defined __cpp_sized_deallocation && __cpp_sized_deallocation
void		operator delete(void* p, __unused size_t n) noexcept { free(p); }
void		operator delete[](void* p, __unused size_t n) noexcept { free(p); }
#endif


/*
	ATTN: This file must be linked into the exectutable directly, not in a library,
	else parts of the application may use this version and others use the stdlib version of malloc.
*/


#define unlikely(x) (__builtin_expect(!!(x), 0))
using uint32 = uint32_t;
using int32	 = int32_t;
using cptr	 = const char*;
using Error	 = const char*;

// defined by linker:
extern uint32 end;
extern uint32 __HeapLimit;
extern uint32 __StackLimit;
extern uint32 __StackTop;


/*	The heap occupies the space between `end` and `__StackLimit`, which are calculated by the linker.

	The whole heap is occupied by a list of chunks which can be either used or free.
	Each chunk is preceeded by a uint32 size, which is the size in uint32 words incl. this size itself.
	Therefore the next chunk after uint32* p can be reached by p += *p.

	The upper bits of the `size` word are used to indicate used or free and to provide minimal validation.

	note: The pico_sdk uses wrapper around malloc library calls.
		  Therefore no multicore precautions are needed here.
*/


constexpr uint32* heap_start = &end;
constexpr uint32* heap_end	 = &__StackLimit;

constexpr uint32 sram_size = SRAM_STRIPED_END - SRAM_STRIPED_BASE;
static_assert((sram_size & (sram_size - 1)) == 0);

constexpr uint32 size_mask = sram_size / 4 - 1;		 // 0x0000ffff  - uint32 words
constexpr uint32 flag_mask = ~size_mask;			 // 0xffff0000  - the other bits
constexpr uint32 flag_used = 0xA53C0000 & flag_mask; // magic number, msb set
constexpr uint32 flag_free = 0x00000000;			 // all bits cleared

constexpr size_t max_size = (size_mask << 2) - 4; // bytes


// helper:
static inline bool is_used(uint32* p) { return int32(*p) < 0; }
static inline bool is_free(uint32* p) { return int32(*p) >= 0; }

[[maybe_unused]] static inline bool is_valid_used(uint32* p)
{
	return (*p & flag_mask) == flag_used && *p != flag_used;
}
[[maybe_unused]] static inline bool is_valid_free(uint32* p)
{
	return (*p & flag_mask) == flag_free && *p != flag_free;
}

static inline uint32* skip_free(uint32* p)
{
	while (p < heap_end && is_free(p)) { p += *p; }
	return p;
}

static inline uint32* skip_used(uint32* p)
{
	while (p < heap_end && is_used(p)) { p += *p & size_mask; }
	return p;
}


void* malloc(size_t size)
{
	// The malloc() function allocates size bytes and returns a pointer to the allocated memory.
	// The memory is not initialized. If size is 0, then malloc() returns either NULL,
	// or a unique pointer value that can later be successfully passed to free().

	// initialize in first call:
	static char initialized = 0;
	if unlikely (!initialized)
	{
		initialized		 = 1;
		uint32 free_size = uint32(heap_end - heap_start);
		assert(free_size <= size_mask);

		*heap_start = free_size | flag_free; // define one big free chunk
	}

	// calc. required size in words, incl. header:
	if unlikely (size > max_size) return nullptr;
	size = (size + 7) >> 2;

	uint32* p = skip_used(heap_start); // find 1st free chunk

	while (p < heap_end)
	{
		size_t gap = uint32(skip_free(p) - p);

		if (gap >= size)
		{
			if (gap > size) { *(p + size) = (gap - size) | flag_free; } // split
			*p = size | flag_used;
			return p + 1;
		}

		// gap too small:
		*p = gap | flag_free;
		p  = skip_used(p + gap); // find next free chunk
	}

	return nullptr;
}

void* calloc(size_t count, size_t size)
{
	// The calloc() function allocates memory for an array of nmemb elements of size bytes each
	// and returns a pointer to the allocated memory. The memory is set to zero.
	// If nmemb or size is 0, then calloc() returns either NULL, or a unique pointer value
	// that can later be successfully passed to free().

	if unlikely (__builtin_clz(count | 1) + __builtin_clz(size | 1) < 38)
		return nullptr; // size has 25 or 26 bits => more than 24 bits => size > 0x00ff.ffff

	size *= count;
	void* p = malloc(size);
	if (p) memset(p, 0, size);
	return p;
}

void* realloc(void* mem, size_t size)
{
	// The realloc() function changes the size of the memory block pointed to by ptr to size bytes.
	// The contents will be unchanged in the range from the start of the region up to the minimum
	// of the old and new sizes. If the new size is larger than the old size, the added memory
	// will not be initialized. If ptr is NULL, then the call is equivalent to malloc(size),
	// for all values of size; if size is equal to zero, and ptr is not NULL, then the call is
	// equivalent to free(ptr). Unless ptr is NULL, it must have been returned by an earlier call
	// to malloc(), calloc() or realloc(). If the area pointed to was moved, a free(ptr) is done.
	//
	// The realloc() function returns a pointer to the newly allocated memory, which is suitably
	// aligned for any kind of variable and may be different from ptr, or NULL if the request fails.
	// If size was equal to 0, either NULL or a pointer suitable to be passed to free() is returned.
	// If realloc() fails the original block is left untouched; it is not freed or moved.

	if (mem == nullptr) return malloc(size);
	if (size == 0)
	{
		free(mem);
		return nullptr;
	}

	size	  = (size + 7) >> 2;
	uint32* p = reinterpret_cast<uint32*>(mem) - 1;
	assert(is_valid_used(p));
	uint32 old_size = *p & size_mask;

	if (size < old_size) // shrinked
	{
		*p = size | flag_used;
		p += size;
		*p = (old_size - size) | flag_free;
		return mem;
	}

	else if (size > old_size) // growed
	{
		size_t avail = uint32(skip_free(p + old_size) - p);
		if (avail >= size)
		{
			*p = size | flag_used;
			p += size;
			if (avail > size) *p = (avail - size) | flag_free;
			return mem;
		}

		// can't grow in place, must relocate:
		void* z = malloc((size - 1) << 2);
		if (z)
		{
			memcpy(z, mem, (old_size - 1) << 2);
			free(mem);
		}
		return z;
	}

	else // new_size == old_size
	{
		return mem;
	}
}

void free(void* mem)
{
	// The free() function frees the memory space pointed to by ptr, which must have been
	// returned by a previous call to malloc(), calloc() or realloc().
	// Otherwise, or if free(ptr) has already been called before, undefined behavior occurs.
	// If ptr is NULL, no operation is performed.

	if (mem)
	{
		uint32* p = reinterpret_cast<uint32*>(mem) - 1;
		assert(is_valid_used(p));
		*p = (*p & size_mask) | flag_free;
	}
}

Error check_heap()
{
	uint32* p = heap_start;

	while (p < heap_end)
	{
		if (is_valid_used(p)) p += *p & size_mask;
		else if (is_valid_free(p)) p += *p;
		else return "heap: invalid block found";
	}
	if (p > heap_end) return "heap: last block extends beyond heap end";
	return nullptr;
}

static inline int min(int a, int b) { return a <= b ? a : b; }
static inline int max(int a, int b) { return a >= b ? a : b; }

static void dump_memory(cptr p, int sz)
{
	for (int i = 0; i < sz; i += 32)
	{
		printf("  ");
		int n = min(32, sz - i);
		for (int j = i; j < i + n; j++) printf("%02x ", p[j]);
		for (int j = i + n; j < i + 32; j++) printf("   ");
		for (int j = i; j < i + n; j++) printf("%c", p[j] >= 32 && p[j] < 127 ? p[j] : '_');
		printf("\n");
	}
}

void dump_heap()
{
	uint32* p = heap_start;

	while (p < heap_end)
	{
		if (is_valid_free(p))
		{
			int sz = skip_free(p) - p;
			printf("0x%08x: free, sz=%i\n", uint32(p + 1), sz * 4 - 4);
			p += sz;
		}
		else if (is_valid_used(p))
		{
			int sz = *p & size_mask;
			printf("0x%08x: used, sz=%i\n", uint32(p + 1), sz * 4 - 4);
			dump_memory(cptr(p) + 4, min(256, sz * 4 - 4));
			p += sz;
		}
		else
		{
			printf("0x%08x: invalid data\n", uint32(p));
			printf("note: dump starts with the void heap link\n");
			dump_memory(cptr(p), 256);
			break;
		}
	}
	if (p > heap_end) printf("error: last block extends beyond heap end\n");
}


void dump_heap_to_fu(dump_heap_print_fu* print_fu, void* data)
{
	for (uint32* p = heap_start; p < heap_end;)
	{
		if (is_valid_free(p))
		{
			int sz = skip_free(p) - p;
			print_fu(data, p + 1, sz * 4 - 4, 0);
			p += sz;
		}
		else if (is_valid_used(p))
		{
			int sz = *p & size_mask;
			print_fu(data, p + 1, sz * 4 - 4, 1);
			p += sz;
		}
		else
		{
			print_fu(data, p, 256, 2);
			break;
		}
	}
}

size_t heap_total_size()
{
	return size_t(heap_end) - size_t(heap_start); //
}

size_t heap_largest_free_block()
{
	int maxfree = 0 + 1;

	for (uint32* p = heap_start; p < heap_end;)
	{
		if (is_valid_used(p)) { p += *p & size_mask; }
		else if (is_valid_free(p))
		{
			int sz	= skip_free(p) - p;
			maxfree = max(maxfree, sz);
			p += sz;
		}
		else { return 0; }
	}

	return uint(maxfree - 1) * 4;
}


/*































*/
