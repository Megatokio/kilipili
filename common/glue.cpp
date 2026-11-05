// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "glue.h"
#ifdef PICO
  #include "common/malloc.h"
  #include "common/trace.h"
  #include <stdio.h>
#else
  #include "common/cdefs.h"
  #include "pico_hw/Interp.h"
  #include "usb_host/hid_handler.h"
  #include <ctime>
  #include <unistd.h>
#endif


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

extern char __scratch_x_end__; // set by linker
extern char __scratch_y_end__; // set by linker

namespace kilipili
{
void __attribute__((noreturn)) __printflike(1, 0) panic(const char* fmt, ...)
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

	char tmp;
	ptr	 stack_bottom = get_core_num() ? &__scratch_x_end__ : &__scratch_y_end__;

	Trace::print(get_core_num());
	printf("stack free = %i\n", int(&tmp) - int(stack_bottom));
	cstr s = check_heap();
	printf("heap: %s\n", s ? s : "valid");
	dump_heap();
	for (;;);
	//_exit(1);	 --> HARDFAULT_EXCEPTION
}
} // namespace kilipili


#else // ====================================================

SpinLock kilipili_spinlock;

thread_local int core_num;
Interp			 interp0_hw_array[2];
Interp			 interp1_hw_array[2];

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


#endif
