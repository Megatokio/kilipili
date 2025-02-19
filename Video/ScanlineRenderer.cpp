// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

/*
	This file provides Scanline Renderers which render one scanline in their respective Color Mode.

	note:
	low bits in screen byte = leftmost pixel
	low bits in attr byte   = leftmost attr / color with lower index in attr
*/

#include "ScanlineRenderer.h"
#include "Interp.h"
#include "basic_math.h"
#include <hardware/gpio.h>
#include <hardware/interp.h>
#include <pico/stdio.h>


#ifndef VIDEO_INTERP0_MODE
  #define VIDEO_INTERP0_MODE ip_any
#endif

#ifndef VIDEO_INTERP1_MODE
  #define VIDEO_INTERP1_MODE ip_a1w8
#endif

#ifndef VIDEO_OPTIMISTIC_A1W8
  #define VIDEO_OPTIMISTIC_A1W8 OFF
#endif

// if 200x150 or 400x300 are not supported but displayed, this causes a bus error!
#ifndef VIDEO_SUPPORT_200x150_A1W8
  #define VIDEO_SUPPORT_200x150_A1W8 true
#endif
#ifndef VIDEO_SUPPORT_400x300_A1W8
  #define VIDEO_SUPPORT_400x300_A1W8 true
#endif


// silence warnings:
// clang-format off
#define interp_hw_array ((interp_hw_t *)(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET))
// clang-format on
#undef interp_hw_array
#define interp_hw_array reinterpret_cast<Interp*>(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET)

// all hot video code should go into ram to allow video while flash lockout.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define XRAM __attribute__((section(".scratch_x.SRFu" __XSTRING(__LINE__))))	 // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.SRFu" __XSTRING(__LINE__)))) // general ram


// ============================================================================================

namespace kio::Video
{

using namespace Graphics;


enum InterpMode {
	ip_any	= -1,
	ip_1bpp = 0,
	ip_2bpp = 1,
	ip_4bpp = 2,
	ip_8bpp = 3,
	ip_a1w8 = 0b101,

	ip0_mode = VIDEO_INTERP0_MODE,
	ip1_mode = VIDEO_INTERP1_MODE,
};

static constexpr uint ss_color	   = msbit(sizeof(Color));
static constexpr uint ss_twocolors = ss_color + 1;
constexpr InterpMode  ip_modes[2]  = {ip0_mode, ip1_mode};

// clang-format off
template<uint sz> struct uint_with_size{using type=uint8;};
template<> struct uint_with_size<2>{using type=uint16;};
template<> struct uint_with_size<4>{using type=uint32;};
template<> struct uint_with_size<8>{using type=uint64;};
// clang-format on

using onecolor	 = uint_with_size<sizeof(Color) * 1>::type;
using twocolors	 = uint_with_size<sizeof(Color) * 2>::type;
using fourcolors = uint_with_size<sizeof(Color) * 4>::type;

static_assert(SIO_INTERP1_ACCUM0_OFFSET - SIO_INTERP0_ACCUM0_OFFSET == sizeof(Interp));


// ============================================================================================

template<InterpMode mode>
static constexpr int ipi = ip1_mode == mode || (ip0_mode != mode && ip1_mode == ip_any);

template<InterpMode mode>
constexpr bool need_setup = (ip_modes[ipi<mode>]) != mode;

template<InterpMode mode>
constexpr bool need_cleanup = need_setup<mode> && ip_modes[ipi<mode>] != ip_any;

template<InterpMode mode>
static __force_inline void setup(Interp* ip) noexcept //
{
	ip->setup(1 << (mode & 3), ss_color + uint(mode >> 2));
}

template<InterpMode mode>
static __force_inline void setup_if_needed() noexcept
{
	if constexpr (need_setup<mode>) setup<mode>(&interp0[ipi<mode>]);
}

template<InterpMode mode>
static __force_inline void cleanup_if_needed() noexcept
{
	if constexpr (need_cleanup<mode>) setup<ip_modes[ipi<mode>]>(&interp0[ipi<mode>]);
}

void initializeInterpolators() noexcept
{
	assert(get_core_num() == 1);
	constexpr uint lane0 = 0;

	interp0->base[lane0] = 0; // interp0.lane0: add nothing
	if constexpr (ip0_mode != ip_any) setup<ip0_mode>(interp0);

	interp1->base[lane0] = 0; // interp1.lane0: add nothing
	if constexpr (ip1_mode != ip_any) setup<ip1_mode>(interp1);
}


// ============================================================================================
// 1-bit indexed color mode:
// this version uses no interp but a pre-computed 4k colormap.

ScanlineRenderer_i1::ScanlineRenderer_i1(const Color* colormap_in) noexcept
{
	// for all values for bytes from pixmap
	// which contain 8 pixels
	// create corresponding stripe of 8 colors:

	Color* p = colormap;
	for (uint byte = 0; byte < 256; byte++)
	{
		for (uint bit = 0; bit < 8; bit++) { *p++ = colormap_in[(byte >> bit) & 1]; }
	}
}

void XRAM ScanlineRenderer_i1::render(uint32* dest, uint width, const uint8* pixels) noexcept
{
	const twocolors* colors = reinterpret_cast<const twocolors*>(colormap);

	for (uint i = 0; i < width / 8; i++)
	{
		uint byte = *pixels++ * 4;
		*dest++	  = colors[byte];
		*dest++	  = colors[byte + 1];
		*dest++	  = colors[byte + 2];
		*dest++	  = colors[byte + 3];
	}
}


// ============================================================================================
// 2-bit indexed color mode:
// this version uses no interp but a pre-computed 2k colormap.

ScanlineRenderer_i2::ScanlineRenderer_i2(const Color* colormap_in) noexcept
{
	// for all values for bytes from pixmap
	// which contain 4 pixels
	// create corresponding stripe of 4 colors:

	Color* p = colormap;
	for (uint byte = 0; byte < 256; byte++)
	{
		for (uint bit = 0; bit < 8; bit += 2) { *p++ = colormap_in[(byte >> bit) & 3]; }
	}
}

void XRAM ScanlineRenderer_i2::render(uint32* dest, uint width, const uint8* pixels) noexcept
{
	const twocolors* colors = reinterpret_cast<const twocolors*>(colormap);

	for (uint i = 0; i < width / 8; i++)
	{
		uint byte = *pixels++ * 2;
		*dest++	  = colors[byte];
		*dest++	  = colors[byte + 1];

		byte	= *pixels++ * 2;
		*dest++ = colors[byte];
		*dest++ = colors[byte + 1];
	}
}


// ============================================================================================
// 4-bit indexed color mode:

void XRAM ScanlineRenderer_i4::render(uint32* _dest, uint width, const uint8* _pixels) noexcept
{
	constexpr InterpMode ip = ip_4bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];
	interp->set_color_base(colormap);

	onecolor*	  dest	 = reinterpret_cast<onecolor*>(_dest);
	const uint16* pixels = reinterpret_cast<const uint16*>(_pixels);

	for (uint i = 0; i < width / 4; i++)
	{
		interp->set_pixels(*pixels++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// 8-bit indexed color mode:

void XRAM ScanlineRenderer_i8::render(uint32* _dest, uint width, const uint8* _pixels) noexcept
{
	constexpr InterpMode ip = ip_8bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];
	interp->set_color_base(colormap);

	Color*		  dest	 = reinterpret_cast<Color*>(_dest);
	const uint16* pixels = reinterpret_cast<const uint16*>(_pixels);

	for (uint i = 0; i < width / 4; i++)
	{
		interp->set_pixels(*pixels++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();

		interp->set_pixels(*pixels++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// true color mode:

void XRAM ScanlineRenderer_rgb(uint32* dest, uint width, const uint8* q) noexcept
{
	volatile uint32* z = dest;

	if constexpr (colordepth_rgb == colordepth_1bpp) // 1 bit b&w video. TODO: similar to i1
	{
		for (uint n = 0; n < width / 8; n++)
		{
			uint pixels = *q++;

			uint32 fourpixels = pixels & 1;
			fourpixels += (pixels & 2) << 7;
			fourpixels += (pixels & 4) << 14;
			fourpixels += (pixels & 8) << 21;
			*z++ = fourpixels;

			pixels >>= 4;
			fourpixels = pixels & 1;
			fourpixels += (pixels & 2) << 7;
			fourpixels += (pixels & 4) << 14;
			fourpixels += (pixels & 8) << 21;
			*z++ = fourpixels;
		}
	}
	if constexpr (colordepth_rgb == colordepth_2bpp) // 2 bit greyscale video. TODO: similar to i2
	{
		for (uint n = 0; n < width / 4; n++)
		{
			uint   pixels	  = *q++;
			uint32 fourpixels = pixels & 3;
			fourpixels += (pixels & 0x0c) << 6;
			fourpixels += (pixels & 0x30) << 12;
			fourpixels += (pixels & 0xc0) << 18;
			*z++ = fourpixels;
		}
	}
	if constexpr (colordepth_rgb == colordepth_4bpp) // 4 bit color output. TODO: similar to i1, i2
	{
		assert((size_t(q) & 1) == 0);
		const uint16* qq = reinterpret_cast<const uint16*>(q);

		for (uint n = 0; n < width / 4; n++)
		{
			uint   pixels	  = *qq++;
			uint32 fourpixels = pixels & 0x000f;
			fourpixels += (pixels & 0x00f0) << 4;
			fourpixels += (pixels & 0x0f00) << 8;
			fourpixels += (pixels & 0xf000) << 12;
			*z++ = fourpixels;
		}
	}
	if constexpr (colordepth_rgb >= colordepth_8bpp) // 8 or 16 bit color output
	{
		assert((size_t(q) & 3) == 0);
		const volatile uint32* qq = reinterpret_cast<const uint32*>(q);

		width = width * sizeof(Color) / sizeof(uint32);
		for (uint n = 0; n < width; n++) z[n] = qq[n];
	}
}


// ============================================================================================
// attribute mode with 1 bit/pixel with 1 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a1w1>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attr) noexcept
{
	constexpr InterpMode ip = ip_1bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];

	onecolor*		 dest		= reinterpret_cast<onecolor*>(_dest);
	const twocolors* attributes = reinterpret_cast<const twocolors*>(_attr);

	for (uint i = 0; i < width / 8; i++)
	{
		interp->set_pixels(*pixels++);

		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// attribute mode with 1 bit/pixel with 2 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a1w2>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attr) noexcept
{
	constexpr InterpMode ip = ip_1bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];

	onecolor*		 dest		= reinterpret_cast<onecolor*>(_dest);
	const twocolors* attributes = reinterpret_cast<const twocolors*>(_attr);

	for (uint i = 0; i < width / 8; i++)
	{
		interp->set_pixels(*pixels++);

		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// attribute mode with 1 bit/pixel with 4 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a1w4>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attr) noexcept
{
	constexpr InterpMode ip = ip_1bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];

	onecolor*		 dest		= reinterpret_cast<onecolor*>(_dest);
	const twocolors* attributes = reinterpret_cast<const twocolors*>(_attr);

	for (uint i = 0; i < width / 8; i++)
	{
		interp->set_pixels(*pixels++);

		interp->set_color_base(uint32(attributes++));
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();

		interp->set_color_base(uint32(attributes++));
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// attribute mode with 1 bit/pixel with 8 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a1w8>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attr) noexcept
{
	// 2023-10-27
	// this version displays 1024x768 with avg/max load = 247.1/259.3MHz

	constexpr InterpMode ip		= ip_a1w8;
	Interp* const		 interp = &interp0[ipi<ip>];
	setup_if_needed<ip>();

	constexpr int ssx = sizeof(Color) * CHAR_BIT; // shift distance for swapping colors

	twocolors ctable[4];
	interp->set_color_base(ctable);

	if (!(VIDEO_SUPPORT_200x150_A1W8 || VIDEO_SUPPORT_400x300_A1W8) || (width & 31) == 0)
	{
		twocolors*		 dest		= reinterpret_cast<twocolors*>(_dest);
		const uint32*	 pixels		= reinterpret_cast<const uint32*>(_pixels);
		const twocolors* attributes = reinterpret_cast<const twocolors*>(_attr);
		constexpr uint	 lane0		= 0;

		twocolors color10a;
		twocolors color10b = 0;

		for (uint i = 0; i < width / 32; i++)
		{
			color10a = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8 || color10a != color10b)
			{
				twocolors color01 = (color10a >> ssx) | twocolors(color10a << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10a);
				ctable[1]		  = color01;
				ctable[2]		  = color10a;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10a ^ xxx;
			}

			uint32 bits = *pixels++;
			interp->set_accumulator(lane0, bits >> (2 - ss_twocolors));

			*dest++ = ctable[bits & 3];
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();

			color10b = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8 || color10a != color10b)
			{
				twocolors color01 = (color10b >> ssx) | twocolors(color10b << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10b);
				ctable[1]		  = color01;
				ctable[2]		  = color10b;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10b ^ xxx;
			}

			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();

			color10a = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8 || color10a != color10b)
			{
				twocolors color01 = (color10a >> ssx) | twocolors(color10a << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10a);
				ctable[1]		  = color01;
				ctable[2]		  = color10a;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10a ^ xxx;
			}

			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();

			color10b = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8 || color10a != color10b)
			{
				twocolors color01 = (color10b >> ssx) | twocolors(color10b << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10b);
				ctable[1]		  = color01;
				ctable[2]		  = color10b;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10b ^ xxx;
			}

			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
		}
	}
	else if constexpr (VIDEO_SUPPORT_200x150_A1W8)
	{
		// 200*150: 200 = 8 * 25 => odd!

		twocolors*		 dest		= reinterpret_cast<twocolors*>(_dest);
		const uint8*	 pixels		= reinterpret_cast<const uint8*>(_pixels);
		const twocolors* attributes = reinterpret_cast<const twocolors*>(_attr);

		for (uint i = 0; i < width / 8; i++)
		{
			interp->set_pixels(*pixels++, ss_twocolors);

			{
				twocolors color10 = *attributes++;
				twocolors color01 = (color10 >> ssx) | twocolors(color10 << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10);
				ctable[1]		  = color01;
				ctable[2]		  = color10;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10 ^ xxx;
			}

			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
		}
	}
	else if constexpr (VIDEO_SUPPORT_400x300_A1W8)
	{
		// 400*300: 400 = 32 * 12.5 => not a multiple of 32!

		twocolors*		 dest		= reinterpret_cast<twocolors*>(_dest);
		const uint16*	 pixels		= reinterpret_cast<const uint16*>(_pixels);
		const twocolors* attributes = reinterpret_cast<const twocolors*>(_attr);

		for (uint i = 0; i < width / 16; i++)
		{
			interp->set_pixels(*pixels++, ss_twocolors);

			{
				twocolors color10 = *attributes++;
				twocolors color01 = (color10 >> ssx) | twocolors(color10 << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10);
				ctable[1]		  = color01;
				ctable[2]		  = color10;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10 ^ xxx;
			}

			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();

			{
				twocolors color10 = *attributes++;
				twocolors color01 = (color10 >> ssx) | twocolors(color10 << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10);
				ctable[1]		  = color01;
				ctable[2]		  = color10;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10 ^ xxx;
			}

			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
			*dest++ = *interp->next_color<twocolors>();
		}
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// attribute mode with 2 bit/pixel with 1 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a2w1>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attr) noexcept
{
	constexpr InterpMode ip = ip_2bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];

	onecolor*		  dest		 = reinterpret_cast<onecolor*>(_dest);
	const fourcolors* attributes = reinterpret_cast<const fourcolors*>(_attr);
	const uint16*	  pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp->set_pixels(*pixels++);

		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// attribute mode with 2 bit/pixel with 2 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a2w2>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attr) noexcept
{
	constexpr InterpMode ip = ip_2bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];

	onecolor*		  dest		 = reinterpret_cast<onecolor*>(_dest);
	const fourcolors* attributes = reinterpret_cast<const fourcolors*>(_attr);
	const uint16*	  pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp->set_pixels(*pixels++);

		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// attribute mode with 2 bit/pixel with 4 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a2w4>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attr) noexcept
{
	constexpr InterpMode ip = ip_2bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];

	onecolor*		  dest		 = reinterpret_cast<onecolor*>(_dest);
	const fourcolors* attributes = reinterpret_cast<const fourcolors*>(_attr);
	const uint16*	  pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp->set_pixels(*pixels++);

		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();

		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}


// ============================================================================================
// attribute mode with 2 bit/pixel with 8 pixel wide attributes and true colors:

template<>
void XRAM ScanlineRenderer<colormode_a2w8>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attr) noexcept
{
	constexpr InterpMode ip = ip_2bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];

	onecolor*		  dest		 = reinterpret_cast<onecolor*>(_dest);
	const fourcolors* attributes = reinterpret_cast<const fourcolors*>(_attr);
	const uint16*	  pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp->set_pixels(*pixels++);

		interp->set_color_base(attributes++);
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
		*dest++ = *interp->next_color();
	}

	cleanup_if_needed<ip>();
}

// ============================================================================================
// special 8-bit indexed color mode for Hold-and-Modify image:

static __force_inline Color operator+(Color a, Color b) { return Color(a.raw + b.raw); }

void XRAM HamImageScanlineRenderer::render(uint32* framebuffer, uint width, const uint8* _pixels) noexcept
{
	constexpr InterpMode ip = ip_8bpp;
	setup_if_needed<ip>();
	Interp* const interp = &interp0[ipi<ip>];
	interp->set_color_base(colormap);

	// we don't check the row
	// we rely on do_vblank() to reset the pointer
	// and if we actually miss a scanline then let it be

	const Color*  first_rel_color = &colormap[first_rel_code];
	Color		  current_color	  = first_color;
	const uint16* pixels		  = reinterpret_cast<const uint16*>(_pixels);

	Color* dest		   = reinterpret_cast<Color*>(framebuffer);
	Color* first_pixel = dest;

	for (uint i = 0; i < width / 4; i++)
	{
		const Color* color;

		interp->set_pixels(*pixels++);
		color	= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;
		color					= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;

		interp->set_pixels(*pixels++);
		color	= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;
		color					= interp->next_color();
		*dest++ = current_color = color >= first_rel_color ? current_color + *color : *color;
	}

	first_color = *first_pixel;
	cleanup_if_needed<ip>();
}

} // namespace kio::Video


/*

































*/
