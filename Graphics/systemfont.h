// Copyright (c) 2025 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

namespace kilipili::Graphics
{

// new format: all bytes of each scanline in one block:
//
extern const unsigned char latin1_256x12[12][256];
extern const unsigned char ascii_256x12_inverse[12][256];
extern const unsigned char ascii_256x12_bold[12][256];

//	0x20 .. 0x2F    4/4 Block Graphics, black&white
//	0x30 .. 0x3F:   4/4 Block Graphics, grey/white
//	0x40 .. 0x4F:   4/4 Block Graphics, black/grey
//	0x50 .. 0x57:   1/8  .. 8/8   Bargraph from left
//	0x58 .. 0x5F:   1/8  .. 8/8   Bargraph from right
//	0x60 .. 0x6B:   1/12 .. 12/12 Bargraph from bottom
//	0x6C .. 0x77:   1/12 .. 12/12 Bargraph from top
//	0x78 .. 0xAF:   (undefined)
//	0xB0 .. 0xFF:   Line Graphics
//
//	The line graphics are calculated with the following formula:
//		0xAF + A*27 + B*9 + C*3 * D
//		where A/B/C/D = left/top/right/bottom line stub
//		with 0/1/2 = no/thin/thick line
//		code point 0xAF (no line at all) does not exist
extern const unsigned char graphics_256x12[12][256];


// clang-format off
enum:char
{
	block_space=0,
	block_br,
	block_bl,
	block_bottom,
	block_tr,
	block_right,
	block_bl_tr,
	block_bottom_right,
	block_left_14,		// for progress bar
	block_left_34,		// for progress bar

	line2_corner_tl,	// outlined button or text box
	line2_corner_tr,
	line2_corner_bl,
	line2_corner_br,
	line2_horizontal,
	line2_vertical,

	box_outline_tl,
	box_outline_tr,
	box_outline_bl,
	box_outline_br,
	box_outline_bottom,
	box_outline_top,
	box_outline_right,
	box_outline_left,

	symbol_bullet,
	symbol_triangle_right,
	symbol_arrow_right,
	symbol_checkmark,
	symbol_failed_mark,
	symbol_tbd_5,
	symbol_tbd_6,
	symbol_tbd_7,

	symbol_rubbout = 0x7f,

	block_black,
	block_top_left,
	block_top_right,
	block_top,
	block_bottom_left,
	block_left,
	block_tl_br,
	block_tl,
	block_right_34,
	block_right_14,

	block_button_tl,
	block_button_top,
	block_button_tr,
	block_button_bl,
	block_button_bottom,
	block_button_br,

	symbol_info_tl,
	symbol_info_tr,
	symbol_info_bl,
	symbol_info_br,

	symbol_warning_tl,
	symbol_warning_tm,
	symbol_warning_tr,
	symbol_warning_bl,
	symbol_warning_bm,
	symbol_warning_br,
	
	symbol_error_tl,
	symbol_error_tm,
	symbol_error_tr,
	symbol_error_bl,
	symbol_error_bm,
	symbol_error_br,
};
// clang-format on

static_assert(symbol_tbd_7 == 0x20 - 1);
static_assert(symbol_error_br == 0xa0 - 1);

} // namespace kilipili::Graphics
