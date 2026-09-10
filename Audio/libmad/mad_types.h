// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/standard_types.h"

// fixed float type:
using mad_fixed_t	  = int32; // signed
using mad_fixed64hi_t = int32; // signed
using mad_fixed64lo_t = uint32;
using mad_fixed64_t	  = int64; // signed
using mad_sample_t	  = mad_fixed_t;

// number of fractional bits in mad_fixed_t: DO NOT CHANGE!
#define MAD_F_FRACBITS 28
