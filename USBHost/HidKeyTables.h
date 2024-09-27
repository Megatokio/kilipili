// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"


namespace kio::USB
{

struct HidKeyTable
{
	static constexpr uint table_size = 0x68;

	cstr		 name;
	const uchar* solo;
	const uchar* with_shift;
	const uchar* with_alt;
	const uchar* with_shift_alt;

	char get_key(uint key, bool shift, bool alt) const noexcept
	{
		const uchar* table = (&solo)[(shift) + (2 * alt)];
		return key < table_size ? char(table[key]) : 0;
	}
};

extern const HidKeyTable key_table_us;
extern const HidKeyTable key_table_ger;

} // namespace kio::USB
