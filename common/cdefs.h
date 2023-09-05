// Copyright (c) 1994 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

// test for DEBUG and RELEASE are preferred:
#if defined(NDEBUG) || defined(RELEASE)
  #undef DEBUG
  #undef NDEBUG
  #undef RELEASE
  #define NDEBUG  1
  #define RELEASE 1
#else
  #undef DEBUG
  #define DEBUG 1
#endif

#include "standard_types.h"
using Error = const char*;
#include <cassert>
#include <cerrno>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <new>
#include <pico/stdlib.h>
#include <utility>


// ######################################
// constants

static constexpr bool on	   = true;
static constexpr bool off	   = false;
static constexpr bool enabled  = true;
static constexpr bool disabled = false;


// ######################################
// #defines:

#define kB *0x400u
#define MB *0x100000u
#define GB *0x40000000ul

#define throws		noexcept(false)
#define NELEM(feld) (sizeof(feld) / sizeof((feld)[0])) // UNSIGNED !!

#define likely(x)	__builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define __deprecated __attribute__((__deprecated__))
#define __noreturn	 __attribute__((__noreturn__))

#ifndef __unused
  #define __unused __attribute__((__unused__))
#endif

#ifndef __printflike
  #define __printflike(A, B) __attribute__((format(printf, A, B)))
#endif

// this is the most portable FALLTHROUGH annotation.
// only disadvantage: you must not write a ';' after it
#define __fallthrough             \
  goto __CONCAT(label, __LINE__); \
  __CONCAT(label, __LINE__) :


// ######################################
// Abort:

__unused static constexpr cstr filenamefrompath(cstr path)
{
	cptr p = path;
	while (char c = *path++)
		if (c == '/') p = path;
	return p;
}

#define IERR() panic("IERR: %s:%u", filenamefrompath(__FILE__), __LINE__)
#define TODO() panic("TODO: %s:%u", filenamefrompath(__FILE__), __LINE__)
#define OMEM() panic("OMEM: %s:%u", filenamefrompath(__FILE__), __LINE__)

#undef assert
#ifdef DEBUG
  #define assert(COND) (likely(COND) ? ((void)0) : panic("assert: %s:%u", filenamefrompath(__FILE__), __LINE__))
#else
  #define assert(...) ((void)0)
#endif


// ######################################
// Debugging:

#undef debugstr
#ifdef DEBUG
  #define debugstr printf
#else
  #define debugstr(...) ((void)0)
#endif

#if !defined debug_break
  #define debug_break() __asm__ volatile("bkpt")
#endif

#define LOL printf("@%s:%u\n", filenamefrompath(__FILE__), __LINE__);


// ######################################
// Initialization at start:
//   • during statics initialization
//   • only once
//   • beware of undefined sequence of inclusion!
//   • use:  -->  ON_INIT(function_to_call);
//   • use:  -->  ON_INIT([]{ init_foo(); init_bar(); });

struct on_init
{
	on_init(void (*f)()) { f(); }
};
#define ON_INIT(fu) static on_init __CONCAT(z, __LINE__)(fu)


// ######################################
// basic maths

template<class T, class T2>
inline constexpr T min(T a, T2 b) noexcept
{
	return a < b ? a : b;
}
template<class T, class T2>
inline constexpr T max(T a, T2 b) noexcept
{
	return a > b ? a : b;
}
template<class T>
inline int sign(T a) noexcept
{
	return int(a > 0) - int(a < 0);
}
template<class T>
inline T abs(T a) noexcept
{
	return a < 0 ? -a : a;
}
template<class T>
inline T minmax(T a, T n, T e) noexcept
{
	return n <= a ? a : n >= e ? e : n;
}
template<class T>
inline void limit(T a, T& n, T e) noexcept
{
	if (n < a) n = a;
	else if (n > e) n = e;
}


// ######################################
// class helper:

#define DEFAULT_COPY(T)             \
  T(const T&)			 = default; \
  T& operator=(const T&) = default

#define DEFAULT_MOVE(T)                 \
  T(T&&) noexcept			 = default; \
  T& operator=(T&&) noexcept = default

#define NO_COPY(T)                 \
  T(const T&)			 = delete; \
  T& operator=(const T&) = delete

#define NO_MOVE(T)            \
  T(T&&)			= delete; \
  T& operator=(T&&) = delete

#define NO_COPY_MOVE(T) \
  NO_COPY(T);           \
  NO_MOVE(T)
