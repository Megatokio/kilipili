// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "graphics_types.h"


/*	ScanlineRenderers for the FrameBuffers

	Tweakable Parts:

	VIDEO_INTERP0_MODE: configure pixel size for interp0: -1=any, 0…3 -> 1,2,4,8 bit, 5=a1w8 (default)
	VIDEO_INTERP1_MODE: configure pixel size for interp1: -1=any (default), 0…3 -> 1,2,4,8 bit, 5=a1w8
	  -	these options define the default setting for interp0 and interp1 on core1.
	  - an interpolator may be set for a specific color mode or for use by any color mode.
	  - if an interpolator is set for a specific color mode then it is setup at startup by the VideoController.
	  - otherwise the ScanlineRenderer must set it up at the start of each scanline.
	  - if it uses an interpolator which is reserved for another color mode (because no interpolator is set to 'any')
		then the ScanlineRenderer must also restore it to the reserved mode at the end of the scanline.


	Interpolator settings needed for the various color modes:
	(may change. please check source)

		0 = 1 bpp: a1w1 a1w2 a1w4 a1w8 
		1 = 2 bpp: a2w1 a2w2 a2w4 a2w8
		2 = 4 bpp: i4
		3 = 8 bpp: i8 ham
		5 = 2 bp2: a1w8
		none:      i1 i2 rgb


	Own Render Functions using an Interpolator:

	If an application defines a new render function which uses an interpolator, it must take care
	to avoid conflicts with the existing ScanlineRenderers it uses:
	  - either fully setup the interpolator at the start of each scanline and restore it at the end
		to the extend needed (whether it is set to 'any' or a specific mode, see helper templates in the source)
	  -	or reserve interp1 (not interp0) for your renderer exclusively with VIDEO_INTERP1_MODE=99 or similar.
*/


namespace kio::Video
{
using ColorMode = Graphics::ColorMode;
using Color		= Graphics::Color;


// one-time initialization:
// called by VideoController
extern void initializeInterpolators() noexcept;


// _________________________________________________________________
struct ScanlineRenderer_i1
{
	Color colormap[256 * 8]; // 2 or 4kB

	ScanlineRenderer_i1(const Color* colormap) noexcept;
	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
struct ScanlineRenderer_i2
{
	Color colormap[256 * 4]; // 1 or 2kB

	ScanlineRenderer_i2(const Color* colormap) noexcept;
	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
struct ScanlineRenderer_i4
{
	const Color* colormap;

	ScanlineRenderer_i4(const Color* colormap) noexcept : colormap(colormap) {}

	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
struct ScanlineRenderer_i8
{
	const Color* colormap;

	ScanlineRenderer_i8(const Color* colormap) noexcept : colormap(colormap) {}

	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
void ScanlineRenderer_rgb(uint32* scanline_out, uint width_in_pixels, const uint8* pixels_in) noexcept;

// _________________________________________________________________
template<ColorMode CM>
void ScanlineRenderer(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a1w1>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a1w2>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a1w4>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a1w8>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a2w1>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a2w2>(uint32* dest, uint width, const uint8* px, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a2w4>(uint32* dest, uint width, const uint8* px, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::colormode_a2w8>(uint32* dest, uint width, const uint8* px, const uint8* attr) noexcept;

// _________________________________________________________________
struct HamImageScanlineRenderer
{
	const Color* colormap;
	uint16		 first_rel_code;
	Color		 first_color; // initial color at start of next row

	HamImageScanlineRenderer(const Color* colors, uint16 num_abs_codes) noexcept : //
		colormap(colors),
		first_rel_code(num_abs_codes)
	{}

	inline void vblank() noexcept { first_color = Graphics::black; }
	void		render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};


} // namespace kio::Video


/*


























*/
