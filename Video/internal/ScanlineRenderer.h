// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Pixmap_wAttr.h"


/*	ScanlineRenderers for the Pixmap based FrameBuffers
*/


namespace kilipili::Video
{
using ColorMode = Graphics::ColorMode;
using Color		= Graphics::Color;


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
template<class Pixmap>
void ScanlineRenderer(uint32* dest, uint width_in_pixels, const uint8* pixels_in, const uint8* attributes) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a1w1>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a1w2>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a1w4>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a1w8>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a2w1>(uint32* dest, uint width, const uint8* pix, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a2w2>(uint32* dest, uint width, const uint8* px, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a2w4>(uint32* dest, uint width, const uint8* px, const uint8* attr) noexcept;
template<>
void ScanlineRenderer<Graphics::Pixmap_a2w8>(uint32* dest, uint width, const uint8* px, const uint8* attr) noexcept;


} // namespace kilipili::Video


/*


























*/
