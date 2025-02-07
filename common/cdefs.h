// Copyright (c) 1994 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <cstdlib> // __CONCAT


// test for DEBUG and RELEASE are preferred:
#if defined(NDEBUG) || defined(RELEASE)
  #undef DEBUG
  #undef NDEBUG
  #undef RELEASE
  #define NDEBUG  1
  #define RELEASE 1
constexpr bool debug = false;
#else
  #undef DEBUG
  #define DEBUG 1
constexpr bool debug = true;
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

#ifndef __weak_symbol
  #define __weak_symbol __attribute__((__weak__))
#endif

#ifndef __aligned
  #define __aligned(x) __attribute__((__aligned__(x)))
#endif

#ifndef __printflike
  #define __printflike(A, B) __attribute__((format(printf, A, B)))
#endif

#ifndef __packed
  #define __packed __attribute__((__packed__))
#endif

// this is the most portable FALLTHROUGH annotation.
// only disadvantage: you must not write a ';' after it
#define __fallthrough               \
	goto __CONCAT(label, __LINE__); \
	__CONCAT(label, __LINE__) :


// ######################################
// Abort:

static constexpr const __unused char* filenamefrompath(const char* path)
{
	const char* p = path;
	while (char c = *p++)
		if (c == '/') path = p;
	return path;
}

//extern "C" __printflike(1, 2) __noreturn void panic(const char* fmt, ...);
namespace kio
{
extern void __noreturn __printflike(1, 2) panic(const char* fmt, ...);
};
#define IERR() kio::panic("IERR: %s:%i", filenamefrompath(__FILE__), __LINE__)
#define TODO() kio::panic("TODO: %s:%i", filenamefrompath(__FILE__), __LINE__)
#define OMEM() kio::panic("OMEM: %s:%i", filenamefrompath(__FILE__), __LINE__)

#undef assert
// clang-format off
#define assert(COND)    (!debug || (COND)     ? (void)0 : kio::panic("assert: %s:%i", filenamefrompath(__FILE__), __LINE__))
#define assert_eq(A, B) (!debug || (A) == (B) ? (void)0 : kio::panic("failed: %s:%i: (%li) == (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
#define assert_ne(A, B) (!debug || (A) != (B) ? (void)0 : kio::panic("failed: %s:%i: (%li) != (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
#define assert_lt(A, B) (!debug || (A) <  (B) ? (void)0 : kio::panic("failed: %s:%i: (%li) < (%li)",  filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
#define assert_le(A, B) (!debug || (A) <= (B) ? (void)0 : kio::panic("failed: %s:%i: (%li) <= (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
#define assert_gt(A, B) (!debug || (A) >  (B) ? (void)0 : kio::panic("failed: %s:%i: (%li) > (%li)",  filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
#define assert_ge(A, B) (!debug || (A) >= (B) ? (void)0 : kio::panic("failed: %s:%i: (%li) >= (%li)", filenamefrompath(__FILE__),__LINE__,long(A),long(B)))
// clang-format on


// ######################################
// Debugging:

#undef debugstr
#ifdef DEBUG
  #define debugstr ::printf
#else
  #define debugstr(...) ((void)0)
#endif

#if !defined debug_break
  #define debug_break() __asm__ volatile("bkpt")
#endif

#define LOL                                                         \
	do {                                                            \
		::printf("@%s:%i\n", filenamefrompath(__FILE__), __LINE__); \
		stdio_flush();                                              \
	}                                                               \
	while (0);

#if defined DEBUG || defined MEMORY_ID
  #define Id(n) \
	  char _id[8] { n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7] }
#else
  #define Id(n) static const char _id[8]
#endif
