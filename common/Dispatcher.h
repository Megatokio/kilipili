// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "basic_math.h"

/*
	The Dispatcher allows you to run state machines in parallel to the main program
	and to convert interrupts into synchronous events.
*/

namespace kio
{
namespace Dispatcher
{

/*	Type of function which can be registered with the Dispatcher.
	- the return value indicates the delay in µsec when to call again:
	  rval > 0	 call again after rval µs
	  rval == 0	 don't call again, remove me 
	  rval < 0	 call again -rval µsec after last scheduled time for drift-free callback times
	- The Handler must not throw.
	- If the Handler uses temp strings then it must preserve the caller's tempmem:
	  either create a TempMemSave or a TempMemOnStack (or a TempMem on heap).
*/
using Handler = int(void* data) noexcept;

/*	Add a handler optionally with void data pointer.
		Handler will be called on next run() or after delay or at set time cc.
		addHandler() is ideal for converting interrupts into synchronous events.
		addWithDelay() and addAtTime() are ideal for one-shot or repeating timers.
	*/
extern void addHandler(Handler*, const void* data = nullptr);
extern void addIfNew(Handler*, const void* data = nullptr);
extern void addWithDelay(Handler*, const void* data, int32 delay);
extern void addAtTime(Handler*, const void* data, CC when);

/*	Remove a handler, either identified by the function or if needed by function and data.
		Be cautious if you remove a handler from an interrupt or from core 1:
		In a race condition the handler may be still executed while or after you removed it.
		(Removing the handler from a synchronous event or telling it to return 0 may help.)
	*/
extern void removeHandler(Handler*, const void* data = nullptr);

/*	Run the next handler if scheduled time is reached.
		Always calls only one handler at a time.
		If timeout>0 then wait for timeout or the next scheduled time. ("idle")
	***	Returns quickly if timeout==0 and no handler needs to run  
		to allow frequent polling by the main program.
	*** The currently executed handler is removed from the dispatcher list.
		=> a handler which discovers it has to do a longish job can recursively
		   call Dispatcher::run() to keep the rest of the system happy.
		=> a handler cannot be removed from within and not safely from the other core.
		   to remove a handler from within the handler can simply return 0.
	*/
extern void run(int timeout = 0) noexcept;

} // namespace Dispatcher


// short name:
inline void disp(int timeout = 0) noexcept { Dispatcher::run(timeout); }

// A Handler which blinks the on-board LED of the Pico board
extern Dispatcher::Handler blinkOnboardLed;


} // namespace kio
