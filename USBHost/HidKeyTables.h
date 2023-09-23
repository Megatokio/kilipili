// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"


namespace kio::USB
{

constexpr uint KEY_TRANSLATION_TABLE_SIZE = 0x68;
using KeyTable							  = uchar[KEY_TRANSLATION_TABLE_SIZE];


extern const KeyTable key_table_us;
extern const KeyTable key_table_us_shift;

extern const KeyTable key_table_ger;
extern const KeyTable key_table_ger_shift;

} // namespace kio::USB
