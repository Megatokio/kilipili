// Copyright (c) 2025 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "systemfont.h"


namespace kilipili::Graphics
{

// old format: all bytes of each char in one block:
//
const unsigned char systemfont256x12[256 * 12] = {
#include "rsrc/latin1_12x8_source.h"
};


// new format: all bytes of each scanline in one block:
//
const unsigned char latin1_256x12[12][256] = {
#include "rsrc/latin1_12x8.h"
};

const unsigned char ascii_256x12_inverse[12][256] = {
#include "rsrc/ascii_12x8_inverse.h"
};


} // namespace kilipili::Graphics
