// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

extern "C" __attribute__((__weak__)) const char* check_heap(); // returns nullptr or error text

#include "utilities.h"
#include "Trace.h"
#include "cdefs.h"
#include "malloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>

// defined by linker:
extern char end;
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

extern char __flash_binary_start;
extern char __flash_binary_end;

// ram from ram_vector_table to end / __HeapLimit / __StackLimit / __StackTop
extern char __data_start__; // TODO: ram_vector_table

// =========================================

namespace kio
{
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

size_t flash_start() noexcept { return XIP_BASE; } // normal address for reading: cached
size_t flash_end() noexcept { return XIP_BASE + PICO_FLASH_SIZE_BYTES; }
size_t flash_size() noexcept { return PICO_FLASH_SIZE_BYTES; }
size_t flash_used() noexcept { return flash_binary_end() - flash_start(); }
size_t flash_free() noexcept { return flash_end() - flash_binary_end(); }
size_t flash_binary_end() noexcept { return size_t(&__flash_binary_end); }
size_t flash_binary_size() noexcept { return flash_binary_end() - flash_start(); }

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

size_t stack_free() noexcept
{
	// calc remaining free size on stack of current core

	char tmp;
	return size_t(&tmp) - stack_bottom(get_core_num());
}

void print_core() { printf("### Hello core%u ###\n", get_core_num()); }

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

void print_stack_free() { printf("core%u stack free: %u bytes\n", get_core_num(), stack_free()); }

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

void print_system_info(uint)
{
	print_core();
	printf("total heap size = %u\n", heap_total_size());
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

	printf("system clock = %u MHz\n", clock_get_hz(clk_sys) / 1000000);
}


// ===============================================================


void sleep_us(int delay_usec) noexcept
{
	if (delay_usec > 0)
	{
		idle_start();
		::sleep_us(uint(delay_usec));
		idle_end();
	}
}

void wfe_or_timeout(int timeout_usec) noexcept
{
	if (timeout_usec > 0)
	{
		idle_start();
		::best_effort_wfe_or_timeout(from_us_since_boot(time_us_64() + uint(timeout_usec)));
		idle_end();
	}
}

void __attribute__((noreturn)) __printflike(1, 0) panic(const char* fmt, ...)
{
	puts("\n*** <PANIC> ***\n");

	if (fmt)
	{
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		puts("\n");
	}

	printf("core: %u\n", get_core_num());
	Trace::print(get_core_num());
	printf("stack free = %i\n", int(stack_free()));
	if (check_heap)
	{
		cstr s = check_heap();
		printf("heap: %s\n", s ? s : "valid");
		dump_heap();
	}

	for (;;);
	//_exit(1);	 --> HARDFAULT_EXCEPTION
}


} // namespace kio


/*































*/
