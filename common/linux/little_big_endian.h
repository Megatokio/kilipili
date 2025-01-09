// Copyright (c) 1994 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include<bit>

#if defined __cpp_lib_endian

namespace kio
{
static constexpr bool little_endian = std::endian::native == std::endian::little;
static constexpr bool big_endian = std::endian::native == std::endian::big;
}
 
#else
  
#if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)

  #if defined(__BYTE_ORDER__)
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	  #define __LITTLE_ENDIAN__ 1
	#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	  #define __BIG_ENDIAN__ 1
	#else
	  #error "__PDP_ENDIAN__"
	#endif

  #elif defined(__FreeBSD__) || defined(__NetBSD__)
	#include <sys/endian.h>
	#if defined(_BYTE_ORDER)
	  #if _BYTE_ORDER == _LITTLE_ENDIAN
		#define __LITTLE_ENDIAN__ 1
	  #elif _BYTE_ORDER == _BIG_ENDIAN
		#define __BIG_ENDIAN__ 1
	  #else
		#error "__PDP_ENDIAN__"
	  #endif
	#endif

  #elif defined(_WIN32)
	#define __LITTLE_ENDIAN__ 1

  #elif defined(__sun__)
	#include <sys/isa_defs.h>
	#if defined(_LITTLE_ENDIAN) && !defined(_BIG_ENDIAN)
	  #define __LITTLE_ENDIAN__ 1
	#elif defined(_BIG_ENDIAN) && !defined(_LITTLE_ENDIAN)
	  #define __BIG_ENDIAN__ 1
	#endif
  #endif

  #if !defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
	#include <endian.h>
	#if defined(BYTE_ORDER)
	  #if BYTE_ORDER == LITTLE_ENDIAN
		#define __LITTLE_ENDIAN__ 1
	  #elif BYTE_ORDER == BIG_ENDIAN
		#define __BIG_ENDIAN__ 1
	  #else
		#error "__PDP_ENDIAN__"
	  #endif
	#elif defined(__BYTE_ORDER)
	  #if __BYTE_ORDER == __LITTLE_ENDIAN
		#define __LITTLE_ENDIAN__ 1
	  #elif __BYTE_ORDER == __BIG_ENDIAN
		#define __BIG_ENDIAN__ 1
	  #else
		#error "__PDP_ENDIAN__"
	  #endif
	#elif defined(_BYTE_ORDER)
	  #if _BYTE_ORDER == _LITTLE_ENDIAN
		#define __LITTLE_ENDIAN__ 1
	  #elif _BYTE_ORDER == _BIG_ENDIAN
		#define __BIG_ENDIAN__ 1
	  #else
		#error "__PDP_ENDIAN__"
	  #endif
	#endif
  #endif
#endif

namespace kio
{
#if defined __LITTLE_ENDIAN__ && __LITTLE_ENDIAN__
static constexpr bool little_endian = 1;
static constexpr bool big_endian = 0;
#elif defined __BIG_ENDIAN__ && __BIG_ENDIAN__
static constexpr bool little_endian = 0;
static constexpr bool big_endian = 1;
#endif
}

#endif 

