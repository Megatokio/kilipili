
#define char_height 12
#define num_codes	256

// lsbit is left => revert bits
#define r(b) ((b & 128) >> 7) + ((b & 64) >> 5) + ((b & 32) >> 3) + ((b & 16) >> 1) + \
			 ((b & 8) << 1) + ((b & 4) << 3) + ((b & 2) << 5) + ((b & 1) << 7)

#include "glyphs_12x8/lcd_0-f.h"
#include "glyphs_12x8/signs.h"
#include "glyphs_12x8/ascii_0x20-0x7e.h"
#include "glyphs_12x8/char_0x7f.h"

#include "glyphs_12x8/block_graphics_0x80-0x89.h"
#include "glyphs_12x8/line_graphics_0x8a-0x97.h"
#include "glyphs_12x8/char_bullet.h" 		 // 0x98
#include "glyphs_12x8/char_triangle_right.h" // 0x99
#include "glyphs_12x8/char_arrow_right.h"	 // 0x9A
#include "glyphs_12x8/char_checkmark.h"		 // 0x9B
#include "glyphs_12x8/char_failed.h"		 // 0x9C
#include "glyphs_12x8/all_black.h"			 // 0x9D undecided
#include "glyphs_12x8/char_no_glyph.h"		 // 0x9E undefined
#include "glyphs_12x8/char_no_glyph.h"		 // 0x9F undefined

#include "glyphs_12x8/latin1_extension_0xa0-0xff.h"

