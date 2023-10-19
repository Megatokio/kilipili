// Copyright (c) 1994 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <cstdlib> // __CONCAT
#ifdef UNIT_TEST
extern __attribute__((format(printf, 1, 2))) __attribute__((__noreturn__)) void panic(const char* fmt, ...);
#else
  #include <pico/platform.h> // panic()
#endif


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


// ######################################
// #defines:

#define throws		noexcept(false)
#define NELEM(feld) (sizeof(feld) / sizeof((feld)[0])) // UNSIGNED !!

#define likely(x)	(__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

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

__unused static constexpr const char* filenamefrompath(const char* path)
{
	const char* p = path;
	while (char c = *p++)
		if (c == '/') path = p;
	return path;
}

#define IERR() panic("IERR: %s:%u", filenamefrompath(__FILE__), __LINE__)
#define TODO() panic("TODO: %s:%u", filenamefrompath(__FILE__), __LINE__)
#define OMEM() panic("OMEM: %s:%u", filenamefrompath(__FILE__), __LINE__)

#undef assert
#ifdef DEBUG
// clang-format off
  #define assert(COND) ((COND) ? (void)0 : panic("assert: %s:%u", filenamefrompath(__FILE__), __LINE__))
  #define assert_eq(A, B) ((A) == (B) ? (void)0 : panic("failed: %s:%u: (%li) == (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
  #define assert_ne(A, B) ((A) != (B) ? (void)0 : panic("failed: %s:%u: (%li) != (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
  #define assert_lt(A, B) ((A) <  (B) ? (void)0 : panic("failed: %s:%u: (%li) < (%li)",  filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
  #define assert_le(A, B) ((A) <= (B) ? (void)0 : panic("failed: %s:%u: (%li) <= (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
  #define assert_gt(A, B) ((A) >  (B) ? (void)0 : panic("failed: %s:%u: (%li) > (%li)",  filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
  #define assert_ge(A, B) ((A) >= (B) ? (void)0 : panic("failed: %s:%u: (%li) >= (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
// clang-format on
#else
  #define assert(...)	  ((void)0)
  #define assert_eq(A, B) ((void)0)
  #define assert_ne(A, B) ((void)0)
  #define assert_lt(A, B) ((void)0)
  #define assert_le(A, B) ((void)0)
  #define assert_gt(A, B) ((void)0)
  #define assert_ge(A, B) ((void)0)
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

#define LOL ::printf("@%s:%u\n", filenamefrompath(__FILE__), __LINE__);
