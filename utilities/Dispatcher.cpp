// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Dispatcher.h"
#include "cdefs.h"
#include "utilities.h"
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

void Dispatcher::addWithDelay(Handler* handler, void* data, int32 delay) { addAtTime(handler, data, now() + delay); }

void Dispatcher::addAtTime(Handler* handler, void* data, CC when)
{
	Lock _;
	assert(num_tasks < NELEM(tasks));
	int i = num_tasks++;
	for (; i - 1 >= 0 && tasks[i - 1].when < when; i--) { tasks[i] = tasks[i - 1]; }
	Task& task	 = tasks[i];
	task.handler = handler;
	task.data	 = data;
	task.when	 = when;
}

void Dispatcher::addHandler(Handler* handler, void* data)
{
	// add task at the end of the list with when = now

	Lock _;
	assert(num_tasks < NELEM(tasks));
	Task& task	 = tasks[num_tasks++];
	task.handler = handler;
	task.data	 = data;
	task.when	 = now();
	//__sev();  may be triggered by caller if needed <hardware/sync.h>
}

void Dispatcher::addIfNew(Handler* handler, void* data)
{
	Lock _;
	for (int i = num_tasks; --i >= 0;)
	{
		if (tasks[i].handler != handler) continue;
		if (tasks[i].data != data) continue;
		return;
	}

	assert(num_tasks < NELEM(tasks));
	Task& task	 = tasks[num_tasks++];
	task.handler = handler;
	task.data	 = data;
	task.when	 = now();
}

void Dispatcher::removeHandler(Handler* handler, void* data)
{
	Lock _;
	for (int i = num_tasks; --i >= 0;)
	{
		if (tasks[i].handler != handler) continue;
		if (tasks[i].data != data) continue;
		remove(i);
		break;
	}
}

void Dispatcher::run(int timeout) noexcept
{
	if (timeout)
	{
		int i = num_tasks - 1;
		if unlikely (i < 0) return;
		wfe_or_timeout(min(timeout, tasks[i].when - now()));
	}

	int i = num_tasks - 1;
	if unlikely (i < 0) return;
	if (tasks[i].when > now()) return;

	uint zz = lock();

	i = num_tasks - 1;
	if (i >= 0)
	{
		Task&	 task	 = tasks[i];
		Handler* handler = task.handler;
		void*	 data	 = task.data;
		CC		 when	 = task.when;

		if (now() >= when)
		{
			unlock(zz);
			int delay = handler(data);
			zz		  = lock();

			for (i = num_tasks; --i >= 0;) // find again
			{
				if (tasks[i].handler != handler) continue;
				if (tasks[i].data != data) continue;
				break;
			}

			if (i >= 0) // still there?
			{
				if (delay == 0) { remove(i); }
				else // reschedule
				{
					when = delay >= 0 ? now() + delay : when - delay;
					for (; i - 1 >= 0 && tasks[i - 1].when < when; i--) { tasks[i] = tasks[i - 1]; }

					Task& task	 = tasks[i];
					task.handler = handler;
					task.data	 = data;
					task.when	 = when;
				}
			}
		}
	}

	unlock(zz);
}

int blinkOnboardLed(void*) noexcept
{
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
