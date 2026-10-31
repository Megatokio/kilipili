
#define char_height 12
#define num_codes	256

// clang-format off

#include "define_r.h"
#include "block_graphics_8.h"
#include "char_2px_left_vertical_bar.h"
#include "char_6px_left_vertical_bar.h"
#include "line_graphics_2px_centered_6.h"
#include "line_graphics_box_outline_8.h"

// symbols:
#include "char_bullet.h" 		 // 0x98
#include "char_triangle_right.h" // 0x99
#include "char_arrow_right.h"	 // 0x9A
#include "char_checkmark.h"		 // 0x9B
#include "char_failed.h"		 // 0x9C
#include "char_no_glyph.h"		 // hollow bullet?	triangle down?
#include "char_no_glyph.h"		 // connection line top-right?		
#include "char_no_glyph.h"		 // connection line top-bottom-right?

// 0x20:
#include "ascii_0x20-0x7e.h"
#include "char_0x7f.h"

// 0x80:
#include"define_r_inverse.h"
#include "block_graphics_8.h"	// => same as in ascii_xxx
#include "char_2px_left_vertical_bar.h"
#include "char_6px_left_vertical_bar.h"
#include "define_r.h"
#include "block_graphics_for_buttons_6.h"
#include "signs.h"

// 0xA0:
#include "latin1_extension_0xa0-0xff.h"

