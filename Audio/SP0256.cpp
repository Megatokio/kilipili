// Copyright (c) 2014 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "SP0256.h"
#include "basic_math.h"
#include "common/cdefs.h"
#include <cstdio>
#include <cstring>
#include <math.h>


// issues:

// cmd 46 'WW' sounds like a bass bang
// cmd 51 'ER1' the 'r' is somehow not there
// cmd 39 'RR2' the 'r' is somehow sounds like 'n'


namespace kio::Audio
{

// --------------------------------------------------------------
//					tables and stuff:
// --------------------------------------------------------------


/* non-linear conversion table for coefficients 7 bit -> 9 bit
   from SP0250 data sheet
*/
static constexpr uint16 coeff_tab[128] = {			  //
	0,	 9,	  17,  25,	33,	 41,  49,  57,	65,	 73,  // +8		37x
	81,	 89,  97,  105, 113, 121, 129, 137, 145, 153, //
	161, 169, 177, 185, 193, 201, 209, 217, 225, 233, //
	241, 249, 257, 265, 273, 281, 289, 297, 301, 305, // +4		32x
	309, 313, 317, 321, 325, 329, 333, 337, 341, 345, //
	349, 353, 357, 361, 365, 369, 373, 377, 381, 385, //
	389, 393, 397, 401, 405, 409, 413, 417, 421, 425, //
	427, 429, 431, 433, 435, 437, 439, 441, 443, 445, // +2		28x
	447, 449, 451, 453, 455, 457, 459, 461, 463, 465, //
	467, 469, 471, 473, 475, 477, 479, 481, 482, 483, // +1		30x
	484, 485, 486, 487, 488, 489, 490, 491, 492, 493, //
	494, 495, 496, 497, 498, 499, 500, 501, 502, 503, //
	504, 505, 506, 507, 508, 509, 510, 511};


/* indexes in coefficient list c[]
 */
#define B0 c[0]
#define B1 c[2]
#define B2 c[4]
#define B3 c[6]
#define B4 c[8]
#define B5 c[10]
#define F0 c[1]
#define F1 c[3]
#define F2 c[5]
#define F3 c[7]
#define F4 c[9]
#define F5 c[11]


/*	Enumeration of SP0256 micro sequencer opcodes:
	opcode values are for not-bitswapped rom!
*/
enum {
	SETPAGE = 0b0000,
	SETMODE = 0b0001,
	LOAD_4	= 0b0010,
	LOAD_C	= 0b0011,
	LOAD_2	= 0b0100,
	SETMSBA = 0b0101,
	SETMSB6 = 0b0110,
	LOAD_E	= 0b0111,
	LOADALL = 0b1000,
	DELTA_9 = 0b1001,
	SETMSB5 = 0b1010,
	DELTA_D = 0b1011,
	SETMSB3 = 0b1100,
	JSR		= 0b1101,
	JMP		= 0b1110,
	PAUSE	= 0b1111
};

static constexpr char opcode_names[64][8] = {
	"SETPAGE", "SETMODE", "LOAD_4 ", "LOAD_C ", "LOAD_2 ", "SETMSBA", "SETMSB6", "LOAD_E ",
	"LOADALL", "DELTA_9", "SETMSB5", "DELTA_D", "SETMSB3", "JSR    ", "JMP    ", "PAUSE  ",
};


/*	Allophone names for the AL2 rom:
 */
static constexpr char al2_allophone_names[64][4] = {
	"PA1", //	10	  pause
	"PA2", //	30	  pause
	"PA3", //	50	  pause
	"PA4", //	100	  pause
	"PA5", //	200	  pause

	"OY",  //	420   oy
	"AY",  //	260   (ii)
	"EH",  //	70    e
	"KK3", //	120   c
	"PP",  //	210   p
	"JH",  //	140   j
	"NN1", //	140   n
	"IH",  //	70    i
	"TT2", //	140   (tt)
	"RR1", //	170   (rr)
	"AX",  //	70    u
	"MM",  //	180   m
	"TT1", //	100   t
	"DH1", //	290   dth
	"IY",  //	250   (ee)
	"EY",  //	280   (aa), (ay)
	"DD1", //	70    d
	"UW1", //	100   ou
	"AO",  //	100   o
	"AA",  //	100   a
	"YY2", //	180   (yy)
	"AE",  //	120   eh
	"HH1", //	130   h
	"BB1", //	80    b
	"TH",  //	180   th
	"UH",  //	100   uh
	"UW2", //	260   ouu
	"AW",  //	370   ow
	"DD2", //	160   (dd)
	"GG3", //	140   (ggg)
	"VV",  //	190   v
	"GG1", //	80    g
	"SH",  //	160   sh
	"ZH",  //	190   zh
	"RR2", //	120   r
	"FF",  //	150   f
	"KK2", //	190   ck?		was: ck? (gg)?
	"KK1", //	160   k
	"ZZ",  //	210   z
	"NG",  //	220   ng
	"LL",  //	110   l
	"WW",  //	180   w
	"XR",  //	360   aer
	"WH",  //	200   wh
	"YY1", //	130   y
	"CH",  //	190   ch
	"ER1", //	160   er
	"ER2", //	300   err
	"OW",  //	240   (oo), (eau)
	"DH2", //	240   ?
	"SS",  //	90	  s
	"NN2", //	190   (nn)
	"HH2", //	180   (hh)
	"OR",  //	330   or
	"AR",  //	290   ar
	"YR",  //	350   ear
	"GG2", //	40    (gg)?		was: ?
	"EL",  //	190   (ll)
	"BB2", //	50    (bb)
};

/*
inline uint32 X32(uint32 n)
{
	n = ((n & 0xFFFF0000) >> 16) | ((n & 0x0000FFFF) << 16);
	n = ((n & 0xFF00FF00) >>  8) | ((n & 0x00FF00FF) <<  8);
	n = ((n & 0xF0F0F0F0) >>  4) | ((n & 0x0F0F0F0F) <<  4);
	n = ((n & 0xCCCCCCCC) >>  2) | ((n & 0x33333333) <<  2);
	n = ((n & 0xAAAAAAAA) >>  1) | ((n & 0x55555555) <<  1);
	return n;
}
*/


// bit-swap byte:
inline constexpr uint X8(uint n)
{
	n = ((n & 0xF0u) >> 4) | ((n & 0x0Fu) << 4);
	n = ((n & 0xCCu) >> 2) | ((n & 0x33u) << 2);
	n = ((n & 0xAAu) >> 1) | ((n & 0x55u) << 1);
	return n;
}

// bit-swap nibble:
inline constexpr uint X4(uint n)
{
	n = ((n & 0xCu) >> 2) | ((n & 0x3u) << 2);
	n = ((n & 0xAu) >> 1) | ((n & 0x5u) << 1);
	return n;
}

struct UnswappedByte
{
	uint8 byte;
	constexpr UnswappedByte(uint8 byte) : byte(uint8(X8(byte))) {}
	constexpr operator uint() const noexcept { return byte; }
};

static_assert(UnswappedByte(0xC8) == 0x13);

static constexpr UnswappedByte al2_rom[] = {
	0xE0, 0x7B, 0xE0, 0x07, 0xE0, 0x47, 0xE0, 0x27, 0xE0, 0x67, 0xE0, 0x97, 0xE8, 0x28, 0xE8, 0xFC, //
	0xE8, 0x32, 0xE8, 0xFA, 0xE8, 0x4E, 0xE8, 0x89, 0xE8, 0xB5, 0xE8, 0x5D, 0xE8, 0x4B, 0xE8, 0xF7, //
	0xE8, 0x3F, 0xE4, 0x08, 0xE4, 0xC4, 0xE4, 0xDC, 0xE4, 0xEE, 0xE4, 0x59, 0xE4, 0xD5, 0xE4, 0xFD, //
	0xE4, 0x33, 0xE4, 0xFB, 0xEC, 0xA8, 0xEC, 0x44, 0xEC, 0xDC, 0xEC, 0xCA, 0xEC, 0xBA, 0xEC, 0x56, //
	0xEC, 0x91, 0xEC, 0xC5, 0xEC, 0x9D, 0xEC, 0xF3, 0xEC, 0x8F, 0xE2, 0xE0, 0xE2, 0xE4, 0xE2, 0xDC, //
	0xE2, 0x5A, 0xE2, 0x26, 0xE2, 0xAE, 0xE2, 0xF1, 0xE2, 0x75, 0xE2, 0x63, 0xE2, 0x5B, 0xE2, 0x3F, //
	0xEA, 0x8C, 0xEA, 0x1A, 0xEA, 0x3E, 0xEA, 0xF1, 0xEA, 0x7B, 0xE6, 0xAC, 0xE6, 0x0A, 0xE6, 0x16, //
	0xE6, 0x4E, 0xE6, 0x15, 0xE6, 0xBD, 0xE6, 0xA7, 0xEE, 0xDC, 0xEE, 0x06, 0xEE, 0x6E, 0xEE, 0x19, //
	0xE1, 0x00, 0xE1, 0x40, 0xE1, 0x20, 0xE1, 0x60, 0xE1, 0x10, 0xE1, 0x50, 0xE1, 0x30, 0xE1, 0x70, //
	0xE1, 0x08, 0xE1, 0x48, 0xE1, 0x28, 0xE1, 0x68, 0xE1, 0x18, 0xE1, 0x58, 0xE1, 0x38, 0xE1, 0x78, //
	0xE1, 0x04, 0xE1, 0x44, 0xE1, 0x24, 0xE1, 0x64, 0xE1, 0x14, 0xE1, 0x54, 0xE1, 0x34, 0xE1, 0x74, //
	0xE1, 0x0C, 0xE1, 0x4C, 0xE1, 0x2C, 0xE1, 0x6C, 0xE1, 0x1C, 0xE1, 0x5C, 0xE1, 0x3C, 0xE1, 0x7C, //
	0x08, 0x00, 0x04, 0x00, 0x0C, 0x00, 0x02, 0x00, 0x0A, 0x00, 0x06, 0x00, 0x0E, 0x00, 0x01, 0x00, //
	0x09, 0x00, 0x05, 0x00, 0x0D, 0x00, 0x03, 0x00, 0x0B, 0x00, 0x07, 0x00, 0x0F, 0x00, 0xF1, 0x00, //
	0xF4, 0x00, 0xF7, 0x00, 0xFF, 0x00, 0x1D, 0xFF, 0x00, 0x10, 0x33, 0xE5, 0x96, 0xA9, 0xAF, 0x3F, //
	0x43, 0xB0, 0x64, 0xCA, 0xA3, 0xF6, 0x47, 0x55, 0xB4, 0xFE, 0x29, 0x8E, 0xDA, 0x1F, 0x77, 0x6D, //
	0x51, 0x75, 0xF4, 0x7E, 0xA9, 0xB3, 0xE2, 0x4F, 0xD5, 0x56, 0xFD, 0xA5, 0xDA, 0xCA, 0x7F, 0x16, //
	0x49, 0xFB, 0x07, 0x00, 0x10, 0x31, 0xEE, 0xD6, 0xED, 0xB3, 0xBF, 0x1A, 0xA2, 0x27, 0xAA, 0xCD, //
	0xF6, 0xCB, 0xB9, 0x5B, 0x52, 0xAD, 0xCD, 0x5F, 0x8A, 0xCD, 0xFF, 0x4A, 0xB5, 0x56, 0xFF, 0xA9, //
	0xD7, 0x7E, 0x1E, 0xE5, 0x56, 0xFE, 0xA7, 0x5A, 0xDA, 0x81, 0x14, 0x49, 0x3D, 0x00, 0x00, 0x18, //
	0x36, 0xFB, 0x56, 0x41, 0x4B, 0x91, 0xF8, 0x2C, 0x9D, 0x4C, 0x15, 0x00, 0xF4, 0x18, 0x23, 0x0D, //
	0x00, 0x3A, 0x82, 0x1F, 0x6D, 0xB9, 0x84, 0x01, 0x18, 0x04, 0x84, 0x88, 0x15, 0x03, 0x00, 0xFD, //
	0x18, 0x24, 0x05, 0x00, 0x2A, 0x96, 0x7E, 0xE7, 0xD7, 0x84, 0x01, 0x50, 0x45, 0xE4, 0xEB, 0x3C, //
	0x03, 0x00, 0x18, 0x24, 0x15, 0x00, 0x29, 0x21, 0x03, 0x46, 0x9F, 0xE6, 0xDC, 0xF2, 0xA8, 0xD1, //
	0x11, 0x00, 0x06, 0xE0, 0x98, 0xD3, 0x94, 0x5B, 0x1D, 0x2C, 0x43, 0xFD, 0xA7, 0x74, 0x8B, 0x6E, //
	0x00, 0x18, 0x37, 0xCF, 0xD6, 0x80, 0x06, 0x0F, 0xFF, 0x15, 0x9C, 0x2A, 0x74, 0xD2, 0x00, 0x92, //
	0xB4, 0x81, 0x14, 0x1E, 0x32, 0x03, 0x00, 0x01, 0x00, 0x20, 0x5F, 0x19, 0x00, 0x18, 0x35, 0xFB, //
	0x56, 0x81, 0x44, 0x0D, 0xEB, 0x8F, 0xC6, 0x0D, 0x14, 0x00, 0xF5, 0x18, 0x23, 0x1D, 0x40, 0x35, //
	0xA7, 0x23, 0x84, 0x9E, 0xA4, 0x02, 0x20, 0x46, 0x74, 0x4C, 0xA9, 0xCF, 0x2E, 0x78, 0x07, 0x9C, //
	0x0E, 0x00, 0x18, 0x35, 0xC7, 0x96, 0xA7, 0x71, 0x39, 0x0E, 0x1E, 0x64, 0x45, 0x66, 0xAA, 0x9A, //
	0xB8, 0xC7, 0x79, 0x5B, 0x52, 0x2D, 0xC5, 0x3E, 0xEA, 0xA4, 0xD7, 0xCB, 0xB1, 0x5B, 0x00, 0x18, //
	0x36, 0xFB, 0x56, 0xBD, 0x86, 0x0B, 0xD3, 0x0C, 0x25, 0x0C, 0x15, 0x00, 0x10, 0x36, 0xDD, 0x56, //
	0xFD, 0xB0, 0xB8, 0x00, 0x22, 0xA4, 0xCE, 0xDB, 0xAA, 0xFA, 0x3C, 0xCF, 0x74, 0xE5, 0x16, 0x00, //
	0xF6, 0x18, 0x21, 0x14, 0x40, 0x42, 0x20, 0xE2, 0xE7, 0xBB, 0xA4, 0x01, 0x98, 0x04, 0xA4, 0xFC, //
	0xA0, 0x03, 0x00, 0x18, 0x3C, 0xDD, 0xD6, 0xC2, 0x06, 0x8F, 0xED, 0x97, 0x1A, 0x64, 0x79, 0xA6, //
	0xDD, 0x32, 0xE8, 0xE8, 0x89, 0xFE, 0x75, 0x73, 0x85, 0x02, 0x00, 0x18, 0x33, 0xFB, 0x16, 0x02, //
	0x0B, 0x0F, 0x07, 0x33, 0x5E, 0x2B, 0x74, 0x66, 0xDF, 0x62, 0xE1, 0x41, 0xD0, 0x20, 0xD6, 0x5F, //
	0x91, 0xCA, 0xEC, 0x5B, 0x09, 0x2D, 0x3C, 0x19, 0xC8, 0x7A, 0x31, 0xCD, 0xC9, 0x05, 0x01, 0x00, //
	0x18, 0x3C, 0x08, 0xA7, 0x74, 0x10, 0x00, 0x41, 0x3D, 0x00, 0x71, 0x9A, 0x72, 0x8B, 0x81, 0x85, //
	0x87, 0x7D, 0x43, 0x1F, 0x07, 0x09, 0x00, 0x10, 0x33, 0xEE, 0xD6, 0xA9, 0xBB, 0x80, 0x05, 0x29, //
	0x25, 0xCA, 0xA5, 0x1E, 0xCC, 0xB5, 0x5B, 0x51, 0xE7, 0xFC, 0xBF, 0xEA, 0x9C, 0x1F, 0xC4, 0x9D, //
	0x5B, 0x56, 0x9D, 0xFC, 0x40, 0xEA, 0x92, 0xFF, 0x03, 0x00, 0x18, 0x33, 0xED, 0xD6, 0xE5, 0xB9, //
	0x81, 0x10, 0xAB, 0x23, 0x49, 0x47, 0x8A, 0x9D, 0x1C, 0x00, 0x00, 0x18, 0x33, 0xF5, 0x96, 0xA7, //
	0xBD, 0xF7, 0x1E, 0xA7, 0x84, 0x25, 0x47, 0xAA, 0xD6, 0x9E, 0x4A, 0xD1, 0x3E, 0x53, 0x00, 0x18, //
	0x38, 0xF4, 0x56, 0x89, 0xC6, 0x10, 0xFB, 0x30, 0x58, 0x4B, 0x16, 0x00, 0x18, 0x33, 0xF5, 0x96, //
	0xB3, 0xAF, 0x7F, 0x15, 0x9B, 0x23, 0x88, 0x48, 0xAE, 0xDE, 0x92, 0xAA, 0x6F, 0xFE, 0x00, 0x18, //
	0x33, 0xE7, 0x56, 0x05, 0xCB, 0x8C, 0x09, 0x32, 0x1E, 0xCE, 0x51, 0xF2, 0x01, 0x10, 0x20, 0xFF, //
	0x0E, 0xE3, 0x29, 0x0F, 0xF8, 0xC7, 0xBF, 0x78, 0xD0, 0x24, 0xF2, 0x00, 0x92, 0x2B, 0xF7, 0xFF, //
	0x5C, 0x66, 0xEE, 0x2D, 0x12, 0x96, 0x8C, 0x04, 0x60, 0x7C, 0x1A, 0x66, 0x24, 0x81, 0x1F, 0x40, //
	0x00, 0x0F, 0x9F, 0x00, 0x00, 0x18, 0x39, 0xEE, 0x16, 0x7F, 0x49, 0x0D, 0xF1, 0xA6, 0xDB, 0xCC, //
	0x15, 0x00, 0x18, 0x26, 0x07, 0x40, 0x25, 0x27, 0x81, 0x61, 0xDD, 0x84, 0x02, 0xB8, 0xE6, 0x33, //
	0x68, 0xC4, 0x8B, 0x14, 0x00, 0x86, 0xE4, 0xF5, 0x9F, 0x01, 0x00, 0x18, 0x33, 0xC1, 0xD6, 0x3E, //
	0xC7, 0x10, 0xE5, 0x02, 0xC3, 0x0E, 0x31, 0xC6, 0xDD, 0x2A, 0xC9, 0xA0, 0x79, 0x5F, 0x87, 0xB3, //
	0x61, 0x02, 0x00, 0x19, 0x24, 0x0D, 0x80, 0x31, 0x12, 0x62, 0xA7, 0x1C, 0x00, 0x18, 0x38, 0xED, //
	0xD6, 0x7F, 0x49, 0x4B, 0xC3, 0x03, 0xC3, 0x8B, 0x14, 0x00, 0x18, 0x38, 0xED, 0x96, 0xBD, 0x07, //
	0x09, 0xDB, 0x06, 0x24, 0xAC, 0x93, 0xC6, 0xDD, 0xEA, 0x28, 0xD9, 0x61, 0x7E, 0x46, 0x4F, 0x99, //
	0x5E, 0x3A, 0x08, 0x90, 0x04, 0xE0, 0xEE, 0x2E, 0x00, 0x10, 0x38, 0xE7, 0x96, 0xAF, 0x75, 0x3F, //
	0x0D, 0x22, 0xA4, 0x8A, 0xB4, 0xF9, 0x53, 0x75, 0x16, 0x7F, 0x2A, 0xAE, 0x62, 0x70, 0xD5, 0xD0, //
	0x0B, 0x00, 0x00, 0xF4, 0x18, 0x23, 0x0F, 0x00, 0x29, 0x99, 0x62, 0xE4, 0x7C, 0xC6, 0xDE, 0xEA, //
	0x28, 0x19, 0x62, 0x3F, 0x97, 0x77, 0x75, 0x02, 0x00, 0xF8, 0x18, 0x25, 0x0F, 0x40, 0x32, 0xA1, //
	0x5E, 0x45, 0x7D, 0xA6, 0xDC, 0x1A, 0xA9, 0xC9, 0x68, 0x9F, 0xA5, 0xA3, 0x71, 0x02, 0x00, 0x18, //
	0x36, 0xCC, 0xD6, 0x42, 0x0B, 0x55, 0xF2, 0x34, 0xF9, 0x08, 0xD5, 0xE6, 0xDB, 0xA2, 0x60, 0xA9, //
	0x42, 0xBE, 0x41, 0xEB, 0x78, 0xCB, 0x94, 0x5B, 0xF6, 0x1C, 0x24, 0x4D, 0x33, 0x96, 0x92, 0x63, //
	0x00, 0xF4, 0x18, 0x23, 0x0C, 0x80, 0x15, 0xF8, 0x3F, 0x68, 0x7F, 0xE6, 0xDD, 0xA2, 0x30, 0xD9, //
	0x31, 0xFF, 0xD5, 0x73, 0x85, 0x02, 0x00, 0x18, 0x26, 0x04, 0x80, 0x1E, 0x87, 0x81, 0x6B, 0x7F, //
	0xC4, 0x02, 0x98, 0x24, 0x64, 0x58, 0xE9, 0x67, 0xC8, 0x16, 0xC0, 0x13, 0x14, 0x41, 0x52, 0x01, //
	0x4C, 0x72, 0x21, 0x98, 0xF0, 0x01, 0x00, 0x10, 0x37, 0xE5, 0xD6, 0x30, 0xB9, 0xFF, 0x16, 0xA4, //
	0x04, 0x63, 0x85, 0x03, 0x00, 0x20, 0x84, 0xFC, 0xF8, 0x03, 0x00, 0x18, 0x32, 0xCF, 0x16, 0xC3, //
	0xC8, 0x4E, 0xDE, 0xAC, 0x97, 0x8A, 0x74, 0xC6, 0xDA, 0xB2, 0xBE, 0x18, 0xE1, 0x97, 0x70, 0x74, //
	0x85, 0x52, 0x1E, 0x38, 0x0E, 0x1D, 0x30, 0xF1, 0x05, 0x00, 0x19, 0x21, 0x0F, 0xC0, 0x29, 0x94, //
	0xE0, 0x64, 0x1C, 0x00, 0x1D, 0xF2, 0x18, 0x21, 0x0F, 0x80, 0x35, 0x89, 0xC0, 0xCA, 0x5B, 0xB6, //
	0xC0, 0xDD, 0x78, 0x7A, 0x00, 0xF7, 0x18, 0x21, 0x1D, 0xC0, 0x31, 0xB1, 0xE1, 0x46, 0x3C, 0xE4, //
	0x00, 0xA0, 0x65, 0x43, 0x10, 0xE5, 0xA7, 0x54, 0x00, 0xB5, 0x88, 0x86, 0x8D, 0x73, 0x00, 0x18, //
	0x36, 0xF4, 0x56, 0x89, 0x51, 0xD7, 0x02, 0xAA, 0xBB, 0xE9, 0x34, 0xC5, 0x02, 0xD8, 0x08, 0xB0, //
	0xC3, 0x84, 0xD0, 0xD4, 0x5B, 0x25, 0x45, 0x4C, 0x08, 0xC0, 0xEE, 0x9B, 0x5C, 0x00, 0x18, 0x35, //
	0xEF, 0x16, 0x37, 0x2F, 0xFF, 0x06, 0x9E, 0x45, 0xAB, 0x6A, 0x8A, 0xF5, 0x76, 0x5B, 0x9D, 0xD6, //
	0x6F, 0xAC, 0xD2, 0x5B, 0x0D, 0x00, 0x18, 0x33, 0xDE, 0x96, 0xA7, 0x33, 0x83, 0x19, 0x22, 0xA5, //
	0xC4, 0x65, 0xAA, 0xA5, 0x39, 0x4C, 0xB1, 0x14, 0x03, 0x00, 0x18, 0x35, 0xCD, 0x96, 0x3E, 0xC7, //
	0xCA, 0xC1, 0x7C, 0x42, 0x0B, 0xB4, 0xE6, 0xD9, 0x5A, 0x30, 0xE1, 0x89, 0x1F, 0xC5, 0x7E, 0x79, //
	0xDA, 0x54, 0x5B, 0x05, 0x1E, 0x25, 0xAE, 0x06, 0x0A, 0xB9, 0x49, 0x00, 0x18, 0x33, 0xED, 0x96, //
	0xA9, 0xBB, 0xBF, 0x00, 0xA9, 0x23, 0x4B, 0x48, 0xAE, 0xDD, 0x92, 0x3A, 0x69, 0xF7, 0x52, 0x2D, //
	0xE5, 0x5E, 0xCA, 0xAD, 0xDC, 0x4B, 0xB5, 0x75, 0xF7, 0x39, 0x76, 0x2B, 0xEE, 0xDC, 0x9A, 0xFA, //
	0xAA, 0xE6, 0x51, 0x1C, 0xD5, 0x5C, 0xCA, 0xAB, 0xBA, 0xC7, 0x5D, 0x5B, 0x53, 0x55, 0xDD, 0x1D, //
	0x00, 0x18, 0x26, 0x03, 0x00, 0x21, 0x8E, 0x1F, 0x45, 0x7A, 0x65, 0x00, 0xB8, 0x84, 0x11, 0x54, //
	0xD1, 0xCA, 0x78, 0x5B, 0xFC, 0x37, 0x22, 0x17, 0xFB, 0x89, 0xAE, 0x51, 0x99, 0x72, 0xCB, 0xBF, //
	0xA5, 0xA5, 0xD9, 0xC1, 0x81, 0x35, 0x0A, 0x00, 0x18, 0x33, 0xE7, 0x56, 0x05, 0xCB, 0x8C, 0x09, //
	0x32, 0x1E, 0xCE, 0x51, 0xF2, 0x01, 0x10, 0x20, 0xFF, 0x0E, 0xE3, 0x29, 0x0F, 0xF8, 0xC7, 0xBF, //
	0x78, 0xD0, 0x24, 0xF2, 0x00, 0x92, 0x2B, 0xF7, 0xFF, 0x5C, 0x00, 0x00, 0xF5, 0x18, 0x25, 0x05, //
	0x00, 0x2A, 0x27, 0x21, 0x83, 0xBC, 0xA5, 0x02, 0xD0, 0x66, 0x46, 0x24, 0xD9, 0x03, 0x00, 0x18, //
	0x31, 0xED, 0x16, 0x07, 0x89, 0x0C, 0xE7, 0xB4, 0xF9, 0xAB, 0x54, 0x12, 0x00, 0x00, 0xFC, 0x07, //
	0x0E, 0x00, 0x62, 0xDA, 0x2D, 0x73, 0x0E, 0x12, 0xA6, 0x13, 0xC9, 0x16, 0x6A, 0xE4, 0x03, 0x20, //
	0x40, 0x00, 0xFC, 0x68, 0x47, 0x2E, 0x00, 0x02, 0x20, 0x00, 0xE0, 0x85, 0x64, 0x01, 0xE0, 0x00, //
	0x0E, 0x14, 0xA8, 0x47, 0x26, 0x00, 0x02, 0x20, 0xC1, 0x5E, 0xBC, 0xCC, 0xB7, 0xE5, 0x21, 0x12, //
	0xC2, 0x38, 0xAD, 0x76, 0x03, 0x8D, 0x2C, 0x00, 0xE0, 0x11, 0x22, 0xA0, 0x0E, 0x00, 0x18, 0x32, //
	0xED, 0x16, 0x07, 0x89, 0x0C, 0xE7, 0xB4, 0xF9, 0xAB, 0x54, 0x12, 0x00, 0x00, 0xFC, 0x07, 0x0E, //
	0x00, 0x64, 0xD8, 0x2D, 0x73, 0x0E, 0x16, 0xA6, 0x11, 0x49, 0x56, 0xAA, 0x24, 0x00, 0x00, 0xC0, //
	0x01, 0x01, 0xE2, 0x4B, 0x2E, 0x00, 0x02, 0x04, 0xC0, 0x8F, 0xB6, 0x24, 0x00, 0x20, 0x00, 0x02, //
	0x00, 0x5E, 0x48, 0x02, 0x00, 0x0E, 0xE0, 0x40, 0x81, 0xFA, 0xA4, 0x03, 0x20, 0x00, 0x12, 0xEC, //
	0xC5, 0xCF, 0x78, 0x5B, 0x1E, 0x22, 0x21, 0x8C, 0xD3, 0x6A, 0x37, 0xD0, 0xC9, 0x00, 0x00, 0x1E, //
	0x21, 0x02, 0xEA, 0x00, 0x00, 0x10, 0x33, 0xED, 0x96, 0xAB, 0xB1, 0x3F, 0x43, 0xB0, 0x64, 0x8A, //
	0xAD, 0x18, 0xC4, 0x9D, 0x5B, 0x55, 0x1E, 0xBD, 0x20, 0xCE, 0xDB, 0xB2, 0xBB, 0xB6, 0x00, 0x00, //
	0x19, 0x31, 0xDD, 0xD6, 0xC2, 0x06, 0x8F, 0xED, 0x97, 0x1A, 0x64, 0x79, 0xA6, 0xDD, 0x32, 0xE8, //
	0xE8, 0x89, 0xFE, 0x75, 0x73, 0x85, 0x02, 0x00, 0x18, 0x2A, 0x17, 0x00, 0x4A, 0x1C, 0x24, 0x65, //
	0x1C, 0x00, 0x18, 0x34, 0xDD, 0xD6, 0x80, 0x06, 0x0F, 0xFF, 0x15, 0x9C, 0x2A, 0x54, 0x32, 0x00, //
	0x92, 0xB4, 0x81, 0x14, 0x1E, 0x24, 0x0B, 0x00, 0x01, 0x00, 0x20, 0x5F, 0x79, 0x66, 0xDF, 0xA2, //
	0xE0, 0x98, 0x69, 0x3F, 0x76, 0x43, 0xA1, 0x46, 0x02, 0x30, 0x80, 0x00, 0x60, 0x5E, 0xC8, 0x24, //
	0x80, 0xA9, 0x00, 0x02, 0xFE, 0x03, 0x00, 0x00, 0x18, 0x2E, 0x03, 0x80, 0x21, 0x0F, 0x00, 0x29, //
	0x98, 0xC6, 0xDB, 0x22, 0x30, 0xE9, 0x19, 0xBD, 0x80, 0x27, 0x09, 0x03, 0x00, 0x18, 0x32, 0xD6, //
	0x96, 0xA9, 0xAB, 0x3F, 0x11, 0x30, 0xE4, 0xEA, 0x26, 0x8A, 0xA2, 0xD5, 0xCF, 0x55, 0x5B, 0x53, //
	0x1F, 0xB5, 0xBE, 0xAA, 0xA4, 0xB8, 0x53, 0x9D, 0x35, 0x73, 0xAA, 0xB3, 0x6A, 0x1E, 0xF5, 0x51, //
	0xCD, 0xE7, 0xAC, 0x2D, 0x00, 0x18, 0x32, 0xE4, 0x96, 0x07, 0x0B, 0x44, 0xF2, 0xB2, 0x37, 0x8B, //
	0x55, 0x32, 0x00, 0x8E, 0x27, 0x80, 0x87, 0x3E, 0x24, 0x03, 0x1C, 0x07, 0x82, 0xF7, 0x00, 0x41, //
	0xA6, 0xDC, 0xAA, 0xB7, 0x28, 0x61, 0x38, 0x40, 0x44, 0xD9, 0x4A, 0x02, 0xC8, 0x11, 0x04, 0xF1, //
	0x9D, 0xC5, 0x4C, 0xB9, 0xE5, 0x01, 0x31, 0xB2, 0x7A, 0x4C, 0xA6, 0xAA, 0x9D, 0xF9, 0xB6, 0x00, //
	0x3C, 0x6A, 0x38, 0x67, 0xCD, 0x56, 0xB5, 0x93, 0x0F, 0x00, 0xE0, 0x07, 0x48, 0x8C, 0x3E, 0xC3, //
	0x6D, 0xB9, 0x5B, 0x88, 0xF0, 0xEC, 0x0F, 0xAA, 0x64, 0x01, 0x00, 0x18, 0x33, 0xEF, 0xD6, 0x65, //
	0xFD, 0xC0, 0x04, 0x27, 0x25, 0x28, 0x47, 0x8A, 0x96, 0x1F, 0xCC, 0xD5, 0x5B, 0x52, 0xAD, 0xF4, //
	0x60, 0xAA, 0x9D, 0xFD, 0x53, 0x99, 0x94, 0xFB, 0x2A, 0x93, 0x6E, 0x5F, 0x57, 0x6E, 0x01, 0x00, //
	0xF4, 0x18, 0x24, 0x07, 0xC0, 0x21, 0x20, 0x9C, 0xC3, 0x5C, 0xC6, 0xDC, 0xE2, 0xF7, 0xE8, 0x19, //
	0xDF, 0x65, 0xC3, 0x85, 0x02, 0x00, 0x18, 0x35, 0xEE, 0x16, 0x47, 0x8B, 0x48, 0xFF, 0xB0, 0x98, //
	0x6C, 0x74, 0xC6, 0xDC, 0x8A, 0x70, 0xE1, 0xE1, 0x3F, 0x06, 0x83, 0x8D, 0xDE, 0x90, 0x5B, 0x2E, //
	0x19, 0x33, 0x0C, 0xC0, 0x60, 0x34, 0x50, 0x00, 0xF4, 0x18, 0x21, 0x06, 0x80, 0x21, 0x92, 0x7F, //
	0xC8, 0x5B, 0xA6, 0xDE, 0x2A, 0xA1, 0xC9, 0x68, 0xFF, 0x86, 0xB3, 0x65, 0x02, 0x00, 0x00, 0x00, //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, //
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


// --------------------------------------------------------------
//						public methods
// --------------------------------------------------------------

template<uint nc>
SP0256<nc>::SP0256(float frequency, float volume) noexcept : //
	volume(volume),
	hw_frequency(hw_sample_frequency)
{
	setClock(frequency);
	//setVolume(volume);
	reset();
}

template<uint nc>
void SP0256<nc>::setVolume(float v) noexcept
{
	if (v < -1) v = -1;
	if (v > +1) v = +1;
	volume = v;

	ccx_per_sample = int(frequency * (1 << ccx_fract_bits) / hw_frequency + 0.5f);

	v = v * (1 << 20);			   // wg. scale filter output 12 bit -> int32
	v = v / float(ccx_per_sample); // wg. accumulation in resampling

	amplification = int(v);

	// TODO:
	// for 3.12MHz and 44100Hz the amplification is in range 0 .. 57.
	// this is pretty coarse.
	// this could be improved by either
	// - reduce ccx_fract_bits: cc_per_sample is high anyway: 18111 for 3.12MHz&44.1kHz
	// - reduce filter output:  output on real chip had only 8 significant bits anyway
}

template<uint nc>
void SP0256<nc>::setClock(float f) noexcept
{
	frequency = f;
	setVolume(volume);
}

template<uint nc>
void SP0256<nc>::setSampleRate(float hw_sample_frequency) noexcept
{
	hw_frequency = hw_sample_frequency;
	setVolume(volume);
}

template<uint nc>
void SP0256<nc>::reset() noexcept
{
	sm_state	  = 0;
	stand_by	  = true;
	command_valid = false;
}

template<uint nc>
void SP0256<nc>::reset(CC cc) noexcept
{
	run_up_to_cycle(cc << ccx_fract_bits);
	reset();
}

template<uint nc>
bool SP0256<nc>::isSpeaking(CC cc) noexcept
{
	// test whether the SP0256 is speaking (not stand_by)
	// note: the Currah µSpeech couldn't poll this

	run_up_to_cycle(cc << ccx_fract_bits);
	return !stand_by;
}

template<uint nc>
bool SP0256<nc>::acceptsNextCommand(CC cc) noexcept
{
	// test whether the SP0256 accepts a new command
	// as long as stand_by is not active, the SP0256 is still speaking the previous command

	run_up_to_cycle(cc << ccx_fract_bits);
	return !command_valid;
}

template<uint nc>
void SP0256<nc>::writeCommand(uint cmd) noexcept
{
	// write a command into the SP0256 command register
	// only commands 0 .. 63 are valid for the AL2 rom!

	command		  = cmd;
	command_valid = true;
	stand_by	  = false;
}

template<uint nc>
void SP0256<nc>::writeCommand(CC cc, uint cmd) noexcept
{
	// write a command into the SP0256 command register
	// only commands 0 .. 63 are valid for the AL2 rom!

	run_up_to_cycle(cc << ccx_fract_bits);
	command		  = cmd;
	command_valid = true;
	stand_by	  = false;
}

template<uint nc>
auto SP0256<nc>::audioBufferStart(AudioSample<nc>* buffer, uint num_samples) noexcept -> CC
{
	output_buffer = buffer;
	ccx_buffer_end += int(num_samples) * ccx_per_sample;
	return CC(int(ccx_buffer_end >> ccx_fract_bits));
}

template<uint nc>
void SP0256<nc>::audioBufferEnd() noexcept
{
	run_up_to_cycle(CCx(int(ccx_buffer_end)));
	output_buffer = nullptr;
}

template<uint nc>
void SP0256<nc>::shift_timebase(int delta_ccx) noexcept
{
	// subtract delta_ccx from the clock cycle counter

	ccx_next -= delta_ccx;
	ccx_now -= delta_ccx;
	ccx_at_sos -= delta_ccx;
	ccx_buffer_end -= delta_ccx;
}

template<uint nc>
void SP0256<nc>::shiftTimebase(int delta_cc) noexcept
{
	shift_timebase(delta_cc << ccx_fract_bits); //
}

template<uint nc>
void SP0256<nc>::resetTimebase() noexcept
{
	shift_timebase(ccx_now.value); //
}


// --------------------------------------------------------------
//			run the micro sequencer as a co-routine:
// --------------------------------------------------------------

template<uint nc>
void SP0256<nc>::run_up_to_cycle(CCx ccx_end) noexcept
{
	if (ccx_end > CCx(int(ccx_buffer_end))) ccx_end = CCx(int(ccx_buffer_end));
	assert(ccx_at_sos <= ccx_now); // start of hw sample <= now
	assert(ccx_now <= ccx_next);   // now <= time for next SP0256 sample

	while (ccx_now < ccx_end)
	{
		CCx ccx_when = min(ccx_end, ccx_next);

		if (ccx_when < ccx_at_sos + ccx_per_sample)
		{
			current_sample += current_value * (ccx_when - ccx_now);
			ccx_now = ccx_when;
		}
		else
		{
			ccx_at_sos += ccx_per_sample;
			*output_buffer++ = (current_sample + current_value * (ccx_at_sos - ccx_now)) >> 16;

			while (ccx_at_sos + ccx_per_sample <= ccx_when)
			{
				ccx_at_sos += ccx_per_sample;
				*output_buffer++ = (current_value * ccx_per_sample) >> 16;
			}

			current_sample = current_value * (ccx_when - ccx_at_sos);
			ccx_now		   = ccx_when;
		}

		if (ccx_now == ccx_next)
		{
			current_value = next_sample();
			// TODO: which are the maximum possible values at the filter output?
			limit(-2047, current_value, 2047);
			// original chip output was 8 bit resolution
			//current_value &= ~0xF;
			current_value *= amplification;
			ccx_next += 312 << ccx_fract_bits; // each SP0256 sample takes 312 clock cycles
		}
	}
}

// expand compressed arguments:
inline int ampl(int a) { return (a & 0x1F) << ((a >> 5) & 0x07); }
#if 0
inline int coeff(uint c) { return c&0x80 ? coeff_tab[(~c)&0x7F] : -coeff_tab[c]; } // i can't hear a difference
#else
inline int coeff(uint c) { return c & 0x80 ? coeff_tab[(-c) & 0x7F] : -coeff_tab[c]; } // MAME
#endif

//	Tricky co-routine macros:
#define BEGIN       \
  switch (sm_state) \
  {                 \
  default: IERR();  \
  case 0:
#define YIELD(x)         \
  do {                   \
	sm_state = __LINE__; \
	return x;            \
  case __LINE__:;        \
  }                      \
  while (0)
#define FINISH }

template<uint nc>
int SP0256<nc>::next_sample() noexcept
{
	int z0;

	BEGIN; // co-routine

	// init micro sequencer:
	mode		  = 0;
	page		  = 0x1000;
	repeat		  = 0;
	stack		  = 0;
	stand_by	  = true;  // 1 = stand by (utterance completed)
	command_valid = false; // 1 = command valid == !LRQ (load request)

	pitch		   = 0;
	amplitude	   = 0;
	pitch_incr	   = 0;
	amplitude_incr = 0;
	memset(c, 0, sizeof(c));
	memset(_c, 0, sizeof(_c));
	memset(_z, 0, sizeof(_z));

	byte	  = 0; // current/last byte read from rom, remaining valid bits are right-aligned
	bits	  = 0; // number of remaining valid bits
	_shiftreg = 1; // noise


	// run until power off:
	for (;;)
	{
		if (stand_by && command_valid)
		{
			pitch		   = 0;
			amplitude	   = 0;
			pitch_incr	   = 0;
			amplitude_incr = 0;
			memset(c, 0, sizeof(c));

			assert(bits == 0);
			assert(byte == 0);

			debugstr(
				"SP0256: SBY: next command = %u = %s\n", command, command < 64 ? al2_allophone_names[command] : "???");
			pc			  = 0x1000 + (command << 1);
			command_valid = false;
			stand_by	  = false;
		}

		if (stand_by) { repeat = 1; } // TODO: YIELD(0) ?!?
		else						  // load next microcode
		{
			// set/update repeat, pitch, pitch_incr, amplitude, amplitude_incr and
			// filter coefficients c[] & _c[] and clears feed-back values _z[]
			//
			// the sequencer receives a serial bit stream.
			// for the opcode we need 8 bits:
			// the low (1st) nibble contains inline data, the high (2nd) nibble is the instruction
			uint instr = next8();

			switch (instr >> 4)
			{
			default: IERR();
			case SETPAGE: cmdSetPage(instr); continue; // and RTS
			case SETMODE: cmdSetMode(instr); continue;
			case JMP: cmdJmp(instr); continue;
			case JSR: cmdJsr(instr); continue;
			case PAUSE: cmdPause(); break;
			case LOADALL: cmdLoadAll(); break;
			case LOAD_2: cmdLoad2(); break;
			case LOAD_4: cmdLoad4(); break;
			case LOAD_C: cmdLoadC(); break;
			case LOAD_E: cmdLoadE(); break;
			case SETMSB3: cmdSetMsb3(); break;
			case SETMSB5: cmdSetMsb5(); break;
			case SETMSB6: cmdSetMsb6(); break;
			case SETMSBA: cmdSetMsbA(); break;
			case DELTA_9: cmdDelta9(); break;
			case DELTA_D: cmdDeltaD(); break;
			}

			// mockup parameters:
			repeat += (instr & 15);
			assert(repeat < 0x40); // repeat &= 0x3F;		6 bit
			// assert(pitch<0x100);  	// pitch &= 0xff;		8 bit

			// repeat==0 is an ill. condition
			// it does never happen in the AL2 rom
			// it is unclear what the real device did in this case
			if (repeat == 0)
			{
				debugstr("SP0256: repeat=0\n");
				continue;
			}

			// convert coefficients: 8 bit -> 10 bit signed
			for (uint i = 0; i < 12; i++)
			{
				_c[i] = coeff(c[i]);
				_z[i] = 0; // clear feed-back values.		((note: verified))
			}
		}

		// for the given number of repetitions:
		for (; repeat; --repeat)
		{
			// for the given number of samples per pulse:
			for (_i = pitch ? pitch : 0x40; _i; _i--)
			{
				// generate next sample:
				// run pulse through filters and update feedback values:
				z0 = 0; // sample value moving through the filter

				// note: SP0250: pitch.bit6 activates white noise)
				// note: SP0256: pitch==0 activates noise with pitch=64)
				if (pitch == 0) // set z0 from noise:
				{
					// this is the rng from MAME. makes no difference from my version, but anyway:
					_shiftreg = (_shiftreg >> 1) ^ (_shiftreg & 1 ? 0x4001 : 0);
					z0		  = _shiftreg & 1 ? ampl(amplitude) : -ampl(amplitude);
				}
				else // vocal: set z0 for single pulse:
				{
					if (_i == pitch) z0 = ampl(amplitude);
				}

				// apply 6x 2-pole filter:
				for (uint j = 0; j < 12;)
				{
					z0 += _z[j] * _c[j] / 512;
					_z[j] = _z[j + 1];
					j++;
					z0 += _z[j] * _c[j] / 256;
					_z[j] = z0;
					j++;
				}

				YIELD(z0); // current output of chip
			}

			// apply increments:
			pitch += pitch_incr;
			amplitude += amplitude_incr;
		}
	}

	FINISH // co-routine
}


// --------------------------------------------------------------
//				read bits from serial rom
// --------------------------------------------------------------

template<uint nc>
uint8 SP0256<nc>::next8() noexcept
{
	// read next 8 bits from non-reversed rom
	// new bits come in from the left side

	if (bits < 8) byte += al2_rom[pc++ & 0x7ff] << bits;
	else bits -= 8;

	uint rval = byte;
	byte	  = byte >> 8;
	return uint8(rval);
}

template<uint nc>
uint8 SP0256<nc>::nextL(uint n) noexcept
{
	// read next N bits from rom
	// new bits come in from the left side
	// return value: bits are left-aligned

	if (bits < n)
	{
		byte += al2_rom[pc++ & 0x7ff] << bits;
		bits += 8;
	}
	bits -= n;

	uint rval = byte << (8 - n);
	byte	  = byte >> n;
	return uint8(rval);
}

template<uint nc>
inline int8 SP0256<nc>::nextSL(uint n) noexcept
{
	// signed variant

	return int8(nextL(n));
}

template<uint nc>
inline uint8 SP0256<nc>::nextR(uint n) noexcept
{
	// read next N bits from rom
	// new bits come in from the left side
	// return value: bits are right-aligned

	return nextL(n) >> (8 - n);
}

template<uint nc>
inline int8 SP0256<nc>::nextSR(uint n) noexcept
{
	// signed variant

	return nextSL(n) >> (8 - n);
}


// --------------------------------------------------------------
//				handle micro sequencer opcode
// --------------------------------------------------------------

/*	Micro sequencer:

	uint2 mode
		Controls the format of data which follows various instructions.
		In some cases, it also controls whether certain filter coefficients are zeroed or left unmodified.
		The exact meaning of MODE varies by instruction.
		MODE is sticky, meaning that once it is set, it retains its value until it is explicitly changed
		by Opcode 1000 (SETMODE) or the sequencer halts.

	uint2 repeat_prefix
		The parameter load instructions can provide a four bit repeat value to the filter core.
		This register optionally extends that four bit value by providing two more significant bits in the 2 MSBs.
		By setting the repeat prefix with Opcode 1000 (SETMODE), the program can specify repeat values up to $3F (63).
		This register is not sticky.

	uint4 page
		The PAGE register acts as a prefix, providing the upper 4 address bits (15…12) for JMP and JSR instructions.
		The PAGE register can hold any binary value from 0001 to 1111, and is set by the SETPAGE instruction.
		It is not possible to load it with 0000.
		It powers up to the value 0001, and it retains its value across JMP/JSR instructions and stand-by.

	uint16 pc
		This is the program counter. This counter tracks the address of the byte that is currently being processed.
		A copy of the program counter is kept in every Speech ROM that is attached to the SP0256,
		so that the program counter is only broadcast on JMP or JSR.

	uint16 stack
		This is where the program counter is saved when performing a JSR.
		The STACK has room for exactly one address, so nested subroutines are not possible.
		It holds the address of the byte following the JSR instruction.

	uint8 command
		This holds address of the most recent command from the host CPU.
		Addresses are loaded into this register via external pins and the ALD control line.
		When the microsequencer is halted (or is about to halt), it watches for an address in this register.
		When a new command address is available, it copies these bits to bits 1 through 8 of the program counter,
		bit 12 is set to 1 and all other bits are cleared to 0, so that code executes out of page $1.


	Key for opcode formats below

	Bits come out from the serial rom LSB first.
	Note: the original rom image was bit-swapped, but we use the unswapped version!
	Most bit fields, except those which specify branch targets, are unswapped, meaning the left-most bit is the MSB.
	Bit fields narrower than 8 bits are MSB justified unless specified otherwise, meaning that the least significant
	bits are the ones that are missing. These LSBs are filled with zeros.
	When updating filter coefficients with a delta-update, the microsequencer performs plain 2s-complement arithmetic
	on the 8-bit value in the coefficient register file. No attention is paid to the format of the register.
	kio TODO: also true for the once-per-pitch update?

	Field		Description
	AAAAAAAA 	Amplitude bits. The 3 msbits are the exponent, the 5 lsbits are the mantissa.
				the amplitude is decoded 8 bit -> 12 bit at the time when it is loaded into the pulse generator.
	PPPPPPPP 	Pitch period. When set to 0, the impulse switches to random noise.
				For timing purposes, noise and silence have an effective period equivalent to period==64.
	SBBBBBBB 	B coefficient data. The 'S' is the sign bit, if present. If there is no 'S' on a given field,
				the sign is assumed to be 0. (which means NEGATIVE - kio)
				coefficients are expanded from sign+7 bit to sign+9 bit via a rom table at time of calculation.
	SFFFFFFF 	F coefficient data.
	RRRR		Repeat bits. On Opcode SETMODE, the 2 repeat bits go to the two MSBs of the repeat count
				for the next instruction. On all other instructions, the repeat bits go to the 4 LSBs of the
				repeat count for the current instruction.
	MM			Mode bits. These are set by Opcode SETMODE. They specify the data format for following opcodes.
	LLLLLLLL 	Byte address for a branch target. Branch targets are 16 bits long.
				The JMP/JSR instruction provides the lower 12 bits, and the PAGE register provides the upper 4 bits.
				The branch target bits (LLLL in SETPAGE, LLLL and LLLLLLLL in JMP and JSR) are bit-swapped.
	aaaaa		Amplitude delta. (unsigned) note: applied to the 8-bit encoded amplitude!
	ppppp		Pitch delta. (unsigned)
	saaa		Amplitude delta. (2s complement) note: applied to the 8-bit encoded amplitude!
	sppp		Pitch delta. (2s complement)
	sbbb sfff 	Filter coefficient deltas. (2s complement) note: applied to the 8-bit encoded coefficients!
*/

template<uint nc>
void SP0256<nc>::cmdSetPage(uint llll) noexcept
{
	/*	RTS/SETPAGE – Return OR set the PAGE register
		0000 LLLL >>>

		• When LLLL is non-zero, this instruction sets the PAGE register to the value in LLLL.
		• LLLL is bit-swapped. (kio 2015-04-20 checked with rom disass)
		• The 4-bit PAGE register determines which 4K page the next JMPs or JSRs will go to.
		  Note that address loads via ALD ignore PAGE, and set the four MSBs to $1000. They do not modify
		  the PAGE register, so subsequent JMP/JSR instructions will jump relative to the current value in PAGE.
		• The PAGE register retains its setting until the next SETPAGE is encountered.
		• Valid values for PAGE are in the range $1..$F.
		• The RESROM starts at address $1000, and no code exists below that address.
		  Therefore, the microsequencer can address speech data over the address range $1000 through $FFFF, for a total
		  of 60K of speech data. (Up to 64K may be possible by wrapping around, but that's not yet verified.)

		RTS
		• When LLLL is zero, this opcode causes the microsequencer to pop the PC stack into the PC, and resume execution
		  there. The contents of the stack are replaced with $0000 (or some other flag which represents an empty stack).
		• If the address that was popped was itself $0000 (eg. an empty stack), execution halts, pending a new address
		  write via ALD. (Of course, if an address was previously written via ALD and is pending, control transfers to
		  that address immediately.)
	*/

	if (llll) // LLLL!=0 => SETPAGE
	{
		page = X4(llll) << 12;
		debugstr("SP0256: SETPAGE(%u)\n", page >> 12); // this command makes no sense with the 2kB AL2 rom
	}
	else // LLLL==0 => RTS
	{
		pc	  = stack;
		stack = byte = bits = 0; // pop address
		if (pc)
		{
			debugstr("SP0256: RTS\n"); // no JSR in AL2 rom => SETPAGE never used for RTS
			return;					   // RTS
		}

		// no address on stack => next command
		if (command_valid)
		{
			debugstr(
				"SP0256: RTS: next command = %u = %s\n", command, command < 64 ? al2_allophone_names[command] : "???");
			pc			  = 0x1000 + (command << 1);
			command_valid = false;
			stand_by	  = false;
			return;
		}

		// no next command => stand-by
		debugstr("SP0256: RTS: stand-by\n");
		stand_by = true;
	}
}

template<uint nc>
void SP0256<nc>::cmdJmp(uint instr) noexcept
{
	/*	JMP – Jump to Byte Address
		LLLLLLLL 1110 LLLL >>>
	
		Performs a jump to the specified 12-bit address inside the 4K page specified by the PAGE register.
		That is, the JMP instruction jumps to the location "PAGE.LLLL.LLLLLLLL", where the upper four bits come from
		the PAGE register and the lower 12 bits come from the JMP instruction.
		At power-up, the PAGE register defaults to address $1000. (value 0b0001)
		The PAGE register may be set using opcode SETPAGE.
		note: destination address bits LLLL and LLLLLLLL are reverse ordered (kio 2015-04-20 checked with rom disass)
	*/

	pc = page + (X4(instr & 15) << 8) + X8(next8());
	debugstr("SP0256: JMP: 0x%4u\n", pc);
}

template<uint nc>
void SP0256<nc>::cmdJsr(uint instr) noexcept
{
	/*	JSR – Jump to Subroutine
		LLLLLLLL 1101 LLLL >>>
		not used in AL2 rom
 
	 	Jump to the specified 12-bit address inside the 4K page specified by the PAGE register.
		RTS pushes the byte-aligned return address onto the PC stack, which can only hold one entry.
		To return to the next instruction, use Opcode RTS.
	*/
	stack = pc + 1;
	pc	  = page + (X4(instr & 15) << 8) + X8(next8());
	debugstr("SP0256: JSR: 0x%4u\n", pc);
}

template<uint nc>
void SP0256<nc>::cmdSetMode(uint instr) noexcept
{
	/*	SETMODE – Set the Mode bits and Repeat msbits
		0001 MM RR >>>
	 
	 	Prefix for parameter load opcodes.
		• Load the two RR bits into the two msbits of the 6-bit repeat register.
		  These two bits combine with the four lsbits that are provided by most parameter load opcodes
		  to provide longer repetition periods.
		  The RR bits are not sticky.
			kio: they are probably counted down to 0 by the audio filter loop
				 therefore: not only JMP and JSR, but also SETPAGE/RTS should not reset these bits.
		• The two MM bits select the data format for many of the parameter load opcodes.
		  The MM mode bits are sticky, meaning that they stay in effect until the next Opcode SETMODE instruction.
		• This opcode is known to have no effect on JMP/JSR instructions and JMP/JSR instructions have no effect on it.
	*/

	repeat = (instr & 3) << 4;
	mode   = (instr & 0xC) >> 2;
	debugstr("SP0256: SETMODE: RR=%u, MM=%u\n", instr & 3, mode);
}

template<uint nc>
void SP0256<nc>::cmdPause() noexcept
{
	/*	PAUSE – Silent pause
		1111 RRRR >>>
 
	 	Provides a silent pause of varying length.
		The length of the pause is given by the 4-bit immediate constant RRRR.
		The pause duration can be extended with the prefix opcode SETMODE.
		Notes: The pause behaves identially to a pitch with Amplitude=0 and Period=64.
		All coefficients are cleared.
	*/

	amplitude	   = 0;
	pitch		   = 64;
	amplitude_incr = 0; // not expressively stated, but otherwise pause with R>1 would not be silent
	pitch_incr	   = 0; // not expressively stated, but otherwise pause with R>1 would not be silent
	memset(c, 0, sizeof(c));
	debugstr("SP0256: PAUSE\n");
}

template<uint nc>
void SP0256<nc>::cmdLoadAll() noexcept
{
	/*	LOADALL – Load All Parameters
		[data] 1000 RRRR >>>
		not used in AL2 rom
  
	 	Load amplitude, pitch, and all coefficient pairs at full 8-bit precision.
		The pitch and amplitude deltas that are available in Mode 1x are applied every pitch period, not just once.
		Wraparound may occur. If the Pitch goes to zero, the periodic excitation switches to noise.
		TODO: also applied after the last loop?

		all modes:
		AAAAAAAA PPPPPPPP
		SBBBBBBB SFFFFFFF   (B0,F0)
		SBBBBBBB SFFFFFFF   (B1,F1)
		SBBBBBBB SFFFFFFF   (B2,F2)
		SBBBBBBB SFFFFFFF   (B3,F3)
		SBBBBBBB SFFFFFFF   (B4,F4)
		SBBBBBBB SFFFFFFF   (B5,F5)

		mode 1x:
		saaaaaaa sppppppp   (amplitude and pitch interpolation)
	*/

	amplitude = next8();
	pitch	  = next8();

	for (uint i = 0; i < 12; i++) { c[i] = next8(); }

	if (mode & 2) // 1x
	{
		amplitude_incr = int8(next8());
		pitch_incr	   = int8(next8());
	}
	else // 0x
	{
		amplitude_incr = 0;
		pitch_incr	   = 0;
	}

	debugstr("SP0256: LOADALL\n");
}

template<uint nc>
void SP0256<nc>::cmdLoadE() noexcept
{
	/*	LOAD_E – Load Pitch and Amplitude
		PPPPPPPP AAAAAA 0111 RRRR >>>
 
	 	Load new amplitude and pitch.
		There seem to be no data format variants for the different modes,
		although the repeat count may be extended using opcode SETMODE.

		all other registers preserved.
	*/

	amplitude = nextL(6);
	pitch	  = next8();

	debugstr("SP0256: LOAD_E\n");
}

template<uint nc>
void SP0256<nc>::cmdLoad4() noexcept
{
	/*	LOAD_4 – Load Pitch, Amplitude and Coefficients (2 or 3 stages)
		[data] 0010 RRRR >>>
 
	 	Load new amplitude and pitch parameters.
		Load new filter coefficients, setting the unspecified coefficients to 0.
		The exact combination and precision of data is determined by the mode bits as set by opcode SETMODE.
		For all modes, the sign bit for B0 has an implied value of 0.

		all modes:
		AAAAAA   PPPPPPPP

		mode x0:
		BBBB     SFFFFF     (B3,F3)
		SBBBBBB  SFFFFF     (B4,F4)

		mode x1:
		BBBBBB   SFFFFFF    (B3,F3)
		SBBBBBBB SFFFFFFF   (B4,F4)

		mode 1x:
		SBBBBBBB SFFFFFFF   (B5,F5)
	*/

	amplitude	   = nextL(6);
	pitch		   = next8();
	amplitude_incr = 0;
	pitch_incr	   = 0;

	memset(c, 0, sizeof(c)); // set the unspecified coefficients to 0

	if (mode & 1) // x1
	{
		B3 = nextL(6) >> 1;
		F3 = nextL(7);
		B4 = next8();
		F4 = next8();
	}
	else // x0
	{
		B3 = nextL(4) >> 1;
		F3 = nextL(6);
		B4 = nextL(7);
		F4 = nextL(6);
	}

	if (mode & 2) // 1x
	{
		B5 = next8();
		F5 = next8();
	}

	debugstr("SP0256: LOAD_4\n");
}

template<uint nc>
void SP0256<nc>::cmdLoad2C(uint instr) noexcept
{
	/*	LOAD_C – Load Pitch, Amplitude, Coefficients (5 or 6 stages)
		LOAD_2 – Load Pitch, Amplitude, Coefficients (5 or 6 stages), and Interpolation Registers
		[data] 0011 RRRR >>>
		[data] 0100 RRRR >>>
 
	 	Load new amplitude and pitch parameters.
		Load new filter coefficients, setting the unspecified coefficients to zero.
		The exact combination and precision of data is determined by the mode bits as set by opcode SETMODE.
		• For all Modes, the Sign bit for B0, B1, B2 and B3 has an implied value of 0.
		• LOAD_2 and LOAD_C only differ in that LOAD_2 also loads new values into the
		  Amplitude and Pitch Interpolation Registers while LOAD_C doesn't.

		all modes:
		AAAAAA   PPPPPPPP

		mode x0:
		BBB      SFFFF      (coeff pair 0)
		BBB      SFFFF      (coeff pair 1)
		BBB      SFFFF      (coeff pair 2)
		BBBB     SFFFFF     (coeff pair 3)
		SBBBBBB  SFFFFF     (coeff pair 4)

		mode x1:
		BBBBBB   SFFFFF     (coeff pair 0)
		BBBBBB   SFFFFF     (coeff pair 1)
		BBBBBB   SFFFFF     (coeff pair 2)
		BBBBBB   SFFFFFF    (coeff pair 3)
		SBBBBBBB SFFFFFFF   (coeff pair 4)

		mode 1x:
		SBBBBBBB SFFFFFFF   (coeff pair 5)

		LOAD_2 only, all modes:
		aaaaa    ppppp      (Interpolation register LSBs)
	*/

	amplitude = nextL(6);
	pitch	  = next8();

	if (mode & 1) // x1
	{
		B0 = nextL(6) >> 1;
		F0 = nextL(6);
		B1 = nextL(6) >> 1;
		F1 = nextL(6);
		B2 = nextL(6) >> 1;
		F2 = nextL(6);
		B3 = nextL(6) >> 1;
		F3 = nextL(7);
		B4 = next8();
		F4 = next8();
	}
	else // x0
	{
		B0 = nextL(3) >> 1;
		F0 = nextL(5);
		B1 = nextL(3) >> 1;
		F1 = nextL(5);
		B2 = nextL(3) >> 1;
		F2 = nextL(5);
		B3 = nextL(4) >> 1;
		F3 = nextL(6);
		B4 = nextL(7);
		F4 = nextL(6);
	}
	if (mode & 2) // 1x
	{
		B5 = next8();
		F5 = next8();
	}
	else // 0x
	{
		B5 = 0;
		F5 = 0;
	}

	if (instr == LOAD_2) // LOAD_2
	{
		amplitude_incr = int8(nextR(5));
		pitch_incr	   = int8(nextR(5));
	}
	else // LOAD_C
	{
		amplitude_incr = 0;
		pitch_incr	   = 0;
	}
}

template<uint nc>
inline void SP0256<nc>::cmdLoad2() noexcept
{
	/*	LOAD_2 – Load Pitch, Amplitude, Coefficients, and Interpolation Registers
		[data] 0100 RRRR >>>
	*/

	debugstr("SP0256: LOAD_2\n");
	cmdLoad2C(LOAD_2);
}

template<uint nc>
inline void SP0256<nc>::cmdLoadC() noexcept
{
	/*	LOAD_C – Load Pitch, Amplitude, Coefficients (5 or 6 stages)
		[data] 0011 RRRR >>>
	*/
	debugstr("SP0256: LOAD_C\n");
	cmdLoad2C(LOAD_C);
}

template<uint nc>
void SP0256<nc>::cmdSetMsb6() noexcept
{
	/*	SETMSB_6 – Load Amplitude and MSBs of 2 or 3 'F' Coefficients
		[data] 0110 RRRR >>>
		not used in AL2 rom

	 	Load new amplitude. Updates the msbits of a set of filter coefficients.
		The mode bits controls the update process as noted below. Opcode SETMODE provides the mode bits.
		Notes:
		MODE x0: the 6 msbits of F3 and F4 are set, the lsbits are not modified.
		MODE x1: the 7 msbits of F3 and all 8 bits of F4 are set, the lsbit of F3 is not modified.
		MODE 1x: all 8 bits of F5 are set, and B5 is not modified.
		MODE 0x: B5 and F5 are set to zero.

		all modes:
		AAAAAA

		mode x0:
		SFFFFF              (New F3 6 MSBs)
		SFFFFF              (New F4 6 MSBs)

		mode x1:
		SFFFFFF             (New F3 7 MSBs)
		SFFFFFFF            (New F4 8 MSBs)

		mode 1x:
		SFFFFFFF            (New F5 8 MSBs)
	*/

	debugstr("SP0256: SETMSB_6\n"); // not used in AL2 rom

	amplitude = nextL(6);

	if (mode & 1) // x1
	{
		F3 = nextL(7) + (F3 & 1);
		F4 = next8();
	}
	else // x0
	{
		F3 = nextL(6) + (F3 & 3);
		F4 = nextL(6) + (F4 & 3);
	}

	if (mode & 2) // 1x
	{
		F5 = next8();
	}
	else // 0x
	{
		F5 = B5 = 0;
	}
}

template<uint nc>
void SP0256<nc>::cmdSetMsb35A(uint instr) noexcept
{
	/*	SETMSB_5 – Load Amplitude, Pitch, and MSBs of 3 Coefficients
		SETMSB_A – Load Amplitude,        and MSBs of 3 Coefficients
		SETMSB_3 – Load Amplitude,        and MSBs of 3 Coefficients, and Interpolation Registers
		[data] 1010 RRRR >>>
		[data] 0101 RRRR >>>
		[data] 1100 RRRR >>>
	 
	 	Load new amplitude and pitch parameters.
		Update the MSBs of a set of filter coefficients.
		The Mode bits control the update process as noted below. Opcode SETMODE provides the mode bits.
		Notes:
		MODE x0: Set the 5 or 6 msbits of F0, F1, and F2, the lsbits are preserved.
		MODE x1: Set the 5 or 6 msbits of F0, F1, and F2, the lsbits are preserved.
		MODE 0x: F5 and B5 are set to 0. All other coefficients are preserved.
		MODE 1x: F5 and B5 are preserved. All other coefficients are preserved.
		Opcode SETMSB_5 also sets the Pitch.
		Opcode SETMSB_3 also sets the interpolation registers.

		all opcodes, all modes:
		AAAAAA

		SETMSB_5, all modes:
		PPPPPPPP

		all opcodes, mode x0:
		SFFFF			(F0 5 MSBs)
		SFFFF			(F1 5 MSBs)
		SFFFF			(F2 5 MSBs)

		all opcodes, mode x1:
		SFFFFF			(F0 6 MSBs)
		SFFFFF			(F1 6 MSBs)
		SFFFFF			(F2 6 MSBs)

		SETMSB_3, all modes:
		aaaaa    ppppp	(incr. LSBs)
	*/

	amplitude = nextL(6);
	if (instr == SETMSB5) { pitch = next8(); }

	if (mode & 1) // x1
	{
		F0 = nextL(6) + (F0 & 3);
		F1 = nextL(6) + (F1 & 3);
		F2 = nextL(6) + (F2 & 3);
	}
	else // x0
	{
		F0 = nextL(5) + (F0 & 7);
		F1 = nextL(5) + (F1 & 7);
		F2 = nextL(5) + (F2 & 7);
	}

	if (mode & 2) // 1x
	{}
	else // 0x
	{
		F5 = B5 = 0;
	}

	if (instr == SETMSB3)
	{
		amplitude_incr = int8(nextR(5));
		pitch_incr	   = int8(nextR(5));
	}
}

template<uint nc>
inline void SP0256<nc>::cmdSetMsb3() noexcept
{
	// SETMSB_3 – Load Amplitude, MSBs of 3 Coefficients, and Interpolation Registers
	// [data] 1100 RRRR >>>

	debugstr("SP0256: SETMSB_3\n");
	cmdSetMsb35A(SETMSB3);
}

template<uint nc>
inline void SP0256<nc>::cmdSetMsb5() noexcept
{
	// SETMSB_5 – Load Amplitude, Pitch, and MSBs of 3 Coefficients
	// [data] 1010 RRRR >>>

	debugstr("SP0256: SETMSB_5\n");
	cmdSetMsb35A(SETMSB5);
}

template<uint nc>
inline void SP0256<nc>::cmdSetMsbA() noexcept
{
	// SETMSB_A – Load Amplitude and MSBs of 3 Coefficients
	// [data] 0101 RRRR >>>

	debugstr("SP0256: SETMSB_A\n");
	cmdSetMsb35A(SETMSBA);
}

template<uint nc>
void SP0256<nc>::cmdDelta9() noexcept
{
	/*	DELTA_9 – Delta update Amplitude, Pitch and 5 or 6 Coefficients
		[data] 1001 RRRR
	
		Performs a delta update, adding small 2s complement numbers to a series of coefficients.
		The 2s complement updates for the various filter coefficients only update some of the MSBs,
		the LSBs are unaffected.
		The exact bits which are updated are noted below.
		Notes
		• The delta update is applied exactly once, as long as the repeat count is at least 1.
		• If the repeat count is greater than 1, the updated value is held through the repeat period,
		  but the delta update is not reapplied.
		• The delta updates are applied to the 8-bit encoded forms of the coefficients, not the 10-bit decoded forms.
		  Normal 2s complement arithmetic is performed, and no protection is provided against overflow.
		  Adding 1 to the largest value for a bit field wraps around to the smallest value for that bitfield.
		• The update to the amplitude register is a normal 2s complement update to the 8-bit register.
		  This means that any carry/borrow from the mantissa will change the value of the exponent.
		  The update doesn't know anything about the format of that register.

		all modes:
		saaa     spppp      (Amplitude 6 MSBs, Pitch LSBs.)

		mode x0:
		sbb      sff        (B0 4 MSBs, F0 5 MSBs.)
		sbb      sff        (B1 4 MSBs, F1 5 MSBs.)
		sbb      sff        (B2 4 MSBs, F2 5 MSBs.)
		sbb      sfff       (B3 5 MSBs, F3 6 MSBs.)
		sbbb     sfff       (B4 6 MSBs, F4 6 MSBs.)	// Zbiciak: 6 bits, MAME: B4 7 Bits!

		mode x1:
		sbbb     sfff       (B0 7 MSBs, F0 6 MSBs.)
		sbbb     sfff       (B1 7 MSBs, F1 6 MSBs.)
		sbbb     sfff       (B2 7 MSBs, F2 6 MSBs.)
		sbbb     sffff      (B3 7 MSBs, F3 7 MSBs.)
		sbbbb    sffff      (B4 8 MSBs, F4 8 MSBs.)

		mode 1x:
		sbbbb    sffff      (B5 8 MSBs, F5 8 MSBs.)
	*/

	debugstr("SP0256: DELTA_9\n");

	amplitude += nextSL(4) >> 2;
	pitch += nextSL(5) >> 3; // TODO: verify, ob alle 8 bits beeinflusst werden. sollte aber stimmen.

	if (mode & 1) // x1
	{
		B0 += nextSL(4) >> 3;
		F0 += nextSL(4) >> 2;
		B1 += nextSL(4) >> 3;
		F1 += nextSL(4) >> 2;
		B2 += nextSL(4) >> 3;
		F2 += nextSL(4) >> 2;
		B3 += nextSL(4) >> 3;
		F3 += nextSL(5) >> 2;
		B4 += nextSL(5) >> 3;
		F4 += nextSL(5) >> 3;
	}
	else // x0
	{
		B0 += nextSL(3) >> 1;
		F0 += nextSL(3) >> 2;
		B1 += nextSL(3) >> 1;
		F1 += nextSL(3) >> 2;
		B2 += nextSL(3) >> 1;
		F2 += nextSL(3) >> 2;
		B3 += nextSL(3) >> 2;
		F3 += nextSL(4) >> 2;
#if 0
		B4 += nextSL(4)>>2;	  // Zbiciak: 6 bits	i can't hear a difference
		F4 += nextSL(4)>>2;	  // Zbiciak: 6 bits	i can't hear a difference
#else
		B4 += nextSL(4) >> 2;
		F4 += nextSL(4) >> 3; // MAME: 7 bits		but i tend to MAME beeing right
#endif
	}

	if (mode & 2)
	{
		B5 += nextSL(5) >> 3;
		F5 += nextSL(5) >> 3;
	}
}

template<uint nc>
void SP0256<nc>::cmdDeltaD() noexcept
{
	/* 	DELTA_D – Delta update Amplitude, Pitch and 2 or 3 Coefficients
		[data] 1011 RRRR >>>
	 
	 	Performs a delta update, adding small 2s complement numbers to a series of coefficients.
		The 2s complement updates for the various filter coefficients only update some of the MSBs --
		the LSBs are unaffected.
		The exact bits which are updated are noted below.
		Notes
		• The delta update is applied exactly once, as long as the repeat count is at least 1.
		  If the repeat count is greater than 1, the updated value is held through the repeat period,
		  but the delta update is not reapplied.
		• The delta updates are applied to the 8-bit encoded forms of the coefficients, not the 10-bit decoded forms.
		• Normal 2s complement arithmetic is performed, and no protection is provided against overflow.
		  Adding 1 to the largest value for a bit field wraps around to the smallest value for that bitfield.
		• The update to the amplitude register is a normal 2s complement update to the entire register.
		  This means that any carry/borrow from the mantissa will change the value of the exponent.
		  The update doesn't know anything about the format of that register.

		all modes:
		saaa     spppp      (Amplitude 6 MSBs, Pitch LSBs.)

		mode x0:
		sbb      sfff       (B3 5 MSBs, F3 6 MSBs.)
		sbbb     sfff       (B4 7 MSBs, F4 6 MSBs.)

		mode x1:
		sbbb     sffff      (B3 7 MSBs, F3 7 MSBs.)
		sbbbb    sffff      (B4 8 MSBs, F4 8 MSBs.)

		mode 1x:
		sbbbb    sffff      (B5 8 MSBs, F5 8 MSBs.)
	*/

	debugstr("SP0256: DELTA_D\n");

	amplitude += nextSL(4) >> 2;
	pitch += nextSL(5) >> 3; // TODO: verify, ob alle 8 bits beeinflusst werden. sollte aber stimmen.

	if (mode & 1) // x1
	{
		B3 += nextSL(4) >> 3;
		F3 += nextSL(5) >> 2;
		B4 += nextSL(5) >> 3;
		F4 += nextSL(5) >> 3;
	}
	else // x0
	{
		B3 += nextSL(3) >> 2;
		F3 += nextSL(4) >> 2;
		B4 += nextSL(4) >> 3;
		F4 += nextSL(4) >> 2;
	}

	if (mode & 2) // 1x
	{
		B5 += nextSL(5) >> 3;
		F5 += nextSL(5) >> 3;
	}
}


// --------------------------------------------------------------
//				disassemble SP0256 rom
// --------------------------------------------------------------

template<uint nc>
void SP0256<nc>::disassAllophones() noexcept
{
	debugstr("SP0256: allophone rom disassembly\n");

	for (uint i = 0; i < 64; i++)
	{
		debugstr("allophone %2u: %s\n", i, al2_allophone_names[i]);

		pc			  = i << 1;
		byte		  = 0;
		bits		  = 0;
		repeat		  = 0;
		stack		  = 0;
		mode		  = 0;
		page		  = 0;
		command_valid = false;
		stand_by	  = false;

		while (!stand_by)
		{
			uint instr			= next8();
			uint current_opcode = instr >> 4;

			switch (current_opcode)
			{
			case SETPAGE: cmdSetPage(instr); continue; // and RTS
			case SETMODE: cmdSetMode(instr); continue;
			case JMP: cmdJmp(instr); continue;
			case JSR: cmdJsr(instr); continue;
			default: break;
			}

			repeat += (instr & 15);
			debugstr(
				"%2u: %s: m=%u, r=%u%s ", current_opcode, opcode_names[current_opcode], mode, repeat,
				repeat > 9 ? "" : " "); // no \n
			repeat = 0;

			switch (current_opcode)
			{
			case PAUSE: logPause(); break;
			case LOADALL: IERR();
			case LOAD_2: logLoad2(); break;
			case LOAD_4: logLoad4(); break;
			case LOAD_C: logLoadC(); break;
			case LOAD_E: logLoadE(); break;
			case SETMSB3: logSetMsb3(); break;
			case SETMSB5: logSetMsb5(); break;
			case SETMSB6: IERR();
			case SETMSBA: logSetMsbA(); break;
			case DELTA_9: logDelta9(); break;
			case DELTA_D: logDeltaD(); break;
			default: IERR();
			}

			assert(pitch_incr == 0);
			assert(amplitude_incr == 0);
			debugstr("\n");
		}
	}
}

#define AMP(A) (((A)&0x1F) << (((A) >> 5) & 0x07))
#define COF(N) ((N)&0x80 ? coeff_tab[(~(N)) & 0x7F] : -coeff_tab[N])

template<uint nc>
void SP0256<nc>::logPause() noexcept
{
	/*	PAUSE – Silent pause
		1111 RRRR >>>
	 
	 	Provides a silent pause of varying length.
		The length of the pause is given by the 4-bit immediate constant RRRR.
		The pause duration can be extended with the prefix opcode SETMODE.
		Notes: The pause behaves identially to a pitch with Amplitude=0 and Period=64.
		All coefficients are cleared.
	*/

	amplitude	   = 0;
	pitch		   = 64;
	amplitude_incr = 0; // not expressively stated, but otherwise pause with R>1 would not be silent
	pitch_incr	   = 0; // not expressively stated, but otherwise pause with R>1 would not be silent
	memset(c, 0, sizeof(c));

	debugstr("p=%2u a=%4u  ", pitch, AMP(amplitude)); // no \n
}

template<uint nc>
void SP0256<nc>::logLoadE() noexcept
{
	/*	LOAD_E – Load Pitch and Amplitude
		PPPPPPPP AAAAAA 0111 RRRR >>>
	
		Load new amplitude and pitch.
		There seem to be no data format variants for the different modes,
		although the repeat count may be extended using opcode SETMODE.

		probably: all other registers preserved.
	*/

	amplitude = nextL(6);
	pitch	  = next8();

	debugstr("p=%2u a=%4u  ", pitch, AMP(amplitude)); // no \n
}

template<uint nc>
void SP0256<nc>::logLoad4() noexcept
{
	/*	LOAD_4 – Load Pitch, Amplitude and Coefficients (2 or 3 stages)
		[data] 0010 RRRR >>>
	 
	 	Load new amplitude and pitch parameters.
		Load new filter coefficients, setting the unspecified coefficients to 0.
		The exact combination and precision of data is determined by the mode bits as set by opcode SETMODE.
		For all modes, the sign bit for B0 has an implied value of 0.

		all modes:
		AAAAAA   PPPPPPPP

		mode x0:
		BBBB     SFFFFF     (B3,F3)
		SBBBBBB  SFFFFF     (B4,F4)

		mode x1:
		BBBBBB   SFFFFFF    (B3,F3)
		SBBBBBBB SFFFFFFF   (B4,F4)

		mode 1x:
		SBBBBBBB SFFFFFFF   (B5,F5)
	*/

	amplitude	   = nextL(6);
	pitch		   = next8();
	amplitude_incr = 0;
	pitch_incr	   = 0;

	memset(c, 0, sizeof(c)); // set the unspecified coefficients to 0

	if (mode & 1) // x1
	{
		B3 = nextL(6) >> 1;
		F3 = nextL(7);
		B4 = next8();
		F4 = next8();
	}
	else // x0
	{
		B3 = nextL(4) >> 1;
		F3 = nextL(6);
		B4 = nextL(7);
		F4 = nextL(6);
	}

	if (mode & 2) // 1x
	{
		B5 = next8();
		F5 = next8();
	}

	debugstr("p=%2u a=%4u  ", pitch, AMP(amplitude));
	debugstr("F0=%+4i,%+4i ", COF(B0), COF(F0));
	debugstr("F1=%+4i,%+4i ", COF(B1), COF(F1));
	debugstr("F2=%+4i,%+4i ", COF(B2), COF(F2));
	debugstr("F3=%+4i,%+4i ", COF(B3), COF(F3));
	debugstr("F4=%+4i,%+4i ", COF(B4), COF(F4));
	debugstr("F5=%+4i,%+4i ", COF(B5), COF(F5));
}

template<uint nc>
void SP0256<nc>::logLoad2C(uint instr) noexcept
{
	/*	LOAD_C – Load Pitch, Amplitude, Coefficients (5 or 6 stages)
		LOAD_2 – Load Pitch, Amplitude, Coefficients (5 or 6 stages), and Interpolation Registers
		[data] 0011 RRRR >>>
		[data] 0100 RRRR >>>
	 
	 	Load new amplitude and pitch parameters.
		Load new filter coefficients, setting the unspecified coefficients to zero.
		The exact combination and precision of data is determined by the mode bits as set by opcode SETMODE.
		• For all Modes, the Sign bit for B0, B1, B2 and B3 has an implied value of 0.
		• LOAD_2 and LOAD_C only differ in that LOAD_2 also loads new values into the
		  Amplitude and Pitch Interpolation Registers while LOAD_C doesn't.

		all modes:
		AAAAAA   PPPPPPPP

		mode x0:
		BBB      SFFFF      (coeff pair 0)
		BBB      SFFFF      (coeff pair 1)
		BBB      SFFFF      (coeff pair 2)
		BBBB     SFFFFF     (coeff pair 3)
		SBBBBBB  SFFFFF     (coeff pair 4)

		mode x1:
		BBBBBB   SFFFFF     (coeff pair 0)
		BBBBBB   SFFFFF     (coeff pair 1)
		BBBBBB   SFFFFF     (coeff pair 2)
		BBBBBB   SFFFFFF    (coeff pair 3)
		SBBBBBBB SFFFFFFF   (coeff pair 4)

		mode 1x:
		SBBBBBBB SFFFFFFF   (coeff pair 5)

		LOAD_2 only, all modes:
		aaaaa    ppppp      (Interpolation register LSBs)
	*/

	amplitude = nextL(6);
	pitch	  = next8();

	if (mode & 1) // x1
	{
		B0 = nextL(6) >> 1;
		F0 = nextL(6);
		B1 = nextL(6) >> 1;
		F1 = nextL(6);
		B2 = nextL(6) >> 1;
		F2 = nextL(6);
		B3 = nextL(6) >> 1;
		F3 = nextL(7);
		B4 = next8();
		F4 = next8();
	}
	else // x0
	{
		B0 = nextL(3) >> 1;
		F0 = nextL(5);
		B1 = nextL(3) >> 1;
		F1 = nextL(5);
		B2 = nextL(3) >> 1;
		F2 = nextL(5);
		B3 = nextL(4) >> 1;
		F3 = nextL(6);
		B4 = nextL(7);
		F4 = nextL(6);
	}
	if (mode & 2) // 1x
	{
		B5 = next8();
		F5 = next8();
	}
	else // 0x
	{
		B5 = 0;
		F5 = 0;
	}

	if (instr == LOAD_2) // LOAD_2
	{
		amplitude_incr = int8(nextR(5));
		pitch_incr	   = int8(nextR(5));
	}
	else // LOAD_C
	{
		amplitude_incr = 0;
		pitch_incr	   = 0;
	}

	debugstr("p=%2u a=%4u  ", pitch, AMP(amplitude));
	debugstr("F0=%+4i,%+4i ", COF(B0), COF(F0));
	debugstr("F1=%+4i,%+4i ", COF(B1), COF(F1));
	debugstr("F2=%+4i,%+4i ", COF(B2), COF(F2));
	debugstr("F3=%+4i,%+4i ", COF(B3), COF(F3));
	debugstr("F4=%+4i,%+4i ", COF(B4), COF(F4));
	debugstr("F5=%+4i,%+4i ", COF(B5), COF(F5));
}

template<uint nc>
void SP0256<nc>::logLoad2() noexcept
{
	// LOAD_2 – Load Pitch, Amplitude, Coefficients, and Interpolation Registers
	// [data] 0100 RRRR >>>

	logLoad2C(LOAD_2);
}

template<uint nc>
void SP0256<nc>::logLoadC() noexcept
{
	// LOAD_C – Load Pitch, Amplitude, Coefficients (5 or 6 stages)
	// [data] 0011 RRRR >>>

	logLoad2C(LOAD_C);
}

template<uint nc>
void SP0256<nc>::logSetMsb35A(uint instr) noexcept
{
	/*	SETMSB_5 – Load Amplitude, Pitch, and MSBs of 3 Coefficients
		SETMSB_A – Load Amplitude,        and MSBs of 3 Coefficients
		SETMSB_3 – Load Amplitude,        and MSBs of 3 Coefficients, and Interpolation Registers
		[data] 1010 RRRR >>>
		[data] 0101 RRRR >>>
		[data] 1100 RRRR >>>
	 
	 	Load new amplitude and pitch parameters.
		Update the MSBs of a set of filter coefficients.
		The Mode bits control the update process as noted below. Opcode SETMODE provides the mode bits.
		Notes:
		MODE x0: Set the 5 or 6 msbits of F0, F1, and F2, the lsbits are preserved.
		MODE x1: Set the 5 or 6 msbits of F0, F1, and F2, the lsbits are preserved.
		MODE 0x: F5 and B5 are set to 0. All other coefficients are preserved.
		MODE 1x: F5 and B5 are preserved. All other coefficients are preserved.
		Opcode SETMSB_5 also sets the Pitch.
		Opcode SETMSB_3 also sets the interpolation registers.

		all opcodes, all modes:
		AAAAAA

		SETMSB_5, all modes:
		PPPPPPPP

		all opcodes, mode x0:
		SFFFF			(F0 5 MSBs)
		SFFFF			(F1 5 MSBs)
		SFFFF			(F2 5 MSBs)

		all opcodes, mode x1:
		SFFFFF			(F0 6 MSBs)
		SFFFFF			(F1 6 MSBs)
		SFFFFF			(F2 6 MSBs)

		SETMSB_3, all modes:
		aaaaa    ppppp	(incr. LSBs)
	*/

	amplitude = nextL(6);
	if (instr == SETMSB5) { pitch = next8(); }

	if (mode & 1) // x1
	{
		F0 = nextL(6) + (F0 & 3);
		F1 = nextL(6) + (F1 & 3);
		F2 = nextL(6) + (F2 & 3);
	}
	else // x0
	{
		F0 = nextL(5) + (F0 & 7);
		F1 = nextL(5) + (F1 & 7);
		F2 = nextL(5) + (F2 & 7);
	}

	if (mode & 2) // 1x
	{}
	else // 0x
	{
		F5 = B5 = 0;
	}

	if (instr == SETMSB3)
	{
		amplitude_incr = int8(nextR(5));
		pitch_incr	   = int8(nextR(5));
	}

	debugstr("p=%2u a=%4u  ", pitch, AMP(amplitude));
	debugstr("F0=%+4i,%+4i ", COF(B0), COF(F0));
	debugstr("F1=%+4i,%+4i ", COF(B1), COF(F1));
	debugstr("F2=%+4i,%+4i ", COF(B2), COF(F2));
	debugstr("F5=%+4i,%+4i ", COF(B5), COF(F5));
}

template<uint nc>
void SP0256<nc>::logSetMsb3() noexcept
{
	/*	SETMSB_3 – Load Amplitude, MSBs of 3 Coefficients, and Interpolation Registers
		[data] 1100 RRRR >>>
	*/
	logSetMsb35A(SETMSB3);
}

template<uint nc>
void SP0256<nc>::logSetMsb5() noexcept
{
	/*	SETMSB_5 – Load Amplitude, Pitch, and MSBs of 3 Coefficients
		[data] 1010 RRRR >>>
	*/
	logSetMsb35A(SETMSB5);
}

template<uint nc>
void SP0256<nc>::logSetMsbA() noexcept
{
	/*	SETMSB_A – Load Amplitude and MSBs of 3 Coefficients
		[data] 0101 RRRR >>>
	*/
	logSetMsb35A(SETMSBA);
}

template<uint nc>
void SP0256<nc>::logDelta9() noexcept
{
	/*	DELTA_9 – Delta update Amplitude, Pitch and 5 or 6 Coefficients
		[data] 1001 RRRR
	
		Performs a delta update, adding small 2s complement numbers to a series of coefficients.
		The 2s complement updates for the various filter coefficients only update some of the MSBs,
		the LSBs are unaffected.
		The exact bits which are updated are noted below.
		Notes
		• The delta update is applied exactly once, as long as the repeat count is at least 1.
		• If the repeat count is greater than 1, the updated value is held through the repeat period,
		  but the delta update is not reapplied.
		• The delta updates are applied to the 8-bit encoded forms of the coefficients, not the 10-bit decoded forms.
		  Normal 2s complement arithmetic is performed, and no protection is provided against overflow.
		  Adding 1 to the largest value for a bit field wraps around to the smallest value for that bitfield.
		• The update to the amplitude register is a normal 2s complement update to the 8-bit register.
		  This means that any carry/borrow from the mantissa will change the value of the exponent.
		  The update doesn't know anything about the format of that register.

		all modes:
		saaa     spppp      (Amplitude 6 MSBs, Pitch LSBs.)

		mode x0:
		sbb      sff        (B0 4 MSBs, F0 5 MSBs.)
		sbb      sff        (B1 4 MSBs, F1 5 MSBs.)
		sbb      sff        (B2 4 MSBs, F2 5 MSBs.)
		sbb      sfff       (B3 5 MSBs, F3 6 MSBs.)
		sbbb     sfff       (B4 6 MSBs, F4 6 MSBs.)	// TODO: MAME: B4 7 Bits!

		mode x1:
		sbbb     sfff       (B0 7 MSBs, F0 6 MSBs.)
		sbbb     sfff       (B1 7 MSBs, F1 6 MSBs.)
		sbbb     sfff       (B2 7 MSBs, F2 6 MSBs.)
		sbbb     sffff      (B3 7 MSBs, F3 7 MSBs.)
		sbbbb    sffff      (B4 8 MSBs, F4 8 MSBs.)

		mode 1x:
		sbbbb    sffff      (B5 8 MSBs, F5 8 MSBs.)
	*/

	amplitude += nextSL(4) >> 2;
	pitch += nextSL(5) >> 3; // TODO: verify, ob alle 8 bits beeinflusst werden. sollte aber stimmen.

	if (mode & 1) // x1
	{
		B0 += nextSL(4) >> 3;
		F0 += nextSL(4) >> 2;
		B1 += nextSL(4) >> 3;
		F1 += nextSL(4) >> 2;
		B2 += nextSL(4) >> 3;
		F2 += nextSL(4) >> 2;
		B3 += nextSL(4) >> 3;
		F3 += nextSL(5) >> 2;
		B4 += nextSL(5) >> 3;
		F4 += nextSL(5) >> 3;
	}
	else // x0
	{
		B0 += nextSL(3) >> 1;
		F0 += nextSL(3) >> 2;
		B1 += nextSL(3) >> 1;
		F1 += nextSL(3) >> 2;
		B2 += nextSL(3) >> 1;
		F2 += nextSL(3) >> 2;
		B3 += nextSL(3) >> 2;
		F3 += nextSL(4) >> 2;
		B4 += nextSL(4) >> 2;
		F4 += nextSL(4) >> 3; // Zbiciak: 6 bits, MAME: 7 bits
	}

	if (mode & 2)
	{
		B5 += nextSL(5) >> 3;
		F5 += nextSL(5) >> 3;
	}

	debugstr("p=%2u a=%4u  ", pitch, AMP(amplitude));
	debugstr("F0=%+4i,%+4i ", COF(B0), COF(F0));
	debugstr("F1=%+4i,%+4i ", COF(B1), COF(F1));
	debugstr("F2=%+4i,%+4i ", COF(B2), COF(F2));
	debugstr("F3=%+4i,%+4i ", COF(B3), COF(F3));
	debugstr("F4=%+4i,%+4i ", COF(B4), COF(F4));
	debugstr("F5=%+4i,%+4i ", COF(B5), COF(F5));
}

template<uint nc>
void SP0256<nc>::logDeltaD() noexcept
{
	/* 	DELTA_D – Delta update Amplitude, Pitch and 2 or 3 Coefficients
		[data] 1011 RRRR >>>
	 
	 	Performs a delta update, adding small 2s complement numbers to a series of coefficients.
		The 2s complement updates for the various filter coefficients only update some of the MSBs --
		the LSBs are unaffected.
		The exact bits which are updated are noted below.
		Notes
		• The delta update is applied exactly once, as long as the repeat count is at least 1.
		  If the repeat count is greater than 1, the updated value is held through the repeat period,
		  but the delta update is not reapplied.
		• The delta updates are applied to the 8-bit encoded forms of the coefficients, not the 10-bit decoded forms.
		• Normal 2s complement arithmetic is performed, and no protection is provided against overflow.
		  Adding 1 to the largest value for a bit field wraps around to the smallest value for that bitfield.
		• The update to the amplitude register is a normal 2s complement update to the entire register.
		  This means that any carry/borrow from the mantissa will change the value of the exponent.
		  The update doesn't know anything about the format of that register.

		all modes:
		saaa     spppp      (Amplitude 6 MSBs, Pitch LSBs.)

		mode x0:
		sbb      sfff       (B3 5 MSBs, F3 6 MSBs.)
		sbbb     sfff       (B4 7 MSBs, F4 6 MSBs.)

		mode x1:
		sbbb     sffff      (B3 7 MSBs, F3 7 MSBs.)
		sbbbb    sffff      (B4 8 MSBs, F4 8 MSBs.)

		mode 1x:
		sbbbb    sffff      (B5 8 MSBs, F5 8 MSBs.)
	*/

	amplitude += nextSL(4) >> 2;
	pitch += nextSL(5) >> 3; // TODO: verify, ob alle 8 bits beeinflusst werden. sollte aber stimmen.

	if (mode & 1) // x1
	{
		B3 += nextSL(4) >> 3;
		F3 += nextSL(5) >> 2;
		B4 += nextSL(5) >> 3;
		F4 += nextSL(5) >> 3;
	}
	else // x0
	{
		B3 += nextSL(3) >> 2;
		F3 += nextSL(4) >> 2;
		B4 += nextSL(4) >> 3;
		F4 += nextSL(4) >> 2;
	}

	if (mode & 2) // 1x
	{
		B5 += nextSL(5) >> 3;
		F5 += nextSL(5) >> 3;
	}

	debugstr("p=%2u a=%4u  ", pitch, AMP(amplitude));
	debugstr("F3=%+4i,%+4i ", COF(B3), COF(F3));
	debugstr("F4=%+4i,%+4i ", COF(B4), COF(F4));
	debugstr("F5=%+4i,%+4i ", COF(B5), COF(F5));
}


// instantiate both.
// the linker will know what we need:

template class SP0256<1>;
template class SP0256<2>;

} // namespace kio::Audio

/*




































































*/
