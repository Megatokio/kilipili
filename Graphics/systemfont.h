// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once

namespace kio::Graphics
{

extern const unsigned char systemfont256x12[256 * 12];

// clang-format off
enum:char
{
	lcd0, lcd1,	lcd2,lcd3,lcd4,lcd5,lcd6,lcd7, // tbd.
	lcd8, lcd9,	lcdA,lcdB,lcdC,lcdD,lcdE,lcdF, // tbd.

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

	symbol_rubbout = 0x7f,

	block_br = char(0x80),	// for buttons
	block_bl,
	block_tr,
	block_tl,
	block_bottom,
	block_top,
	block_right,
	block_left,

	block_left_14,		// for progress bar
	block_left_34,		// for progress bar

	line2_corner_tl,	// outlined button or text box
	line2_corner_tr,
	line2_corner_bl,
	line2_corner_br,
	line2_horizontal,
	line2_vertical,

	line1_corner_tl,	// possibly for frame around dialog
	line1_corner_tr,
	line1_corner_bl,
	line1_corner_br,
	line1_hor_top,
	line1_hor_bottom,
	line1_vert_left,
	line1_vert_right,

	symbol_bullet,
	symbol_triangle_right,
	symbol_arrow_right,
	symbol_checkmark,
	symbol_failed_mark,
	symbol_black,		// possibly for cursor
	symbol_9E,
	symbol_9F
};
// clang-format on

} // namespace kio::Graphics
