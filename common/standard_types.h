// Copyright (c) 1999 - 2023 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include <cctype>
#include <cstdint>


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
#define MHz *1000000u


// ######################################
// throwable cstring errors:

using Error					  = const char*;
constexpr Error NO_ERROR	  = nullptr;
constexpr Error OUT_OF_MEMORY = "out of memory";


// ######################################
// exact size defs:

using uint8	 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8	 = int8_t;
using int16	 = int16_t;
using int32	 = int32_t;
using int64	 = int64_t;

using uint8ptr	= uint8*;
using uint16ptr = uint16*;
using uint32ptr = uint32*;
using uint64ptr = uint64*;
using int8ptr	= int8*;
using int16ptr	= int16*;
using int32ptr	= int32*;
using int64ptr	= int64*;

using cuint8  = const uint8_t;
using cuint16 = const uint16_t;
using cuint32 = const uint32_t;
using cuint64 = const uint64_t;
using cint8	  = const int8_t;
using cint16  = const int16_t;
using cint32  = const int32_t;
using cint64  = const int64_t;

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
