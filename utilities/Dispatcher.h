// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "basic_math.h"
#include <hardware/timer.h>

/*
	The Dispatcher allows you to run state machines in parallel to the main program
	and to convert interrupts into synchronous events.
	The Dispatcher only has static functions and requires no real instance.
*/

namespace kio
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

class Dispatcher
{
public:
	/*	Add a handler optionally with void data pointer.
		Handler will be called on next run() or after delay or at set time cc.
		addHandler() is ideal for converting interrupts into synchronous events.
		addWithDelay() and addAtTime() are ideal for one-shot or repeating timers.
	*/
	static void addHandler(Handler*, void* data = nullptr);
	static void addIfNew(Handler*, void* data = nullptr);
	static void addWithDelay(Handler*, void* data, int32 delay);
	static void addAtTime(Handler*, void* data, CC when);

	/*	Remove a handler, either identified by the function or if needed by function and data.
		Be cautious if you remove a handler from an interrupt or from core 1:
		In a race condition the handler may be still executed while or after you removed it.
		(Removing the handler from a synchronous event or telling it to return 0 may help.)
	*/
	static void removeHandler(Handler*, void* data = nullptr);

	/*	Run the next handler if scheduled time is reached.
		Always calls only one handler at a time.
		If timeout>0 then wait for timeout or the next scheduled time. ("idle")
	***	Returns quickly if timeout==0 and no handler needs to run  
		to allow frequent polling by the main program.
	*/
	static void run(int timeout = 0) noexcept;

private:
	Dispatcher() noexcept = default;
};


// A Handler which blinks the on-board LED of the Pico board
extern Handler blinkOnboardLed;


} // namespace kio
