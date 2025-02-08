// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

/*
	This file provides Scanline Renderers which render one scanline in their respective Color Mode.

	note:
	low bits in screen byte = leftmost pixel
	low bits in attr byte   = leftmost attr / color with lower index in attr
*/

#include "ScanlineRenderFu.h"
#include "basic_math.h"
#include <hardware/interp.h>
#include <string.h>

// silence warnings:
// clang-format off
#define interp_hw_array ((interp_hw_t *)(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET))
// clang-format on
#undef interp_hw_array
#define interp_hw_array reinterpret_cast<interp_hw_t*>(SIO_BASE + SIO_INTERP0_ACCUM0_OFFSET)


// all hot video code should go into ram to allow video while flash lockout.
// also, there should be no const data accessed in hot video code for the same reason.
// the most timecritical things should go into core1 stack page because it is not contended.

#define WRAP(X)	 #X
#define XWRAP(X) WRAP(X)
#define XRAM	 __attribute__((section(".scratch_x.SRFu" XWRAP(__LINE__))))	 // the 4k page with the core1 stack
#define RAM		 __attribute__((section(".time_critical.SRFu" XWRAP(__LINE__)))) // general ram


// ============================================================================================

namespace kio::Video
{

using namespace Graphics;

constexpr uint lane0 = 0;
constexpr uint lane1 = 1;

// clang-format off
template<uint sz> struct uint_with_size{using type=uint8;};
template<> struct uint_with_size<2>{using type=uint16;};
template<> struct uint_with_size<4>{using type=uint32;};
// clang-format on

using onecolor	= uint_with_size<sizeof(Color)>::type;
using twocolors = uint_with_size<sizeof(Color) * 2>::type;


static XRAM Color* video_colormap; // for old (non-interp) scanline renderers
static XRAM Color  temp_colors[4]; // for scanline renderers with attribute modes


/*  the interpolators are used for look-ups like
		value = table[byte&mask];
		byte >>= shift;

	there are 4 use case classes to find the vga color of a pixel in a VideoPlane function:

	direct true color:

		vgacolor = *screenbytes++;									// no interp used


	direct indexed color:

		vgacolor = colormap[screenbyte & colormask]; screenbyte >> colorbits;	// interp0


	true color attribute:

		*colors  = attributes; attributes += 2;

		vgacolor = colors[screenbyte & 1]; screenbyte >> 1;	or		// interp1
		vgacolor = colors[screenbyte & 3]; screenbyte >> 2;			// interp1

	indexed color attributes:

		temp_colors[0] = colormap[attrbyte & colormask]; attrbyte >> colorbits;	// interp0
		temp_colors[1] = colormap[attrbyte & colormask]; attrbyte >> colorbits;	// interp0

		vgacolor = temp_colors[screenbyte & 1]; screenbyte >> 1;	// interp1
*/
static void setup_interp(interp_hw_t* interp, const Color* colormap, uint bits) noexcept
{
	// function:  address_out = &colormap[pixels_in & mask];
	//			  pixels_in >> bits;

	constexpr uint ss = msbit(sizeof(onecolor));
	static_assert(msbit(sizeof(uint8)) == 0);
	static_assert(msbit(sizeof(uint16)) == 1);

	interp_config cfg = interp_default_config(); // configure lane0
	interp_config_set_shift(&cfg, bits);		 // shift right by 1 .. 8 bit
	interp_set_config(interp, lane0, &cfg);

	cfg = interp_default_config();					 // configure lane1
	interp_config_set_cross_input(&cfg, true);		 // read from accu lane0
	interp_config_set_mask(&cfg, ss, ss + bits - 1); // mask to select index bits
	interp_set_config(interp, lane1, &cfg);

	interp_set_base(interp, lane0, 0);				  // lane0: add nothing
	interp_set_base(interp, lane1, uint32(colormap)); // lane1: add color table base
}

template<ColorMode CM>
void setupScanlineRenderer(const Color* colormap)
{
	assert(get_core_num() == 1);

	// setup interp0 if indexed color:
	if (is_indexed_color(CM)) { setup_interp(interp0, colormap, 1 << get_colordepth(CM)); }

	// setup interp1 if attribute mode:
	if (is_attribute_mode(CM)) { setup_interp(interp1, temp_colors, 1 << get_attrmode(CM)); }
}

template<ColorMode CM>
void teardownScanlineRenderer() noexcept
{
	// teardown interp0:
	if (is_indexed_color(CM)) {}

	// teardown interp1:
	if (is_attribute_mode(CM)) {}
}

static inline const Color* next_color(interp_hw_t* interp)
{
	return reinterpret_cast<const Color*>(interp_pop_lane_result(interp, lane1));
}


// ============================================================================================
// direct 1-bit indexed color mode:
// this version uses no interp but a pre-computed 4k colormap.

template<>
void XRAM scanlineRenderFunction<colormode_i1>(uint32* dest, uint width, const uint8* pixels)
{
	const twocolors* colors = reinterpret_cast<const twocolors*>(video_colormap);

	for (uint i = 0; i < width / 8; i++)
	{
		uint byte = *pixels++ * 4;
		*dest++	  = colors[byte];
		*dest++	  = colors[byte + 1];
		*dest++	  = colors[byte + 2];
		*dest++	  = colors[byte + 3];
	}
}

template<>
void setupScanlineRenderer<colormode_i1>(const Color* colormap)
{
	assert(video_colormap == nullptr);
	video_colormap = new Color[256 * 8];

	// for all values for bytes from pixmap
	// which contain 8 pixels
	// create corresponding stripe of 8 colors:

	Color* p = video_colormap;
	for (uint byte = 0; byte < 256; byte++)
	{
		for (uint bit = 0; bit < 8; bit++) { *p++ = colormap[(byte >> bit) & 1]; }
	}
}

template<>
void teardownScanlineRenderer<colormode_i1>() noexcept
{
	delete[] video_colormap;
	video_colormap = nullptr;
}

// ============================================================================================
// direct 2-bit indexed color mode:
// this version uses no interp but a pre-computed 2k colormap.

template<>
void XRAM scanlineRenderFunction<colormode_i2>(uint32* dest, uint width, const uint8* pixels)
{
	const twocolors* colors = reinterpret_cast<const twocolors*>(video_colormap);

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

template<>
void setupScanlineRenderer<colormode_i2>(const Color* colormap)
{
	assert(video_colormap == nullptr);
	video_colormap = new Color[256 * 4];

	// for all values for bytes from pixmap
	// which contain 4 pixels
	// create corresponding stripe of 4 colors:

	Color* p = video_colormap;
	for (uint byte = 0; byte < 256; byte++)
	{
		for (uint bit = 0; bit < 8; bit += 2) { *p++ = colormap[(byte >> bit) & 3]; }
	}
}

template<>
void teardownScanlineRenderer<colormode_i2>() noexcept
{
	delete[] video_colormap;
	video_colormap = nullptr;
}

// ============================================================================================
// direct 4-bit indexed color mode:

template<>
void XRAM scanlineRenderFunction<colormode_i4>(uint32* _dest, uint width, const uint8* _pixels)
{
	onecolor*	  dest	 = reinterpret_cast<onecolor*>(_dest);
	const uint16* pixels = reinterpret_cast<const uint16*>(_pixels);

	for (uint i = 0; i < width / 4; i++)
	{
		interp_set_accumulator(interp0, lane0, uint(*pixels++) << 1);
		*dest++ = *next_color(interp0);
		*dest++ = *next_color(interp0);
		*dest++ = *next_color(interp0);
		*dest++ = *next_color(interp0);
	}
}

template void setupScanlineRenderer<colormode_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_i4>() noexcept;

// ============================================================================================
// direct 8-bit indexed color mode:

template<>
void XRAM scanlineRenderFunction<colormode_i8>(uint32* _dest, uint width, const uint8* _pixels)
{
	if constexpr (sizeof(Color) == 1)
	{
		memcpy(_dest, _pixels, width * sizeof(Color)); //
	}
	else
	{
		uint16*		  dest	 = reinterpret_cast<uint16*>(_dest);
		const uint16* pixels = reinterpret_cast<const uint16*>(_pixels);

		for (uint i = 0; i < width / 4; i++)
		{
			interp_set_accumulator(interp0, lane0, uint(*pixels++) << 1);
			*dest++ = *next_color(interp0);
			*dest++ = *next_color(interp0);

			interp_set_accumulator(interp0, lane0, uint(*pixels++) << 1);
			*dest++ = *next_color(interp0);
			*dest++ = *next_color(interp0);
		}
	}
}

template void setupScanlineRenderer<colormode_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_i8>() noexcept;

// ============================================================================================
// direct true color mode:
// this version copies the pixels.
// a VideoPlane which uses nested dma should be implemented separately.

template<>
void RAM scanlineRenderFunction<colormode_i16>(uint32* dest, uint width, const uint8* pixels)
{
	//memcpy(dest, pixels, width * sizeof(Color));

	//assert((width & 1) == 0);

	volatile uint32* z = dest;
	for (uint n = 0; n < width / 2; n++) z[n] = pixels[n];
}

template<>
void setupScanlineRenderer<colormode_i16>(const Color*)
{}

template<>
void teardownScanlineRenderer<colormode_i16>() noexcept
{}

// ============================================================================================
// attribute mode with 1 bit/pixel with 1 pixel wide attributes and 4-bit indexed colors:

template<>
void RAM
scanlineRenderFunction<colormode_a1w1_i4>(uint32* _dest, uint width, const uint8* pixels, const uint8* attributes)
{
	onecolor* dest = reinterpret_cast<onecolor*>(_dest);

	constexpr uint bits_per_color = 4;
	constexpr uint colormask	  = (1 << bits_per_color) - 1;

	for (uint i = 0; i < width / 8; i++)
	{
		uint byte = *pixels++;
		uint attr;

		attr	= *attributes++;
		*dest++ = video_colormap[byte & 1 ? attr >> bits_per_color : attr & colormask];
		byte >>= 1;
		attr	= *attributes++;
		*dest++ = video_colormap[byte & 1 ? attr >> bits_per_color : attr & colormask];
		byte >>= 1;
		attr	= *attributes++;
		*dest++ = video_colormap[byte & 1 ? attr >> bits_per_color : attr & colormask];
		byte >>= 1;
		attr	= *attributes++;
		*dest++ = video_colormap[byte & 1 ? attr >> bits_per_color : attr & colormask];
		byte >>= 1;
		attr	= *attributes++;
		*dest++ = video_colormap[byte & 1 ? attr >> bits_per_color : attr & colormask];
		byte >>= 1;
		attr	= *attributes++;
		*dest++ = video_colormap[byte & 1 ? attr >> bits_per_color : attr & colormask];
		byte >>= 1;
		attr	= *attributes++;
		*dest++ = video_colormap[byte & 1 ? attr >> bits_per_color : attr & colormask];
		byte >>= 1;
		attr	= *attributes++;
		*dest++ = video_colormap[byte ? attr >> bits_per_color : attr & colormask];
	}
}

template void setupScanlineRenderer<colormode_a1w1_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w1_i4>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 1 pixel wide attributes and 8-bit indexed colors:

static constexpr ColorMode colormode_a1w1_ic8 = sizeof(Color) == 1 ? ColorMode(98) : colormode_a1w1_i8;

template<>
void RAM
scanlineRenderFunction<colormode_a1w1_ic8>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	onecolor* dest = reinterpret_cast<onecolor*>(_dest);

	assert(sizeof(Color) == 2);

	const uint8* attributes = _attributes;

	for (uint i = 0; i < width / 8; i++)
	{
		uint byte = *pixels++;
		*dest++	  = video_colormap[attributes[byte & 1]];
		byte >>= 1;
		attributes += 2;
		*dest++ = video_colormap[attributes[byte & 1]];
		byte >>= 1;
		attributes += 2;
		*dest++ = video_colormap[attributes[byte & 1]];
		byte >>= 1;
		attributes += 2;
		*dest++ = video_colormap[attributes[byte & 1]];
		byte >>= 1;
		attributes += 2;
		*dest++ = video_colormap[attributes[byte & 1]];
		byte >>= 1;
		attributes += 2;
		*dest++ = video_colormap[attributes[byte & 1]];
		byte >>= 1;
		attributes += 2;
		*dest++ = video_colormap[attributes[byte & 1]];
		byte >>= 1;
		attributes += 2;
		*dest++ = video_colormap[attributes[byte]];
		attributes += 2;
	}
}

template void setupScanlineRenderer<colormode_a1w1_ic8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w1_ic8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 1 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w1_rgb>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	onecolor*		 dest		= reinterpret_cast<onecolor*>(_dest);
	const twocolors* attributes = reinterpret_cast<const twocolors*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w1_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w1_rgb>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 2 pixel wide attributes and 4-bit indexed colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w2_i4>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w2_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w2_i4>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 2 pixel wide attributes and 8-bit indexed colors:

static constexpr ColorMode colormode_a1w2_ic8 = sizeof(Color) == 1 ? ColorMode(97) : colormode_a1w2_i8;

template<>
void XRAM
scanlineRenderFunction<colormode_a1w2_ic8>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	assert(sizeof(Color) == 2);

	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w2_ic8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w2_ic8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 2 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w2_rgb>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	onecolor*		 dest		= reinterpret_cast<onecolor*>(_dest);
	const twocolors* attributes = reinterpret_cast<const twocolors*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w2_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w2_rgb>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 4 pixel wide attributes and 4-bit indexed colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w4_i4>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);
		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);

		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w4_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w4_i4>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 4 pixel wide attributes and 8-bit indexed colors:

static constexpr ColorMode colormode_a1w4_ic8 = sizeof(Color) == 1 ? ColorMode(96) : colormode_a1w4_i8;

template<>
void XRAM
scanlineRenderFunction<colormode_a1w4_ic8>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	assert(sizeof(Color) == 2);

	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w4_ic8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w4_ic8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 4 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w4_rgb>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	onecolor*		 dest		= reinterpret_cast<onecolor*>(_dest);
	const twocolors* attributes = reinterpret_cast<const twocolors*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w4_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w4_rgb>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 8 pixel wide attributes and 4-bit indexed colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w8_i4>(uint32* _dest, uint width, const uint8* pixels, const uint8* attributes)
{
	onecolor* dest = reinterpret_cast<onecolor*>(_dest);

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);

		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w8_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w8_i4>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 8 pixel wide attributes and 8-bit indexed colors:

static constexpr ColorMode colormode_a1w8_ic8 = sizeof(Color) == 1 ? ColorMode(99) : colormode_a1w8_i8;

template<>
void XRAM
scanlineRenderFunction<colormode_a1w8_ic8>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	assert(sizeof(Color) == 2);

	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes);

	for (uint i = 0; i < width / 8; i++)
	{
		if constexpr ((1))
		{
			interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
			temp_colors[0] = *next_color(interp0);
			temp_colors[1] = *next_color(interp0);
		}
		else
		{
			temp_colors[0] = video_colormap[*_attributes++];
			temp_colors[1] = video_colormap[*_attributes++];
		}

		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a1w8_ic8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w8_ic8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 8 pixel wide attributes and true colors:

template<>
XRAM void
scanlineRenderFunction<colormode_a1w8_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	// 2023-10-27
	// this version displays 1024x768 with avg/max load = 247.1/259.3MHz

#ifndef VIDEO_OPTIMISTIC_A1W8_RGB
  #define VIDEO_OPTIMISTIC_A1W8_RGB OFF
#endif

	// if 200x150 or 400x300 are not supported but displayed, this causes a bus error!
#ifndef VIDEO_SUPPORT_200x150_A1W8_RGB
  #define VIDEO_SUPPORT_200x150_A1W8_RGB true
#endif
#ifndef VIDEO_SUPPORT_400x300_A1W8_RGB
  #define VIDEO_SUPPORT_400x300_A1W8_RGB true
#endif

	constexpr int ssx = sizeof(Color) * 8;		  // shift distance for swapping colors
	constexpr int ss  = msbit(sizeof(twocolors)); // address shift for index in `ctable[]`

	twocolors ctable[4];
	interp_set_base(interp1, lane1, uint32(ctable));

	if (!(VIDEO_SUPPORT_200x150_A1W8_RGB || VIDEO_SUPPORT_400x300_A1W8_RGB) || (width & 31) == 0)
	{
		twocolors*		 dest		= reinterpret_cast<twocolors*>(_dest);
		const uint32*	 pixels		= reinterpret_cast<const uint32*>(_pixels);
		const twocolors* attributes = reinterpret_cast<const twocolors*>(_attributes);

		twocolors color10a;
		twocolors color10b = 0;

		for (uint i = 0; i < width / 32; i++)
		{
			color10a = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8_RGB || color10a != color10b)
			{
				twocolors color01 = (color10a >> ssx) | twocolors(color10a << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10a);
				ctable[1]		  = color01;
				ctable[2]		  = color10a;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10a ^ xxx;
			}

			uint32 bits = *pixels++;
			interp_set_accumulator(interp1, lane0, bits >> (2 - ss));

			*dest++ = ctable[bits & 3];
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));

			color10b = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8_RGB || color10a != color10b)
			{
				twocolors color01 = (color10b >> ssx) | twocolors(color10b << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10b);
				ctable[1]		  = color01;
				ctable[2]		  = color10b;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10b ^ xxx;
			}

			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));

			color10a = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8_RGB || color10a != color10b)
			{
				twocolors color01 = (color10a >> ssx) | twocolors(color10a << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10a);
				ctable[1]		  = color01;
				ctable[2]		  = color10a;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10a ^ xxx;
			}

			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));

			color10b = *attributes++;
			if (!VIDEO_OPTIMISTIC_A1W8_RGB || color10a != color10b)
			{
				twocolors color01 = (color10b >> ssx) | twocolors(color10b << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10b);
				ctable[1]		  = color01;
				ctable[2]		  = color10b;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10b ^ xxx;
			}

			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
		}
	}
	else if constexpr (VIDEO_SUPPORT_200x150_A1W8_RGB)
	{
		// 200*150: 200 = 8 * 25 => odd!
		// 0x154 bytes in scratch_x
		// avg/max load = 248.8/251.4MHz

		twocolors*		 dest		= reinterpret_cast<twocolors*>(_dest);
		const uint8*	 pixels		= reinterpret_cast<const uint8*>(_pixels);
		const twocolors* attributes = reinterpret_cast<const twocolors*>(_attributes);

		for (uint i = 0; i < width / 8; i++)
		{
			interp_set_accumulator(interp1, lane0, uint(*pixels++) << ss);

			{
				twocolors color10 = *attributes++;
				twocolors color01 = (color10 >> ssx) | twocolors(color10 << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10);
				ctable[1]		  = color01;
				ctable[2]		  = color10;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10 ^ xxx;
			}

			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
		}
	}
	else if constexpr (VIDEO_SUPPORT_400x300_A1W8_RGB)
	{
		// 400*300: 400 = 32 * 12.5 => not a multiple of 32!
		// 0x184 bytes in scratch_x
		// avg/max load = 244.1/246.4MHz

		twocolors*		 dest		= reinterpret_cast<twocolors*>(_dest);
		const uint16*	 pixels		= reinterpret_cast<const uint16*>(_pixels);
		const twocolors* attributes = reinterpret_cast<const twocolors*>(_attributes);

		for (uint i = 0; i < width / 16; i++)
		{
			interp_set_accumulator(interp1, lane0, uint(*pixels++) << ss);

			{
				twocolors color10 = *attributes++;
				twocolors color01 = (color10 >> ssx) | twocolors(color10 << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10);
				ctable[1]		  = color01;
				ctable[2]		  = color10;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10 ^ xxx;
			}

			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));

			{
				twocolors color10 = *attributes++;
				twocolors color01 = (color10 >> ssx) | twocolors(color10 << ssx);
				twocolors xxx	  = onecolor(color01 ^ color10);
				ctable[1]		  = color01;
				ctable[2]		  = color10;
				ctable[0]		  = color01 ^ xxx;
				ctable[3]		  = color10 ^ xxx;
			}

			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const twocolors*>(interp_pop_lane_result(interp1, lane1));
		}
	}
}

template<>
void setupScanlineRenderer<colormode_a1w8_rgb>(const Color* /*colormap*/)
{
	assert(get_core_num() == 1);

	// function:  address_out = &colormap[pixels_in & 3];
	//			  pixels_in >> 2;

	// address shift for index in `twocolors ctable[]`
	constexpr int ss = msbit(sizeof(twocolors));

	// setup interp1:
	interp_config cfg = interp_default_config(); // configure lane0
	interp_config_set_shift(&cfg, 2);			 // shift right by 2 bits
	interp_set_config(interp1, lane0, &cfg);

	cfg = interp_default_config();			   // configure lane1
	interp_config_set_cross_input(&cfg, true); // read from accu lane0
	interp_config_set_mask(&cfg, ss, ss + 1);  // mask bits used for index into ctable[]
	interp_set_config(interp1, lane1, &cfg);

	interp_set_base(interp1, lane0, 0); // lane0: add nothing
	//interp_set_base(interp1, lane1, uint32(temp_colors)); // lane1: add base of temp_colors[]
}

template void teardownScanlineRenderer<colormode_a1w8_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 1 pixel wide attributes and 4-bit indexed color:

// NOT SUPPORTED

// ============================================================================================
// attribute mode with 2 bit/pixel with 1 pixel wide attributes and 8-bit indexed color:

// NOT SUPPORTED

// ============================================================================================
// attribute mode with 2 bit/pixel with 1 pixel wide attributes and true colors:

template<>
void RAM
scanlineRenderFunction<colormode_a2w1_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel
	constexpr int ss		 = msbit(sizeof(onecolor));

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << ss);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w1_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w1_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 2 pixel wide attributes and 4-bit indexed color:

// NOT SUPPORTED

// ============================================================================================
// attribute mode with 2 bit/pixel with 2 pixel wide attributes and 8-bit indexed color:

// NOT SUPPORTED

// ============================================================================================
// attribute mode with 2 bit/pixel with 2 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w2_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel
	constexpr int ss		 = msbit(sizeof(onecolor));

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << ss);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w2_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w2_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 4 pixel wide attributes and 4-bit indexed colors:

template<>
void RAM
scanlineRenderFunction<colormode_a2w4_i4>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel
	constexpr int ss		 = msbit(sizeof(onecolor));

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << ss);
		interp_set_accumulator(interp0, lane0, uint(*attributes++) << ss);

		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << ss);

		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w4_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w4_i4>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 4 pixel wide attributes and 8-bit indexed colors:

static constexpr ColorMode colormode_a2w4_ic8 = sizeof(Color) == 1 ? ColorMode(95) : colormode_a2w4_i8;

template<>
void RAM
scanlineRenderFunction<colormode_a2w4_ic8>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel
	constexpr int ss		 = msbit(sizeof(onecolor));

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << ss);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << ss);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << ss);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w4_ic8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w4_ic8>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 4 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w4_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) * sizeof(onecolor));

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_base(interp1, lane1, uint32(attributes++));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w4_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w4_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 8 pixel wide attributes and 4-bit indexed colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w8_i4>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes); // 2 bytes for 4 colors
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels);	 // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp0, lane0, uint(*attributes++) * sizeof(onecolor));
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		interp_set_accumulator(interp1, lane0, uint(*pixels++) * sizeof(onecolor));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w8_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w8_i4>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 8 pixel wide attributes and 8-bit indexed colors:

static constexpr ColorMode colormode_a2w8_ic8 = sizeof(Color) == 1 ? ColorMode(94) : colormode_a2w8_i8;

template<>
void XRAM
scanlineRenderFunction<colormode_a2w8_ic8>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint32* attributes = reinterpret_cast<const uint32*>(_attributes); // 4 bytes for 4 colors
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels);	 // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp0, lane0, uint(*attributes++) * sizeof(onecolor));
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		interp_set_accumulator(interp1, lane0, uint(*pixels++) * sizeof(onecolor));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w8_ic8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w8_ic8>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 8 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w8_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	onecolor*	  dest		 = reinterpret_cast<onecolor*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes); // 8 bytes for 4 colors
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels);	 // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_base(interp1, lane1, uint32(attributes++));
		interp_set_accumulator(interp1, lane0, uint(*pixels++) * sizeof(onecolor));

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
	}
}

template void setupScanlineRenderer<colormode_a2w8_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w8_rgb>() noexcept;


} // namespace kio::Video


/*

































*/
