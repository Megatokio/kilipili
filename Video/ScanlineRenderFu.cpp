// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

/*
	This file provides Scanline Renderers which render one scanline in their respective Color Mode.

	note:
	low bits in screen byte = leftmost pixel
	low bits in attr byte   = leftmost attr / color with lower index in attr
*/

#include "ScanlineRenderFu.h"
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

static XRAM Color* video_colormap; // for old (non-interp) scanline renderers
static XRAM Color  temp_colors[4]; // for scanline renderers with attribute modes


/*  the interpolators are used for look-ups like
		value = table[byte&mask];
		byte >>= shift;

	there are 4 use case classes to find the vga color of a pixel in a VideoPlane function:

	direct true color:

		vgacolor = *screenbytes++;									// no interp used


	direct indexed color:

		idxcolor = screenbyte & colormask; screenbyte >> colorbits;	// interp0
		vgacolor = video_colormap[idxcolor]; 	 					// interp0


	true color attribute:

		*colors  = attributes; attributes += 2;

		vgacolor = colors[screenbyte & 1]; screenbyte >> 1;	or		// interp1
		vgacolor = colors[screenbyte & 3]; screenbyte >> 2;			// interp1

	indexed color attributes:

		temp_colors[0] = video_colormap[attrbyte & colormask]; attrbyte >> colorbits;	// interp0
		temp_colors[1] = video_colormap[attrbyte & colormask]; attrbyte >> colorbits;	// interp0

		vgacolor = temp_colors[screenbyte & 1]; screenbyte >> 1;	// interp1
*/

template<ColorMode CM>
void setupScanlineRenderer(const Color* colormap)
{
	constexpr ColorDepth cd = get_colordepth(CM);
	constexpr AttrMode	 am = get_attrmode(CM);

	assert(get_core_num() == 1);

	// setup interp0:
	if (cd != colordepth_16bpp)
	{
		interp_config cfg = interp_default_config(); // configure lane0
		interp_config_set_shift(&cfg, 1 << cd);		 // shift right by 1 .. 8 bit
		interp_set_config(interp0, lane0, &cfg);

		cfg = interp_default_config();			   // configure lane1
		interp_config_set_cross_input(&cfg, true); // read from accu lane0
		interp_config_set_mask(&cfg, 1, 1 << cd);  // mask lowest bits (shifted by 1 bit for sizeof(VgaColor))
		interp_set_config(interp0, lane1, &cfg);

		interp_set_base(interp0, lane0, 0);				   // lane0: add nothing
		interp_set_base(interp0, lane1, uint32(colormap)); // lane1: add color table base
	}

	// setup interp1:
	if (am != attrmode_none)
	{
		interp_config cfg = interp_default_config(); // configure lane0
		interp_config_set_shift(&cfg, 1 << am);		 // shift right by 1 or 2 bit
		interp_set_config(interp1, lane0, &cfg);

		cfg = interp_default_config();			   // configure lane1
		interp_config_set_cross_input(&cfg, true); // read from accu lane0
		interp_config_set_mask(&cfg, 1, 1 << am);  // mask lowest 1 or 2 bit (shifted by 1 bit for sizeof(VgaColor))
		interp_set_config(interp1, lane1, &cfg);

		interp_set_base(interp1, lane0, 0);					  // lane0: add nothing
		interp_set_base(interp1, lane1, uint32(temp_colors)); // lane1: add base of temp_colors[]
	}
}

template<ColorMode CM>
void teardownScanlineRenderer() noexcept
{
	constexpr ColorDepth cd = get_colordepth(CM);
	constexpr AttrMode	 am = get_attrmode(CM);

	// teardown interp0:
	if (cd != colordepth_16bpp)
	{
		//TODO
	}

	// teardown interp1:
	if (am != attrmode_none)
	{
		//TODO
	}
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
	const uint32* colors = reinterpret_cast<const uint32*>(video_colormap);

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
	const uint32* colors = reinterpret_cast<const uint32*>(video_colormap);

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
	uint16*		  dest	 = reinterpret_cast<uint16*>(_dest);
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

template void setupScanlineRenderer<colormode_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_i8>() noexcept;

// ============================================================================================
// direct true color mode:
// this version copies the pixels.
// a VideoPlane which uses nested dma should be implemented separately.

template<>
void RAM scanlineRenderFunction<colormode_rgb>(uint32* dest, uint width, const uint8* pixels)
{
	memcpy(dest, pixels, width * sizeof(Color));
}

template<>
void setupScanlineRenderer<colormode_rgb>(const Color*)
{}

template<>
void teardownScanlineRenderer<colormode_rgb>() noexcept
{}

// ============================================================================================
// attribute mode with 1 bit/pixel with 1 pixel wide attributes and 4-bit indexed colors:

template<>
void RAM
scanlineRenderFunction<colormode_a1w1_i4>(uint32* _dest, uint width, const uint8* pixels, const uint8* attributes)
{
	uint16* dest = reinterpret_cast<uint16*>(_dest);

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

template<>
void RAM
scanlineRenderFunction<colormode_a1w1_i8>(uint32* _dest, uint width, const uint8* pixels, const uint8* attributes)
{
	uint16* dest = reinterpret_cast<uint16*>(_dest);

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

template void setupScanlineRenderer<colormode_a1w1_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w1_i8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 1 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w1_rgb>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);

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

template<>
void XRAM
scanlineRenderFunction<colormode_a1w2_i8>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
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

template void setupScanlineRenderer<colormode_a1w2_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w2_i8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 2 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w2_rgb>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);

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

template<>
void XRAM
scanlineRenderFunction<colormode_a1w4_i8>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
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

template void setupScanlineRenderer<colormode_a1w4_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w4_i8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 4 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a1w4_rgb>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);

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
	uint16* dest = reinterpret_cast<uint16*>(_dest);

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

template<>
void XRAM
scanlineRenderFunction<colormode_a1w8_i8>(uint32* _dest, uint width, const uint8* pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
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

template void setupScanlineRenderer<colormode_a1w8_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a1w8_i8>() noexcept;

// ============================================================================================
// attribute mode with 1 bit/pixel with 8 pixel wide attributes and true colors:

template<>
XRAM void
scanlineRenderFunction<colormode_a1w8_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	// 2023-10-27
	// this version displays 1024x768 with avg/max load = 247.1/259.3MHz

#ifndef VIDEO_OPTIMISTICAL_A1W8_RGB
  #define VIDEO_OPTIMISTICAL_A1W8_RGB OFF
#endif

	uint32 ctable[4];
	interp_set_base(interp1, lane1, uint32(ctable));

	if ((width & 31) == 0)
	{
		uint32*		  dest		 = reinterpret_cast<uint32*>(_dest);
		const uint32* pixels	 = reinterpret_cast<const uint32*>(_pixels);
		const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);

		uint32 color10a;
		uint32 color10b = 0;

		for (uint i = 0; i < width / 32; i++)
		{
			color10a = *attributes++;
			if (!VIDEO_OPTIMISTICAL_A1W8_RGB || color10a != color10b)
			{
				uint32 color01 = (color10a >> 16) | (color10a << 16);
				uint32 xxx	   = uint16(color01 ^ color10a);
				ctable[1]	   = color01;
				ctable[2]	   = color10a;
				ctable[0]	   = color01 ^ xxx;
				ctable[3]	   = color10a ^ xxx;
			}

			uint32 bits = *pixels++;
			interp_set_accumulator(interp1, lane0, bits);

			*dest++ = ctable[bits & 3];
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));

			color10b = *attributes++;
			if (!VIDEO_OPTIMISTICAL_A1W8_RGB || color10a != color10b)
			{
				uint32 color01 = (color10b >> 16) | (color10b << 16);
				uint32 xxx	   = uint16(color01 ^ color10b);
				ctable[1]	   = color01;
				ctable[2]	   = color10b;
				ctable[0]	   = color01 ^ xxx;
				ctable[3]	   = color10b ^ xxx;
			}

			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));

			color10a = *attributes++;
			if (!VIDEO_OPTIMISTICAL_A1W8_RGB || color10a != color10b)
			{
				uint32 color01 = (color10a >> 16) | (color10a << 16);
				uint32 xxx	   = uint16(color01 ^ color10a);
				ctable[1]	   = color01;
				ctable[2]	   = color10a;
				ctable[0]	   = color01 ^ xxx;
				ctable[3]	   = color10a ^ xxx;
			}

			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));

			color10b = *attributes++;
			if (!VIDEO_OPTIMISTICAL_A1W8_RGB || color10a != color10b)
			{
				uint32 color01 = (color10b >> 16) | (color10b << 16);
				uint32 xxx	   = uint16(color01 ^ color10b);
				ctable[1]	   = color01;
				ctable[2]	   = color10b;
				ctable[0]	   = color01 ^ xxx;
				ctable[3]	   = color10b ^ xxx;
			}

			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
		}
	}
	else if constexpr (1)
	{
		// 400*300: 400 = 16 * 25 => not a multiple of 32!
		// 0x184 bytes in scratch_x
		// avg/max load = 44.1/46.4MHz

		uint32*		  dest		 = reinterpret_cast<uint32*>(_dest);
		const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels);
		const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);

		for (uint i = 0; i < width / 16; i++)
		{
			interp_set_accumulator(interp1, lane0, uint(*pixels++) << 2);

			{
				uint32 color10 = *attributes++;
				uint32 color01 = (color10 >> 16) | (color10 << 16);
				uint32 xxx	   = uint16(color01 ^ color10);
				ctable[1]	   = color01;
				ctable[2]	   = color10;
				ctable[0]	   = color01 ^ xxx;
				ctable[3]	   = color10 ^ xxx;
			}

			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));

			{
				uint32 color10 = *attributes++;
				uint32 color01 = (color10 >> 16) | (color10 << 16);
				uint32 xxx	   = uint16(color01 ^ color10);
				ctable[1]	   = color01;
				ctable[2]	   = color10;
				ctable[0]	   = color01 ^ xxx;
				ctable[3]	   = color10 ^ xxx;
			}

			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
		}
	}
	else
	{
		// 400*300: 400 = 16 * 25 => not a multiple of 32!
		// 0x154 bytes in scratch_x
		// avg/max load = 48.8/51.4MHz

		uint32*		  dest		 = reinterpret_cast<uint32*>(_dest);
		const uint8*  pixels	 = reinterpret_cast<const uint8*>(_pixels);
		const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);

		for (uint i = 0; i < width / 8; i++)
		{
			interp_set_accumulator(interp1, lane0, uint(*pixels++) << 2);

			{
				uint32 color10 = *attributes++;
				uint32 color01 = (color10 >> 16) | (color10 << 16);
				uint32 xxx	   = uint16(color01 ^ color10);
				ctable[1]	   = color01;
				ctable[2]	   = color10;
				ctable[0]	   = color01 ^ xxx;
				ctable[3]	   = color10 ^ xxx;
			}

			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
			*dest++ = *reinterpret_cast<const uint32*>(interp_pop_lane_result(interp1, lane1));
		}
	}
}

template<>
void setupScanlineRenderer<colormode_a1w8_rgb>(const Color* /*colormap*/)
{
	assert(get_core_num() == 1);

	// setup interp1:
	interp_config cfg = interp_default_config(); // configure lane0
	interp_config_set_shift(&cfg, 2);			 // shift right by 2 bits
	interp_set_config(interp1, lane0, &cfg);

	cfg = interp_default_config();			   // configure lane1
	interp_config_set_cross_input(&cfg, true); // read from accu lane0
	interp_config_set_mask(&cfg, 2, 3);		   // mask lowest 2 bits (shifted by 1 bit for sizeof(VgaColor))
	interp_set_config(interp1, lane1, &cfg);

	interp_set_base(interp1, lane0, 0); // lane0: add nothing
	//interp_set_base(interp1, lane1, uint32(temp_colors)); // lane1: add base of temp_colors[]
}

template void teardownScanlineRenderer<colormode_a1w8_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 1 pixel wide attributes and true colors:

template<>
void RAM
scanlineRenderFunction<colormode_a2w1_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

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

template void setupScanlineRenderer<colormode_a2w1_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w1_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 2 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w2_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

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

template void setupScanlineRenderer<colormode_a2w2_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w2_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 4 pixel wide attributes and 4-bit indexed colors:

template<>
void RAM
scanlineRenderFunction<colormode_a2w4_i4>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);
		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);

		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);

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

template<>
void RAM
scanlineRenderFunction<colormode_a2w4_i8>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint32* attributes = reinterpret_cast<const uint32*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);
		*dest++ = *next_color(interp1);

		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
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

template void setupScanlineRenderer<colormode_a2w4_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w4_i8>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 4 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w4_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes);
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels); // 16 bit for 8 pixel

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

template void setupScanlineRenderer<colormode_a2w4_rgb>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w4_rgb>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 8 pixel wide attributes and 4-bit indexed colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w8_i4>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint16* attributes = reinterpret_cast<const uint16*>(_attributes); // 2 bytes for 4 colors
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels);	 // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

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

template void setupScanlineRenderer<colormode_a2w8_i4>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w8_i4>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 8 pixel wide attributes and 8-bit indexed colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w8_i8>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint32* attributes = reinterpret_cast<const uint32*>(_attributes); // 4 bytes for 4 colors
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels);	 // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_accumulator(interp0, lane0, uint(*attributes++) << 1);
		temp_colors[0] = *next_color(interp0);
		temp_colors[1] = *next_color(interp0);
		temp_colors[2] = *next_color(interp0);
		temp_colors[3] = *next_color(interp0);

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

template void setupScanlineRenderer<colormode_a2w8_i8>(const Color* colormap);

template void teardownScanlineRenderer<colormode_a2w8_i8>() noexcept;

// ============================================================================================
// attribute mode with 2 bit/pixel with 8 pixel wide attributes and true colors:

template<>
void XRAM
scanlineRenderFunction<colormode_a2w8_rgb>(uint32* _dest, uint width, const uint8* _pixels, const uint8* _attributes)
{
	uint16*		  dest		 = reinterpret_cast<uint16*>(_dest);
	const uint64* attributes = reinterpret_cast<const uint64*>(_attributes); // 8 bytes for 4 colors
	const uint16* pixels	 = reinterpret_cast<const uint16*>(_pixels);	 // 16 bit for 8 pixel

	for (uint i = 0; i < width / 8; i++)
	{
		interp_set_base(interp1, lane1, uint32(attributes++));
		interp_set_accumulator(interp1, lane0, uint(*pixels++) << 1);

		// next_color(interp) -> reinterpret_cast<const VgaColor*>(interp_pop_lane_result(interp,lane1));

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

} // namespace kio::Video
