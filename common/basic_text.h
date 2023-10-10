// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"


inline constexpr bool is_hex_digit(uchar c) noexcept
{
	return uchar(c - '0') <= '9' - '0' || uchar((c | 0x20) - 'a') <= 'f' - 'a';
}

inline constexpr uint hex_digit_value(uchar c) noexcept // non-digits ≥ 36
{
	return c <= '9' ? uchar(c - '0') : uchar((c | 0x20) - 'a') + 10;
}

inline constexpr char to_lower(char c) noexcept { return uchar(c - 'A') <= 'Z' - 'A' ? c | 0x20 : c; }

inline constexpr bool lceq(cstr s, cstr t) noexcept
{
	if (s && t)
	{
		while (*s)
			if (to_lower(*s++) != to_lower(*t++)) return false;
		return *t == 0;
	}
	else return (!s || !*s) && (!t || !*t);
}
