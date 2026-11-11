// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "memory.h"
#include "common/cdefs.h"
#include "common/malloc.h"
#include "common/timing.h"
#include "common/trace.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef PICO
  #include <hardware/clocks.h>
#else
  #define pico_board_cmake_set(...)
  #define pico_board_cmake_set_default(...)
  #include PICO_BOARD_H
  #include "glue.h"
#endif


// defined by linker:
extern char end;
#ifdef PICO
extern char __HeapLimit;
extern char __StackLimit;
extern char __StackTop;

extern char __scratch_x_start__;
extern char __scratch_x_end__;
extern char __scratch_y_start__;
extern char __scratch_y_end__;

extern char __StackOneTop;	  // = ORIGIN(SCRATCH_X) + LENGTH(SCRATCH_X);
extern char __StackTop;		  // = ORIGIN(SCRATCH_Y) + LENGTH(SCRATCH_Y);
extern char __StackOneBottom; // = __StackOneTop - SIZEOF(.stack1_dummy);
extern char __StackBottom;	  // = __StackTop - SIZEOF(.stack_dummy);

extern const uchar __flash_binary_start;
extern const uchar __flash_binary_end;

// ram from ram_vector_table to end / __HeapLimit / __StackLimit / __StackTop
extern char __data_start__; // TODO: ram_vector_table
#endif

// =========================================

namespace kilipili
{
#ifdef PICO
size_t heap_start() noexcept { return size_t(&end); }
size_t heap_end() noexcept { return size_t(&__StackLimit); }
size_t heap_size() noexcept { return heap_end() - heap_start(); }

size_t core0_scratch_y_start() noexcept { return size_t(&__scratch_y_start__); }
size_t core0_scratch_y_end() noexcept { return size_t(&__scratch_y_end__); }
size_t core1_scratch_x_start() noexcept { return size_t(&__scratch_x_start__); }
size_t core1_scratch_x_end() noexcept { return size_t(&__scratch_x_end__); }

size_t core0_stack_bottom() noexcept { return size_t(&__scratch_y_end__); }
size_t core1_stack_bottom() noexcept { return size_t(&__scratch_x_end__); }
size_t stack_bottom(uint core) noexcept { return core == 0 ? core0_stack_bottom() : core1_stack_bottom(); }

size_t core0_stack_top() noexcept { return size_t(&__StackTop); }
size_t core1_stack_top() noexcept { return size_t(&__StackOneTop); }
size_t stack_top(uint core) noexcept { return core == 0 ? core0_stack_top() : core1_stack_top(); }

cuptr flash_start() noexcept { return cuptr(XIP_BASE); } // normal address for reading: cached
uint  flash_size() noexcept { return PICO_FLASH_SIZE_BYTES; }
cuptr flash_binary_end() noexcept { return &__flash_binary_end; }
#else
size_t heap_start() noexcept { return size_t(&end); }
size_t heap_end() noexcept { return size_t(&end) + 0; }
size_t heap_size() noexcept { return heap_end() - heap_start(); }

size_t core0_scratch_y_start() noexcept { return 0; }
size_t core0_scratch_y_end() noexcept { return 0; }
size_t core1_scratch_x_start() noexcept { return 0; }
size_t core1_scratch_x_end() noexcept { return 0; }

size_t core0_stack_bottom() noexcept { return 0; }
size_t core1_stack_bottom() noexcept { return 0; }
size_t stack_bottom(uint core) noexcept { return core == 0 ? core0_stack_bottom() : core1_stack_bottom(); }

size_t core0_stack_top() noexcept { return 0; }
size_t core1_stack_top() noexcept { return 0; }
size_t stack_top(uint core) noexcept { return core == 0 ? core0_stack_top() : core1_stack_top(); }

cuptr flash_start() noexcept { return flash_image_start; }
uint  flash_size() noexcept { return flash_image_size; }
cuptr flash_binary_end() noexcept { return flash_start(); }
#endif

cuptr flash_end() noexcept { return flash_start() + flash_size(); }
uint  flash_used() noexcept { return uint(flash_binary_end() - flash_start()); }
uint  flash_free() noexcept { return uint(flash_end() - flash_binary_end()); }
uint  flash_binary_size() noexcept { return uint(flash_binary_end() - flash_start()); }

#ifdef PICO
size_t heap_free() noexcept
{
	// calculate the size of the biggest free chunk on the heap:

	size_t lo = 0, hi = 256 kB;
	while (hi - lo >= 4)
	{
		size_t m		 = (hi + lo) / 2;
		void* volatile p = malloc(m);
		if (p) lo = m;
		else hi = m;
		free(p);
	}
	return lo;
}
#else
size_t heap_free() noexcept { return 0; }
#endif

int stack_free() noexcept
{
	// calc remaining free size on stack of current core

	char	tmp;
	ssize_t free = ssize_t(&tmp) - ssize_t(stack_bottom(get_core_num()));
	if (free == int(free)) return int(free);
	return free < 0 ? -0x7fffffff : +0x7fffffff;
}

#ifdef PICO
void assert_stack_free(cstr func, int sz) noexcept
{
	int free = stack_free();
	if (free >= sz) return;
	else if (free >= 0) panic("%s: stack overflow: only %i bytes free (%i required)", func, free, sz);
	else panic("%s: stack overflow by %i bytes", func, -free);
}
#else
void assert_stack_free(cstr func, int sz) noexcept {}
#endif

void print_core() { printf("### Hello core%u ###\n", get_core_num()); }

#ifdef PICO
void print_heap_free(int r)
{
	// print list of all free chunks on the heap:

	uint32 sz = heap_largest_free_block();
	if (sz == 0) return;

	printf("%s: %u bytes\n", r ? "+fragment" : "heap free", sz);

	void* volatile p = malloc(sz);

	if constexpr (0)
	{
		if (!p) // stdclib malloc sometimes fails to return available memory:
		{
			printf("  alloc %u bytes failed\n", sz);
			sz = heap_largest_free_block();
			printf("  now available: %u bytes (recalculated)\n", sz);
			while (!p && sz) p = malloc(sz--);
			printf("  now available: %u bytes (trying hard)\n", ++sz);
		}
	}

	assert(p);
	print_heap_free(1);
	free(p);
}
#else
void print_heap_free(int) { printf("heap free: 0 bytes\n"); }
#endif

void print_stack_free() { printf("core%u stack free: %i bytes\n", get_core_num(), int(stack_free())); }

#ifdef PICO
void print_core0_scratch_y_usage()
{
	const uintptr_t xa = uintptr_t(&__scratch_y_start__);
	const uintptr_t xe = uintptr_t(&__scratch_y_end__);

	if (xa != xe) printf("0x%08x to 0x%08x: core0 scratch_y\n", xa, xe);
	else printf("core0 scratch_y not used\n");
	printf("0x%08x to 0x%08x: core0 stack\n", uint(core0_stack_bottom()), uint(core0_stack_top()));
}

void print_core1_scratch_x_usage()
{
	const uintptr_t xa = size_t(&__scratch_x_start__);
	const uintptr_t xe = size_t(&__scratch_x_end__);

	if (xa != xe) printf("0x%08x to 0x%08x: core1 scratch_x\n", xa, xe);
	else printf("core1 scratch_x not used\n");
	printf("0x%08x to 0x%08x: core1 stack\n", uint(core1_stack_bottom()), uint(core1_stack_top()));
}

void print_flash_usage()
{
	const size_t fa = uintptr_t(&__flash_binary_start);
	const size_t fe = uintptr_t(&__flash_binary_end);

	printf("0x%08x to 0x%08x: flash, used %u, free %u\n", fa, fe, flash_used(), flash_free());
}
#else
void print_core0_scratch_y_usage() { printf("scratch y usage: 0 bytes\n"); }
void print_core1_scratch_x_usage() { printf("scratch x usage: 0 bytes\n"); }
void print_flash_usage() { printf("flash usage: 0 bytes\n"); }
#endif

void print_system_info(uint)
{
	print_core();
	printf("total heap size = %u\n", uint(heap_total_size()));
	print_heap_free();
	print_stack_free();
	print_core0_scratch_y_usage();
	print_core1_scratch_x_usage();
	print_flash_usage();

	// ram    @ 0x20000000 - 0x20040000
	// stack1 @ 0x20040000 - 0x20041000
	// stack0 @ 0x20041000 - 0x20042000
	// flash  @ 0x10000000 - 0x10200000 (address of cached access, 2MB)

	//printf("core0 stack base @ 0x%08x\n", get_stack_base(0));
	//printf("core1 stack base @ 0x%08x\n", get_stack_base(1));

	printf("system clock = %u MHz\n", get_system_clock() / 1000000);
}


// ===============================================================


} // namespace kilipili


/*































*/
