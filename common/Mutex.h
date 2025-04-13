// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

#ifdef LIB_PICO_STDLIB
  #include <pico/mutex.h>

namespace kio
{
class Mutex
{
	mutex_t mutex;

public:
	Mutex() noexcept { mutex_init(&mutex); }
	bool try_lock() noexcept { return mutex_try_enter(&mutex, nullptr); }
	void lock() noexcept { mutex_enter_blocking(&mutex); }
	void unlock() noexcept { mutex_exit(&mutex); }
};
} // namespace kio

#else
  #include <mutex>

namespace kio
{
using Mutex = std::mutex;
}

#endif


// =====================================================================
// class which locks a mutex in it's ctor and
// unlocks it in it's dtor

namespace kio
{
class Locker
{
	Mutex& mutex;

public:
	Locker(Mutex& mutex) : mutex(mutex) { mutex.lock(); }
	~Locker() { mutex.unlock(); }
};
} // namespace kio
