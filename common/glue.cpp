// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "glue.h"
#include "common/cdefs.h"
#include "common/malloc.h"
#include "common/trace.h"
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

// send event:
// broadcast event to all threads
void __sev() noexcept
{
	// increment count to indicate a new sev() event
	global_event_count.fetch_add(1, std::memory_order_release);

	// wake up all threads currently parked in a wfe() loop
	global_event_count.notify_all();
}

// wait for event:
void __wfe() noexcept
{
	// read the current hardware event count
	uint64 global_count = global_event_count; //.load(std::memory_order_acquire);

	// quick test whether there was (at least one) event since last test:
	if (global_count > local_event_count)
	{
		local_event_count = global_count;
		return;
	}

	// no event pending: park the thread until notified.
	// std::atomic::wait loops internally if global_event_sequence == current_global
	global_event_count.wait(global_count, std::memory_order_acquire);

	// event seen!
	// update local tracking register
	local_event_count = global_event_count; //.load(std::memory_order_relaxed);
}

// send event locally
void sevl()
{
	// Set the local event register to 0 forcing the *next* WFE to drop through
	local_event_count = 0;
}

bool wfe_or_timeout(int timeout_us)
{
	// wait for event or timeout
	// return false: event
	// return true:  timeout
	//
	// attn: in case of timeout a spurious event for all threads is generated :-/

	uint64 global_count = global_event_count; //.load(std::memory_order_acquire);
	if (global_count > local_event_count)
	{
		local_event_count = global_count;
		return false; // event!
	}

	if (timeout_us <= 0) return true; // timeout

	std::binary_semaphore sema(0);
	std::thread			  thread(
		  [&](uint64 ec, int to) {
			  local_event_count = ec;
			  __wfe();
			  sema.release();
		  },
		  local_event_count, timeout_us);
	thread.detach();

	if (sema.try_acquire_for(std::chrono::duration<int, std::micro>(timeout_us)))
	{
		// got sema => event occured, thread terminated
		local_event_count = global_event_count; //.load(std::memory_order_relaxed);
		return false;							// event
	}
	else // didn't get it => no event => thread still running
	{
		// wake it up (and everybody else currently waiting)
		__sev();
		sema.acquire(); // wait for thread to set the semaphore
		return true;	// timeout
	}
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
