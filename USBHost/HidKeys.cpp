// Copyright (c) 2022 - 2022 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HidKeys.h"

const char* const hidkey[0xA4 + 1] = {
	"NO_KEY", // 0x00
	"rollover_error",
	"post_failed",
	"other_error",
	"A",				   // 0x04
	"B",				   // 0x05
	"C",				   // 0x06
	"D",				   // 0x07
	"E",				   // 0x08
	"F",				   // 0x09
	"G",				   // 0x0A
	"H",				   // 0x0B
	"I",				   // 0x0C
	"J",				   // 0x0D
	"K",				   // 0x0E
	"L",				   // 0x0F
	"M",				   // 0x10
	"N",				   // 0x11
	"O",				   // 0x12
	"P",				   // 0x13
	"Q",				   // 0x14
	"R",				   // 0x15
	"S",				   // 0x16
	"T",				   // 0x17
	"U",				   // 0x18
	"V",				   // 0x19
	"W",				   // 0x1A
	"X",				   // 0x1B
	"Y",				   // 0x1C
	"Z",				   // 0x1D
	"1",				   // 0x1E
	"2",				   // 0x1F
	"3",				   // 0x20
	"4",				   // 0x21
	"5",				   // 0x22
	"6",				   // 0x23
	"7",				   // 0x24
	"8",				   // 0x25
	"9",				   // 0x26
	"0",				   // 0x27
	"ENTER",			   // 0x28
	"ESCAPE",			   // 0x29
	"BACKSPACE",		   // 0x2A
	"TAB",				   // 0x2B
	"SPACE",			   // 0x2C
	"MINUS",			   // 0x2D
	"EQUAL",			   // 0x2E
	"BRACKET_LEFT",		   // 0x2F
	"BRACKET_RIGHT",	   // 0x30
	"BACKSLASH",		   // 0x31
	"EUROPE_1",			   // 0x32
	"SEMICOLON",		   // 0x33
	"APOSTROPHE",		   // 0x34
	"GRAVE",			   // 0x35
	"COMMA",			   // 0x36
	"PERIOD",			   // 0x37
	"SLASH",			   // 0x38
	"CAPS_LOCK",		   // 0x39
	"F1",				   // 0x3A
	"F2",				   // 0x3B
	"F3",				   // 0x3C
	"F4",				   // 0x3D
	"F5",				   // 0x3E
	"F6",				   // 0x3F
	"F7",				   // 0x40
	"F8",				   // 0x41
	"F9",				   // 0x42
	"F10",				   // 0x43
	"F11",				   // 0x44
	"F12",				   // 0x45
	"PRINT_SCREEN",		   // 0x46
	"SCROLL_LOCK",		   // 0x47
	"PAUSE",			   // 0x48
	"INSERT",			   // 0x49
	"HOME",				   // 0x4A
	"PAGE_UP",			   // 0x4B
	"DELETE",			   // 0x4C
	"END",				   // 0x4D
	"PAGE_DOWN",		   // 0x4E
	"ARROW_RIGHT",		   // 0x4F
	"ARROW_LEFT",		   // 0x50
	"ARROW_DOWN",		   // 0x51
	"ARROW_UP",			   // 0x52
	"NUM_LOCK",			   // 0x53
	"KEYPAD_DIVIDE",	   // 0x54
	"KEYPAD_MULTIPLY",	   // 0x55
	"KEYPAD_SUBTRACT",	   // 0x56
	"KEYPAD_ADD",		   // 0x57
	"KEYPAD_ENTER",		   // 0x58
	"KEYPAD_1",			   // 0x59
	"KEYPAD_2",			   // 0x5A
	"KEYPAD_3",			   // 0x5B
	"KEYPAD_4",			   // 0x5C
	"KEYPAD_5",			   // 0x5D
	"KEYPAD_6",			   // 0x5E
	"KEYPAD_7",			   // 0x5F
	"KEYPAD_8",			   // 0x60
	"KEYPAD_9",			   // 0x61
	"KEYPAD_0",			   // 0x62
	"KEYPAD_DECIMAL",	   // 0x63
	"EUROPE_2",			   // 0x64
	"APPLICATION",		   // 0x65
	"POWER",			   // 0x66
	"KEYPAD_EQUAL",		   // 0x67
	"F13",				   // 0x68
	"F14",				   // 0x69
	"F15",				   // 0x6A
	"F16",				   // 0x6B
	"F17",				   // 0x6C
	"F18",				   // 0x6D
	"F19",				   // 0x6E
	"F20",				   // 0x6F
	"F21",				   // 0x70
	"F22",				   // 0x71
	"F23",				   // 0x72
	"F24",				   // 0x73
	"EXECUTE",			   // 0x74
	"HELP",				   // 0x75
	"MENU",				   // 0x76
	"SELECT",			   // 0x77
	"STOP",				   // 0x78
	"AGAIN",			   // 0x79
	"UNDO",				   // 0x7A
	"CUT",				   // 0x7B
	"COPY",				   // 0x7C
	"PASTE",			   // 0x7D
	"FIND",				   // 0x7E
	"MUTE",				   // 0x7F
	"VOLUME_UP",		   // 0x80
	"VOLUME_DOWN",		   // 0x81
	"LOCKING_CAPS_LOCK",   // 0x82
	"LOCKING_NUM_LOCK",	   // 0x83
	"LOCKING_SCROLL_LOCK", // 0x84
	"KEYPAD_COMMA",		   // 0x85
	"KEYPAD_EQUAL_SIGN",   // 0x86
	"KANJI1",			   // 0x87
	"KANJI2",			   // 0x88
	"KANJI3",			   // 0x89
	"KANJI4",			   // 0x8A
	"KANJI5",			   // 0x8B
	"KANJI6",			   // 0x8C
	"KANJI7",			   // 0x8D
	"KANJI8",			   // 0x8E
	"KANJI9",			   // 0x8F
	"LANG1",			   // 0x90
	"LANG2",			   // 0x91
	"LANG3",			   // 0x92
	"LANG4",			   // 0x93
	"LANG5",			   // 0x94
	"LANG6",			   // 0x95
	"LANG7",			   // 0x96
	"LANG8",			   // 0x97
	"LANG9",			   // 0x98
	"ALTERNATE_ERASE",	   // 0x99
	"SYSREQ_ATTENTION",	   // 0x9A
	"CANCEL",			   // 0x9B
	"CLEAR",			   // 0x9C
	"PRIOR",			   // 0x9D
	"RETURN",			   // 0x9E
	"SEPARATOR",		   // 0x9F
	"OUT",				   // 0xA0
	"OPER",				   // 0xA1
	"CLEAR_AGAIN",		   // 0xA2
	"CRSEL_PROPS",		   // 0xA3
	"EXSEL",			   // 0xA4
};

// RESERVED				// 0xA5-DF

const char* const modifier[8] = {
	"LEFT_CONTROL",	 // 0xE0
	"LEFT_SHIFT",	 // 0xE1
	"LEFT_ALT",		 // 0xE2
	"LEFT_GUI",		 // 0xE3
	"RIGHT_CONTROL", // 0xE4
	"RIGHT_SHIFT",	 // 0xE5
	"RIGHT_ALT",	 // 0xE6
	"RIGHT_GUI",	 // 0xE7
};


cstr tostr(kipili::USB::HIDKey key)
{
	if (key <= 0xA4) return hidkey[key];
	if (key >= 0xE0 && key < 0xE8) return modifier[key - 0xE0];
	return "undefined";
}
