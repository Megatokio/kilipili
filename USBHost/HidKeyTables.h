// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"


#ifndef USE_WIDECHARS
  #define USE_WIDECHARS 0
#endif

#if USE_WIDECHARS
using Char = uint16;
#else
using Char = uchar;
#endif


namespace kipili::USB
{

constexpr uint KEY_TRANSLATION_TABLE_SIZE = 0x68;
using KeyTable							  = Char[KEY_TRANSLATION_TABLE_SIZE];


extern const KeyTable key_table_us;
extern const KeyTable key_table_us_shift;

extern const KeyTable key_table_ger;
extern const KeyTable key_table_ger_shift;

} // namespace kipili::USB
