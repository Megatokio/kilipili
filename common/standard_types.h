// Copyright (c) 1999 - 2023 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include <cctype>
#include <cstdint>
#include <limits.h>
#include <sys/types.h>


// ######################################
// boolean aliases:

static constexpr bool on	   = true;
static constexpr bool off	   = false;
static constexpr bool enabled  = true;
static constexpr bool disabled = false;


// ######################################
// multiplier #defines:

#define kB	*0x400u
#define MB	*0x100000u
#define GB	*0x40000000ul
#define kHz *1000u
#define MHz *1000000u


// ######################################
// throwable cstring errors:

using Error						= const char*;
constexpr Error NO_ERROR		= nullptr;
constexpr char	OUT_OF_MEMORY[] = "out of memory";


// ######################################
// exact size defs:


#if UCHAR_MAX == 0xff
using uint8 = unsigned char;
using int8	= signed char;
#else
using uint8	 = uint8_t;
using int8	 = int8_t;
#endif

#if SHRT_MAX == 0x7fff
using uint16 = unsigned short;
using int16	 = signed short;
#else
using uint16 = uint16_t;
using int16	 = int16_t;
#endif

#if INT_MAX == 0x7fffffff
using uint32 = unsigned int;
using int32	 = signed int;
#else
using uint32 = uint32_t;
using int32	 = int32_t;
#endif

#if LONG_LONG_MAX == 0x7fffffffffffffffl
using uint64 = unsigned long long;
using int64	 = signed long long;
#else
using uint64 = uint64_t;
using int64	 = int64_t;
#endif

using uint8ptr	= uint8*;
using uint16ptr = uint16*;
using uint32ptr = uint32*;
using uint64ptr = uint64*;
using int8ptr	= int8*;
using int16ptr	= int16*;
using int32ptr	= int32*;
using int64ptr	= int64*;

using cuint8  = const uint8;
using cuint16 = const uint16;
using cuint32 = const uint32;
using cuint64 = const uint64;
using cint8	  = const int8;
using cint16  = const int16;
using cint32  = const int32;
using cint64  = const int64;

using cuint8ptr	 = const uint8*;
using cuint16ptr = const uint16*;
using cuint32ptr = const uint32*;
using cuint64ptr = const uint64*;
using cint8ptr	 = const int8*;
using cint16ptr	 = const int16*;
using cint32ptr	 = const int32*;
using cint64ptr	 = const int64*;


// ######################################
// minimum width defs:

// ISO: min. sizeof(char)   =  undefined !
//		min. sizeof(short)  =  undefined !
//		min. sizeof(int)    =  16 bit    !
//		min. sizeof(long)   =  32 bit
//		min. sizeof(llong)  =  64 bit

using schar = signed char;
using llong = long long;

using uchar	 = unsigned char;
using ushort = unsigned short;
using uint	 = unsigned int;
using ulong	 = unsigned long;
using ullong = unsigned long long;

using charptr  = char*;
using shortptr = short*;
using intptr   = int*;
using longptr  = long*;
using llongptr = llong*;

using ucharptr	= uchar*;
using ushortptr = ushort*;
using uintptr	= uint*;
using ulongptr	= ulong*;
using ullongptr = ullong*;


// ######################################
// character pointers and strings:

using str  = char*;
using cstr = const char*;

using ptr  = char*;
using cptr = const char*;

using uptr	= uchar*;
using cuptr = const uchar*;


//
