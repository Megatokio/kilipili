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


#ifndef LIB_PICO_STDLIB
thread_local int core_num;
SpinLock		 kilipili_spinlock;
Interp			 interp0_hw_array[2];
Interp			 interp1_hw_array[2];

int getchar_timeout_us(uint32)
{
	TODO();
	return -1; // no char
}

namespace kilipili::USB
{
// for unknown reason the linker insists on these symbols:
void setHidMouseEventHandler(HidMouseEventHandler* handler) noexcept { IERR(); }
void setHidKeyboardEventHandler(HidKeyboardEventHandler* handler) noexcept { IERR(); }
} // namespace kilipili::USB


#else
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
#endif
