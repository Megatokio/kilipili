// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "graphics_types.h"


/*	ScanlineRenderers for the FrameBuffers

	tweakable parts:

	VIDEO_INTERP_SETUP_PER_SCANLINE
	  -	If set to true, then the interpolators are set up at the start of each and every scanline.
	  -	Normally this is not needed and not done because it takes valuable time every scanline.
		Instead the interpolators are set up at the start of each frame.
	  -	This option is needed if you place 2 incompatible VideoPlanes side by side or on top of each other.

	VIDEO_INTERP0_MODE: configure pixel size for interp0: -1=all, 0…3 -> 1,2,4,8 bit, 5=a1w8 (default)
	VIDEO_INTERP1_MODE: configure pixel size for interp1: -1=all (default), 0…3 -> 1,2,4,8 bit, 5=a1w8
	  -	these options define the default setting for interp0 and interp1.
	  -	a ScanlineRenderer does not need to set up the interpolator if an interpolator is set up for it's need.
	  -	otherwise the ScanlineRenderer must set it up at the start of each scanline or frame.
	  - if no interpolator is set up for -1=all, then you must enable VIDEO_INTERP_SETUP_PER_SCANLINE if
		you want to place 2 of the remaining modes side-by-side or on top of each other, which is rarely needed.

	Interpolator setups for the various color modes:  (may change. please check source)
		-1 = none: i1 i2 rgb
		0 = 1 bpp: a1w1 a1w2 a1w4 a1w8 
		1 = 2 bpp: a1w1 a1w2 a2w4 a2w8
		2 = 4 bpp: i4
		3 = 8 bpp: i8 ham
		5 = 2 bp2: a1w8
*/


namespace kio::Video
{
using ColorMode = Graphics::ColorMode;
using Color		= Graphics::Color;


// one-time initialization:
// called by VideoController
extern void initializeInterpolators() noexcept;


template<ColorMode CM>
struct ScanlineRenderer;


// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_i1>
{
	Color colormap[256 * 8]; // 2 or 4kB

	ScanlineRenderer(const Color* colormap) noexcept;
	~ScanlineRenderer() = default;

	void vblank() noexcept {}
	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_i2>
{
	Color colormap[256 * 4]; // 1 or 2kB

	ScanlineRenderer(const Color* colormap) noexcept;
	~ScanlineRenderer() = default;

	void vblank() noexcept {}
	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_i4> // uses interp1
{
	const Color* colormap;

	ScanlineRenderer(const Color* colormap) noexcept : colormap(colormap) {}
	~ScanlineRenderer() = default;

	void vblank() noexcept;
	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_i8> // uses interp1
{
	const Color* colormap;

	ScanlineRenderer(const Color* colormap) noexcept : colormap(colormap) {}
	~ScanlineRenderer() = default;

	void vblank() noexcept;
	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_rgb>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept {}
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a1w1>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a1w2>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a1w4>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a1w8>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a2w1>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a2w2>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a2w4>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};

// _________________________________________________________________
template<>
struct ScanlineRenderer<ColorMode::colormode_a2w8>
{
	ScanlineRenderer() noexcept {}
	~ScanlineRenderer() = default;

	static void vblank() noexcept;
	static void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
};


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
	~HamImageScanlineRenderer() = default;

	void vblank() noexcept;
	void render(uint32* dest, uint width_in_pixels, const uint8* pixels_in) noexcept;
};


} // namespace kio::Video


/*


























*/
