// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Dispatcher.h"
#include "Trace.h"
#include "cdefs.h"
#include "utilities.h"
#include <cstdio>
#include <pico/sync.h>

#ifndef DISPATCHER_MAX_TASKS
  #define DISPATCHER_MAX_TASKS 10
#endif


namespace kio
{

static spin_lock_t* spinlock = spin_lock_instance(next_striped_spin_lock_num());
static uint32		lock() noexcept { return spin_lock_blocking(spinlock); }
static void			unlock(uint32 sr) noexcept { spin_unlock(spinlock, sr); }

struct Lock
{
	uint32 sr;
	Lock() noexcept : sr(lock()) {}
	~Lock() noexcept { unlock(sr); }
};

struct Task
{
	Handler* handler;
	void*	 data;
	CC		 when;
};

static Task	 tasks[DISPATCHER_MAX_TASKS];
static uint8 num_tasks = 0;


static inline void remove(int i)
{
	for (num_tasks--; i < num_tasks; i++) { tasks[i] = tasks[i + 1]; }
}

static void add(Handler* handler, const void* data, CC when)
{
	assert(num_tasks < NELEM(tasks));
	int i = num_tasks++;
	for (; i - 1 >= 0 && tasks[i - 1].when < when; i--) { tasks[i] = tasks[i - 1]; }
	Task& task	 = tasks[i];
	task.handler = handler;
	task.data	 = const_cast<void*>(data);
	task.when	 = when;
	//__sev();  may be triggered by caller if needed <hardware/sync.h>
}

static int index_of(Handler* handler, const void* data)
{
	int i = num_tasks;
	while (--i >= 0)
	{
		if (tasks[i].handler == handler && tasks[i].data == data) break;
	}
	return i;
}

void Dispatcher::addWithDelay(Handler* handler, const void* data, int32 delay)
{
	Lock _;
	add(handler, data, now() + delay);
}

void Dispatcher::addAtTime(Handler* handler, const void* data, CC when)
{
	Lock _;
	add(handler, data, when);
}

void Dispatcher::addHandler(Handler* handler, const void* data)
{
	Lock _;
	add(handler, data, now());
}

void Dispatcher::addIfNew(Handler* handler, const void* data)
{
	Lock _;
	if (index_of(handler, data) < 0) add(handler, data, now());
}

void Dispatcher::removeHandler(Handler* handler, const void* data)
{
	Lock _;
	int	 i = index_of(handler, data);
	if (i >= 0) remove(i);
}

void Dispatcher::run(int timeout) noexcept
{
	trace(__func__);

	if (timeout)
	{
		uint8 n = num_tasks;
		wfe_or_timeout(n ? min(timeout, tasks[n - 1].when - now()) : timeout);
	}

	uint8 n = const_cast<volatile uint8&>(num_tasks);
	if (n == 0) return;
	if (tasks[n - 1].when > now()) return;

	uint zz = lock();

	if (uint8 n = const_cast<volatile uint8&>(num_tasks))
	{
		Task&	 task	 = tasks[--n];
		Handler* handler = task.handler;
		void*	 data	 = task.data;
		CC		 when	 = task.when;

		if (now() >= when)
		{
			num_tasks = n;
			unlock(zz);
			//printf("call %s\n", cstr(data));
			int delay = handler(data);
			zz		  = lock();

			if (delay) // reschedule
			{
				when = delay >= 0 ? now() + delay : when - delay;
				add(handler, data, when);
			}
		}
	}

	unlock(zz);
}

int blinkOnboardLed(void*) noexcept
{
	trace(__func__);

#ifdef PICO_DEFAULT_LED_PIN
	static constexpr uint pin = PICO_DEFAULT_LED_PIN;
	static struct Init
	{
		Init()
		{
			gpio_init(pin);
			gpio_set_dir(pin, GPIO_OUT);
		}
	} foo;

	gpio_xor_mask(1 << pin);
	return -500 * 1000; // 1 Hz drift-free
#else
	return 0; // remove me
#endif
}


} // namespace kio


/*







































*/
