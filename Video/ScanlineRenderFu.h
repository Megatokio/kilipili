// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "graphics_types.h"
#include <type_traits>

namespace kio::Video
{
using ColorMode = Graphics::ColorMode;
using Color		= Graphics::Color;

template<ColorMode CM, typename = std::enable_if_t<is_direct_color(CM)>>
void scanlineRenderFunction(uint32* dest, uint screen_width_pixels, const uint8* video_pixels);

template<ColorMode CM, typename = std::enable_if_t<!is_direct_color(CM)>>
void scanlineRenderFunction(
	uint32* dest, uint screen_width_pixels, const uint8* video_pixels, const uint8* video_attributes);

template<ColorMode>
void setupScanlineRenderer(const Color* colormap); // may throw

template<ColorMode>
void teardownScanlineRenderer() noexcept;


} // namespace kio::Video
