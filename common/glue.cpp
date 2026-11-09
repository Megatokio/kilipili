// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "glue.h"
#include "common/cdefs.h"
#include "common/malloc.h"
#include "common/trace.h"
#include "timing.h"
#ifdef PICO
  #include <stdio.h>
#else
  #include "pico_hw/Interp.h"
  #include "usb_host/hid_handler.h"
  #include <atomic>
  #include <cstdarg>
  #include <cstdio>
  #include <ctime>
  #include <semaphore>
  #include <thread>
  #include <unistd.h>
  #include <vector>
#endif


namespace kilipili
{
volatile int  hot_row = 0;						  // Sprite
SpinLock	  sprites_spinlock(SpinLock::UNUSED); // Sprite
bool		  hotlist_overflow = false;			  // MultiSpritesPlane
uint		  scanlines_missed = 0;				  // VideoController
volatile bool locked_out	   = false;			  // VideoController
} // namespace kilipili


// ========================================================
//					wfe(), sev()
// ========================================================

#ifndef PICO
// sev() increments a global event counter.
// threads maintain the count they have seen in a thread-local tracking register.
// when the global count is higher than the local count then a event was triggered since.
static std::atomic<uint64> global_event_count = 0;
static thread_local uint64 local_event_count  = 0;
using Sema									  = std::binary_semaphore;
static Sema semaphores[2] {{Sema(0)}, {Sema(0)}};


void __sev() noexcept
{
	// send event:
	// broadcast event to all threads

	// increment count to indicate a new sev() event
	global_event_count.fetch_add(1, std::memory_order_release);

	if ((0))
	{
		// note: if both cores sev() at the very same time this still may overflow the semaphore counter
		// this is highly unlikely but we could build a spinlock (or mutex?) around it...
		semaphores[0].try_acquire(); // decrement to 0 if it was 1
		semaphores[0].release();	 // increment to 1
		semaphores[1].try_acquire();
		semaphores[1].release();
	}
	else
	{
		// the own semaphore needs no release:
		// this core is obviously running and not waiting in wfe() or wfe_with_timeout().
		// next time it calls wfe() or wfe_with_timeout() it will first check the event count
		// before trying to acquire the semaphore => no need to release the semaphore here!
		// simultaneous sev() on core0 and core1 can't collide!
		// this even reduces the cpu load for multiple try_acquire() / release() below:

		// note: if the gui thread calls __sev() at the very same time as one of the cores
		// then this still may overflow the semaphore counter
		uint n = core_num;
		if (n != 0)
		{
			semaphores[0].try_acquire(); // decrement to 0 if it was 1
			semaphores[0].release();	 // increment to 1
		}
		if (n != 1)
		{
			semaphores[1].try_acquire();
			semaphores[1].release();
		}
	}
}

void __wfe() noexcept
{
	// wait for event

	// gui thread has core_num = 2: semaphore may be added if needed.
	assert_lt(core_num, NELEM(semaphores));

	for (;;)
	{
		// read the current hardware event count
		uint64 global_count = global_event_count.load(std::memory_order_acquire);

		// was there any event since last test?
		if (global_count > local_event_count)
		{
			local_event_count = global_count;
			return;
		}

		semaphores[core_num].acquire();
	}
}

bool wfe_or_timeout(int timeout_us)
{
	// wait for event or timeout
	//
	// return false: event
	// return true:  timeout

	// gui thread has core_num = 2: semaphore may be added if needed.
	assert_lt(core_num, NELEM(semaphores));

	using namespace kilipili;

	// was there any event since last test?
	uint64 global_count = global_event_count.load(std::memory_order_acquire);
	if (global_count > local_event_count)
	{
		local_event_count = global_count;
		return false; // event
	}

	CC final_time_us = now() + timeout_us;

	while (timeout_us > 0)
	{
		std::chrono::duration<int, std::micro> delay {timeout_us};
		semaphores[core_num].try_acquire_for(delay);

		// was there an event since last test?
		uint64 global_count = global_event_count.load(std::memory_order_acquire);
		if (global_count > local_event_count)
		{
			local_event_count = global_count;
			return false; // event
		}

		timeout_us = final_time_us - now();
	}

	return true; // timeout
}

void sevl()
{
	// send event locally:
	// force an event on the own core only:
	local_event_count = 0;
}
#endif


// ========================================================
//					SpinLock
// ========================================================

#ifdef PICO
  #if defined KILIPILI_SPINLOCK_NUMBER

static struct OnInit
{
	OnInit()
	{
		spin_lock_claim(KILIPILI_SPINLOCK_NUMBER);
		spin_lock_init(KILIPILI_SPINLOCK_NUMBER);
	}
} init;

  #endif
#else
SpinLock kilipili_spinlock;
#endif


// ========================================================
//					panic(fmt,…)
// ========================================================

extern char __scratch_x_end__; // set by linker
extern char __scratch_y_end__; // set by linker

namespace kilipili
{
void __noreturn __printflike(1, 2) panic(const char* fmt, ...)
{
	printf("\n*** <PANIC> ***\ncore: %u\n", get_core_num());

	if (fmt)
	{
		va_list args;
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
		putchar('\n');
	}

#ifdef PICO
	char tmp;
	ptr	 stack_bottom = get_core_num() ? &__scratch_x_end__ : &__scratch_y_end__;

	print_trace();
	printf("stack free = %i\n", int(&tmp) - int(stack_bottom));
	cstr s = check_heap();
	printf("heap: %s\n", s ? s : "valid");
	dump_heap();
	for (;;); //_exit(1); --> HARDFAULT_EXCEPTION
#else
	print_trace();
	abort();
#endif
}
} // namespace kilipili


// ========================================================
//					malloc.c stubs
// ========================================================

__weak_symbol bool	 mresize(void* mem, size_t size) { return false; }
__weak_symbol cstr	 check_heap() { return NO_ERROR; }
__weak_symbol void	 dump_heap() {}
__weak_symbol void	 dump_heap_to_fu(dump_heap_print_fu*, void* data) {}
__weak_symbol size_t heap_total_size() { return 99000000; }
__weak_symbol size_t heap_largest_free_block() { return 99000000; }
__weak_symbol bool	 heap_cut_exception_block() { return false; }


// ========================================================
//					other stubs
// ========================================================

#ifndef PICO

thread_local uint		 core_num;
thread_local interp_hw_t interp_hw_array[2];

// overridden by emulator:
void __weak_symbol stdio_init_all() {}
int __weak_symbol  stdio_getchar_timeout_us(uint32) { return -1; }


namespace kilipili::USB
{
void initUSBHost() noexcept {}
int	 pollUSB(void*) noexcept { return 0; }
bool keyboardPresent() noexcept { return false; }
bool mousePresent() noexcept { return false; }
// for unknown reason the linker insists on these symbols:
void setHidMouseEventHandler(HidMouseEventHandler* handler) noexcept { IERR(); }
void setHidKeyboardEventHandler(HidKeyboardEventHandler* handler) noexcept { IERR(); }
} // namespace kilipili::USB

// these are also in unit_test/Mock/MockFlash.cpp:
void __weak_symbol flash_range_erase(uint32 flash_offs, uint32 count) { TODO(); }
void __weak_symbol flash_range_program(uint32 flash_offs, const uint8* data, uint32 count) { TODO(); }


#endif
