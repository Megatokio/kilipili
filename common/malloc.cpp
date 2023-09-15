// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "cdefs.h"
#include <pico/malloc.h>
#include <pico/mutex.h>

// defined by linker:
extern uint32 end;
extern uint32 __HeapLimit;
extern uint32 __StackLimit;
extern uint32 __StackTop;


/*	The heap occupies the space between `end` and `__stack_limit`, which are calculated by the linker.

	The whole heap is occupied by a list of chunks which can be either used or free.
	Each chunk is preceeded by a uint32 size, which is the size in uint32 words incl. this size itself.
	Therefore the next chunk after uint32* p can be reached by p += *p.

	The variable `first_free` points to the first free chunk or a used chunk before that.
	Malloc starts search for a free gap at that address. Malloc tries to update this variable
	where possible to eliminate skipping over the first used chunks in every call.

	The upper bits of the `size` word are used to indicate used or free and to provide minimal validation.

	note: The pico_sdk uses wrapper around malloc library calls.
		  Therefore no multicore precautions are needed here.
*/


constexpr uint32* heap_start = &end;
constexpr uint32* heap_end = &__StackLimit;

static uint32* first_free = nullptr;			// (before) first free chunk


// Values for Raspberry Pico RP2040:
constexpr uint32 size_mask = 0x0000ffff;		// uint32 words
constexpr uint32 flag_mask = 0xffff0000;		// the other bits
constexpr uint32 flag_used = 0xA53C0000;		// magic number, msb set
constexpr uint32 flag_free = 0x00000000;		// all bits cleared

constexpr size_t max_size  = (size_mask<<2)-4;	// bytes


// helper:
static inline bool is_used(uint32* p) { return int32(*p) < 0; }
static inline bool is_free(uint32* p) { return int32(*p) >= 0; }

[[maybe_unused]] static inline bool is_valid_used(uint32* p) { return (*p & flag_mask) == flag_used; }
[[maybe_unused]] static inline bool is_valid_free(uint32* p) { return (*p & flag_mask) == flag_free; }

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
	if (unlikely(first_free == nullptr))
	{
		uint32 free_size = uint32(heap_end - heap_start);
		assert (free_size <= size_mask);

		first_free = heap_start;
		*first_free = free_size | flag_free;	// define one big free chunk
	}

	// calc. required size in words, incl. header:
	if (unlikely(size > max_size)) return nullptr;
	size = (size + 7) >> 2;

	uint32* p = first_free = skip_used(first_free);	// find 1st free chunk

	while (p < heap_end)
	{
		size_t gap = uint32(skip_free(p) - p);

		if (gap >= size)
		{
			if (gap > size) { *(p+size) = (gap - size) | flag_free; }  // split
			*p = size | flag_used;
			return p+1;
		}

		// gap too small:
		*p = gap | flag_free;
		p = skip_used(p + gap);  // find next free chunk
	}

	return nullptr;
}

void* calloc(size_t count, size_t size)
{
	// The calloc() function allocates memory for an array of nmemb elements of size bytes each
	// and returns a pointer to the allocated memory. The memory is set to zero.
	// If nmemb or size is 0, then calloc() returns either NULL, or a unique pointer value
	// that can later be successfully passed to free().

	if (unlikely(__builtin_clz(count|1) + __builtin_clz(size|1) < 38))
		return nullptr; // size has 25 or 26 bits => more than 24 bits => size > 0x00ff.ffff

	size *= count;
	void* p = malloc(size);
	if (p) memset(p, 0, size);
	return p;
}

void* realloc(void *mem, size_t size)
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
	if (size == 0) { free(mem); return nullptr; }

	size = (size+7) >> 2;
	uint32* p = reinterpret_cast<uint32*>(mem) - 1;
	assert(is_valid_used(p));
	uint32  old_size = *p & size_mask;

	if (size < old_size) // shrinked
	{
		*p = size | flag_used;
		p += size;
		*p = (old_size - size) | flag_free;
		if (p < first_free) first_free = p;
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
			if (p < first_free) first_free = p;
			return mem;

		}

		// can't grow in place, must relocate:
		void* z = malloc((size-1) << 2);
		if (z) { memcpy(z,mem,(old_size-1)<<2); free(mem); }
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
		if (p < first_free) first_free = p;
	}
}






















