// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "glue.h"
#ifndef LIB_PICO_STDLIB
  #include "common/cdefs.h"
  #include "pico_hw/Interp.h"
  #include "usb_host/hid_handler.h"
  #include <ctime>
  #include <unistd.h>
#endif


#ifdef LIB_PICO_STDLIB
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
