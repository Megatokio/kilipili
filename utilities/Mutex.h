// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <pico/mutex.h>
#include <pico/sem.h>

class Mutex
{
	mutex_t mutex;

public:
	Mutex() { mutex_init(&mutex); }
	bool trylock() const volatile { return mutex_try_enter(const_cast<mutex_t*>(&mutex), nullptr); }
	void lock() const volatile { mutex_enter_blocking(const_cast<mutex_t*>(&mutex)); }
	void unlock() const volatile { mutex_exit(const_cast<mutex_t*>(&mutex)); }
};

// =====================================================================
// class which locks a PLock in it's ctor and
// unlocks it in it's dtor

class Locker //: public std::lock_guard<Mutex>
{
	volatile Mutex& mutex;

public:
	Locker(volatile Mutex& mutex) : mutex(mutex) { mutex.lock(); }
	~Locker() { mutex.unlock(); }
};


class Semaphore
{
	struct semaphore sema;

public:
	Semaphore(int16_t initial_permits = 1, int16_t max_permits = 1) { sem_init(&sema, initial_permits, max_permits); }

	void release() { sem_release(&sema); }
	void acquire() { sem_acquire_blocking(&sema); }
};
