// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HidKeyTables.h"

// see https://deskthority.net/wiki/Scancode

namespace kio::USB
{

// clang-format off

static constexpr uchar us_alone[HidKeyTable::table_size] = 
{
	0,	  // 0x00	 no_event
	0,	  // 0x01	 rollover error
	0,	  // 0x02	 POST failed
	0,	  // 0x03	 other error
	'a',  // 0x04
	'b',  // 0x05
	'c',  // 0x06
	'd',  // 0x07
	'e',  // 0x08
	'f',  // 0x09
	'g',  // 0x0a
	'h',  // 0x0b
	'i',  // 0x0c
	'j',  // 0x0d
	'k',  // 0x0e
	'l',  // 0x0f
	'm',  // 0x10
	'n',  // 0x11
	'o',  // 0x12
	'p',  // 0x13
	'q',  // 0x14
	'r',  // 0x15
	's',  // 0x16
	't',  // 0x17
	'u',  // 0x18
	'v',  // 0x19
	'w',  // 0x1a
	'x',  // 0x1b
	'y',  // 0x1c
	'z',  // 0x1d
	'1',  // 0x1e
	'2',  // 0x1f
	'3',  // 0x20
	'4',  // 0x21
	'5',  // 0x22
	'6',  // 0x23
	'7',  // 0x24
	'8',  // 0x25
	'9',  // 0x26
	'0',  // 0x27
	13,	  // 0x28  return
	27,	  // 0x29  escape
	127,  // 0x2a  backspace
	9,	  // 0x2b  tab
	32,	  // 0x2c  space
	'-',  // 0x2d
	'=',  // 0x2e
	'[',  // 0x2f
	']',  // 0x30
	'\\', // 0x31
	0,	  // 0x32  key left of return key (not present on US keyboard)
	';',  // 0x33
	'\'', // 0x34
	'`',  // 0x35
	',',  // 0x36
	'.',  // 0x37
	'/',  // 0x38
	0,	  // 0x39  caps lock
	0,	  // 0x3a  F1
	0,	  // 0x3b  F2
	0,	  // 0x3c  F3
	0,	  // 0x3d  F4
	0,	  // 0x3e  F5
	0,	  // 0x3f  F6
	0,	  // 0x40  F7
	0,	  // 0x41  F8
	0,	  // 0x42  F9
	0,	  // 0x43  F10
	0,	  // 0x44  F11
	0,	  // 0x45  F12
	0,	  // 0x46  print screen
	0,	  // 0x47  scroll lock
	0,	  // 0x48  pause
	0,	  // 0x49  insert
	0,	  // 0x4a  home
	0,	  // 0x4b  page up
	0,	  // 0x4c  forward delete
	0,	  // 0x4d  end
	0,	  // 0x4e  page down
	0,	  // 0x4f  right
	0,	  // 0x50  left
	0,	  // 0x51  down
	0,	  // 0x52  up
	0,	  // 0x53  num lock
	'/',  // 0x54  keypad
	'*',  // 0x55  "
	'-',  // 0x56  "
	'+',  // 0x57  "
	13,	  // 0x58  "
	'1',  // 0x59  " 1 / end
	'2',  // 0x5a  " 2 / down
	'3',  // 0x5b  " 3 / pg up
	'4',  // 0x5c  " 4 / left
	'5',  // 0x5d  " 5
	'6',  // 0x5e  " 6 / right
	'7',  // 0x5f  " 7 / home
	'8',  // 0x60  " 8 / up
	'9',  // 0x61  " 9 / pg up
	'0',  // 0x62  " 0 / insert
	'.',  // 0x63  " . / delete
	0,	  // 0x64  key right of left shift (not present on US keyboard)
	0,	  // 0x65  menu
	0,	  // 0x66  power
	'=',  // 0x67  keypad
};

static constexpr uchar us_shift[HidKeyTable::table_size] = 
{
	0,	 // 0x00
	0,	 // 0x01
	0,	 // 0x02
	0,	 // 0x03
	'A', // 0x04
	'B', // 0x05
	'C', // 0x06
	'D', // 0x07
	'E', // 0x08
	'F', // 0x09
	'G', // 0x0a
	'H', // 0x0b
	'I', // 0x0c
	'J', // 0x0d
	'K', // 0x0e
	'L', // 0x0f
	'M', // 0x10
	'N', // 0x11
	'O', // 0x12
	'P', // 0x13
	'Q', // 0x14
	'R', // 0x15
	'S', // 0x16
	'T', // 0x17
	'U', // 0x18
	'V', // 0x19
	'W', // 0x1a
	'X', // 0x1b
	'Y', // 0x1c
	'Z', // 0x1d
	'!', // 0x1e
	'@', // 0x1f
	'#', // 0x20
	'$', // 0x21
	'%', // 0x22
	'^', // 0x23
	'&', // 0x24
	'*', // 0x25
	'(', // 0x26
	')', // 0x27
	13,	 // 0x28
	27,	 // 0x29
	127, // 0x2a  backspace
	9,	 // 0x2b
	32,	 // 0x2c
	'_', // 0x2d
	'+', // 0x2e
	'{', // 0x2f
	'}', // 0x30
	'|', // 0x31
	0,	 // 0x32  key left of return key (not present on US keyboard)
	':', // 0x33
	'"', // 0x34
	'~', // 0x35
	'<', // 0x36
	'>', // 0x37
	'?', // 0x38
	0,	 // 0x39  caps lock
	0,	 // 0x3a  F1
	0,	 // 0x3b  F2
	0,	 // 0x3c  F3
	0,	 // 0x3d  F4
	0,	 // 0x3e  F5
	0,	 // 0x3f  F6
	0,	 // 0x40  F7
	0,	 // 0x41  F8
	0,	 // 0x42  F9
	0,	 // 0x43  F10
	0,	 // 0x44  F11
	0,	 // 0x45  F12
	0,	 // 0x46  print screen
	0,	 // 0x47  scroll lock
	0,	 // 0x48  pause
	0,	 // 0x49  insert
	0,	 // 0x4a  home
	0,	 // 0x4b  page up
	0,	 // 0x4c  forward delete
	0,	 // 0x4d  end
	0,	 // 0x4e  page down
	0,	 // 0x4f  right
	0,	 // 0x50  left
	0,	 // 0x51  down
	0,	 // 0x52  up
	0,	 // 0x53  num lock
	'/', // 0x54  keypad
	'*', // 0x55  "
	'-', // 0x56  "
	'+', // 0x57  "
	13,	 // 0x58  "
	'1', // 0x59  " 1 / end
	'2', // 0x5a  " 2 / down
	'3', // 0x5b  " 3 / pg up
	'4', // 0x5c  " 4 / left
	'5', // 0x5d  " 5
	'6', // 0x5e  " 6 / right
	'7', // 0x5f  " 7 / home
	'8', // 0x60  " 8 / up
	'9', // 0x61  " 9 / pg up
	'0', // 0x62  " 0 / insert
	'.', // 0x63  " . / delete
	0,	 // 0x64  key right of left shift (not present on us keyboard)
	0,	 // 0x65  menu
	0,	 // 0x66  power
	'=', // 0x67  keypad
};

static constexpr uchar ger_solo[HidKeyTable::table_size] = 
{
	0,	  // 0x00
	0,	  // 0x01
	0,	  // 0x02
	0,	  // 0x03
	'a',  // 0x04
	'b',  // 0x05
	'c',  // 0x06
	'd',  // 0x07
	'e',  // 0x08
	'f',  // 0x09
	'g',  // 0x0a
	'h',  // 0x0b
	'i',  // 0x0c
	'j',  // 0x0d
	'k',  // 0x0e
	'l',  // 0x0f
	'm',  // 0x10
	'n',  // 0x11
	'o',  // 0x12
	'p',  // 0x13
	'q',  // 0x14
	'r',  // 0x15
	's',  // 0x16
	't',  // 0x17
	'u',  // 0x18
	'v',  // 0x19
	'w',  // 0x1a
	'x',  // 0x1b
	'z',  // 0x1c
	'y',  // 0x1d
	'1',  // 0x1e
	'2',  // 0x1f
	'3',  // 0x20
	'4',  // 0x21
	'5',  // 0x22
	'6',  // 0x23
	'7',  // 0x24
	'8',  // 0x25
	'9',  // 0x26
	'0',  // 0x27
	13,	  // 0x28
	27,	  // 0x29
	127,  // 0x2a  backspace
	9,	  // 0x2b
	32,	  // 0x2c
	223,  // 0x2d  "ß"
	'\'', // 0x2e
	252,  // 0x2f  "ü"
	'+',  // 0x30
	0,	  // 0x31	 not present on German keyboard
	'#',  // 0x32  key left of return key
	246,  // 0x33  "ö"
	228,  // 0x34  "ä"
	'^',  // 0x35
	',',  // 0x36
	'.',  // 0x37
	'-',  // 0x38
	0,	  // 0x39  caps lock
	0,	  // 0x3a  F1
	0,	  // 0x3b  F2
	0,	  // 0x3c  F3
	0,	  // 0x3d  F4
	0,	  // 0x3e  F5
	0,	  // 0x3f  F6
	0,	  // 0x40  F7
	0,	  // 0x41  F8
	0,	  // 0x42  F9
	0,	  // 0x43  F10
	0,	  // 0x44  F11
	0,	  // 0x45  F12
	0,	  // 0x46  print screen
	0,	  // 0x47  scroll lock
	0,	  // 0x48  pause
	0,	  // 0x49  insert
	0,	  // 0x4a  home
	0,	  // 0x4b  page up
	0,	  // 0x4c  forward delete
	0,	  // 0x4d  end
	0,	  // 0x4e  page down
	0,	  // 0x4f  right
	0,	  // 0x50  left
	0,	  // 0x51  down
	0,	  // 0x52  up
	0,	  // 0x53  num lock
	'/',  // 0x54  keypad
	'*',  // 0x55  "
	'-',  // 0x56  "
	'+',  // 0x57  "
	13,	  // 0x58  "
	'1',  // 0x59  " 1 / end
	'2',  // 0x5a  " 2 / down
	'3',  // 0x5b  " 3 / pg up
	'4',  // 0x5c  " 4 / left
	'5',  // 0x5d  " 5
	'6',  // 0x5e  " 6 / right
	'7',  // 0x5f  " 7 / home
	'8',  // 0x60  " 8 / up
	'9',  // 0x61  " 9 / pg up
	'0',  // 0x62  " 0 / insert
	'.',  // 0x63  " . / delete
	'<',  // 0x64  key right of left shift
	0,	  // 0x65  menu
	0,	  // 0x66  power
	'=',  // 0x67  keypad
};

static constexpr uchar ger_shift[HidKeyTable::table_size] = 
{
	0,	  // 0x00
	0,	  // 0x01
	0,	  // 0x02
	0,	  // 0x03
	'A',  // 0x04
	'B',  // 0x05
	'C',  // 0x06
	'D',  // 0x07
	'E',  // 0x08
	'F',  // 0x09
	'G',  // 0x0a
	'H',  // 0x0b
	'I',  // 0x0c
	'J',  // 0x0d
	'K',  // 0x0e
	'L',  // 0x0f
	'M',  // 0x10
	'N',  // 0x11
	'O',  // 0x12
	'P',  // 0x13
	'Q',  // 0x14
	'R',  // 0x15
	'S',  // 0x16
	'T',  // 0x17
	'U',  // 0x18
	'V',  // 0x19
	'W',  // 0x1a
	'X',  // 0x1b
	'Z',  // 0x1c
	'Y',  // 0x1d
	'!',  // 0x1e
	'"',  // 0x1f
	167,  // 0x20  "§"
	'$',  // 0x21
	'%',  // 0x22
	'&',  // 0x23
	'/',  // 0x24
	'(',  // 0x25
	')',  // 0x26
	'=',  // 0x27
	13,	  // 0x28
	27,	  // 0x29
	127,  // 0x2a  backspace
	9,	  // 0x2b
	32,	  // 0x2c
	'?',  // 0x2d
	'`',  // 0x2e
	220,  // 0x2f  "Ü"
	'*',  // 0x30
	0,	  // 0x31	 not present on German keyboard
	'\'', // 0x32  key left of return key
	214,  // 0x33  "Ö"
	196,  // 0x34  "Ä"
	176,  // 0x35  "°"
	';',  // 0x36
	':',  // 0x37
	'_',  // 0x38
	0,	  // 0x39  caps lock
	0,	  // 0x3a  F1
	0,	  // 0x3b  F2
	0,	  // 0x3c  F3
	0,	  // 0x3d  F4
	0,	  // 0x3e  F5
	0,	  // 0x3f  F6
	0,	  // 0x40  F7
	0,	  // 0x41  F8
	0,	  // 0x42  F9
	0,	  // 0x43  F10
	0,	  // 0x44  F11
	0,	  // 0x45  F12
	0,	  // 0x46  print screen
	0,	  // 0x47  scroll lock
	0,	  // 0x48  pause
	0,	  // 0x49  insert
	0,	  // 0x4a  home
	0,	  // 0x4b  page up
	0,	  // 0x4c  forward delete
	0,	  // 0x4d  end
	0,	  // 0x4e  page down
	0,	  // 0x4f  right
	0,	  // 0x50  left
	0,	  // 0x51  down
	0,	  // 0x52  up
	0,	  // 0x53  num lock
	'/',  // 0x54  keypad
	'*',  // 0x55  "
	'-',  // 0x56  "
	'+',  // 0x57  "
	13,	  // 0x58  "
	'1',  // 0x59  " 1 / end
	'2',  // 0x5a  " 2 / down
	'3',  // 0x5b  " 3 / pg up
	'4',  // 0x5c  " 4 / left
	'5',  // 0x5d  " 5
	'6',  // 0x5e  " 6 / right
	'7',  // 0x5f  " 7 / home
	'8',  // 0x60  " 8 / up
	'9',  // 0x61  " 9 / pg up
	'0',  // 0x62  " 0 / insert
	'.',  // 0x63  " . / delete
	'>',  // 0x64  key right of left shift
	0,	  // 0x65  menu
	0,	  // 0x66  power
	'=',  // 0x67  keypad
};

static constexpr uchar ger_alt[HidKeyTable::table_size] = 
{
	0,	  // 0x00
	0,	  // 0x01
	0,	  // 0x02
	0,	  // 0x03
	'a',  // 0x04
	'b',  // 0x05
	'c',  // 0x06
	'd',  // 0x07
	'e',  // 0x08
	'f',  // 0x09
	'g',  // 0x0a
	'h',  // 0x0b
	'i',  // 0x0c
	'j',  // 0x0d
	'k',  // 0x0e
	'l',  // 0x0f
	0xb5, // 0x10  "µ"  (M)  <<<<<
	'n',  // 0x11
	0xf8, // 0x12  "o"  <<<<<
	'p',  // 0x13
	'@',  // 0x14  "@"  (Q)  <<<<<
	'r',  // 0x15
	's',  // 0x16
	't',  // 0x17
	'u',  // 0x18
	'v',  // 0x19
	'w',  // 0x1a
	'x',  // 0x1b
	'z',  // 0x1c
	'y',  // 0x1d
	0xb9, // 0x1e  "¹"  (1)  <<<<<
	0xb2, // 0x1f  "²"  (2)  <<<<<
	0xb3, // 0x20  "³"  (3)  <<<<<
	'4',  // 0x21
	'5',  // 0x22
	'6',  // 0x23
	'{',  // 0x24  "7" <<<<<
	'[',  // 0x25  "8" <<<<<
	']',  // 0x26  "9" <<<<<
	'}',  // 0x27  "0" <<<<<
	13,	  // 0x28
	27,	  // 0x29
	127,  // 0x2a  backspace
	9,	  // 0x2b
	32,	  // 0x2c
	'\\', // 0x2d  "ß"  <<<<<  
	'\'', // 0x2e
	252,  // 0x2f  "ü"
	'~',  // 0x30  "+"  <<<<<
	0,	  // 0x31	 not present on German keyboard
	'#',  // 0x32  key left of return key
	246,  // 0x33  "ö"
	228,  // 0x34  "ä"
	'^',  // 0x35
	0xb7, // 0x36  "·"  (,)  <<<<<
	'.',  // 0x37  
	0xb1, // 0x38  "±"  (-)  <<<<<
	0,	  // 0x39  caps lock
	0,	  // 0x3a  F1
	0,	  // 0x3b  F2
	0,	  // 0x3c  F3
	0,	  // 0x3d  F4
	0,	  // 0x3e  F5
	0,	  // 0x3f  F6
	0,	  // 0x40  F7
	0,	  // 0x41  F8
	0,	  // 0x42  F9
	0,	  // 0x43  F10
	0,	  // 0x44  F11
	0,	  // 0x45  F12
	0,	  // 0x46  print screen
	0,	  // 0x47  scroll lock
	0,	  // 0x48  pause
	0,	  // 0x49  insert
	0,	  // 0x4a  home
	0,	  // 0x4b  page up
	0,	  // 0x4c  forward delete
	0,	  // 0x4d  end
	0,	  // 0x4e  page down
	0,	  // 0x4f  right
	0,	  // 0x50  left
	0,	  // 0x51  down
	0,	  // 0x52  up
	0,	  // 0x53  num lock
	'/',  // 0x54  keypad
	'*',  // 0x55  keypad
	'-',  // 0x56  keypad
	'+',  // 0x57  keypad
	13,	  // 0x58  keypad
	'1',  // 0x59  keypad 1 / end
	'2',  // 0x5a  keypad 2 / down
	'3',  // 0x5b  keypad 3 / pg up
	'4',  // 0x5c  keypad 4 / left
	'5',  // 0x5d  keypad 5
	'6',  // 0x5e  keypad 6 / right
	'7',  // 0x5f  keypad 7 / home
	'8',  // 0x60  keypad 8 / up
	'9',  // 0x61  keypad 9 / pg up
	'0',  // 0x62  keypad 0 / insert
	'.',  // 0x63  keypad . / delete
	'|',  // 0x64  key right of left shift"<"  <<<<<
	0,	  // 0x65  menu
	0,	  // 0x66  power
	'=',  // 0x67  keypad
};

constexpr HidKeyTable key_table_us = 
{
	.name			= "US",
	.solo		    = us_alone, 
	.with_shift     = us_shift,
	.with_alt	    = us_alone,
	.with_shift_alt	= us_shift
};
 
constexpr HidKeyTable key_table_ger = 
{
	.name			= "DE",
	.solo           = ger_solo,
	.with_shift     = ger_shift,
	.with_alt       = ger_alt,
	.with_shift_alt = ger_alt
};

} // namespace kio::USB





















