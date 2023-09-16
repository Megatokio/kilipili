// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "graphics_types.h"
#include "kipili_common.h"

namespace kio::Graphics
{

template<ColorDepth CD>
using ColorMap = Color[1 << (1 << CD)];

extern const ColorMap<colordepth_4bpp> zx_colors;
extern const ColorMap<colordepth_1bpp> default_colormap_i1;
extern const ColorMap<colordepth_2bpp> default_colormap_i2;
extern const ColorMap<colordepth_4bpp> default_colormap_i4;
extern const ColorMap<colordepth_8bpp> default_colormap_i8;

extern const Color* const default_colormaps[5]; // ColorMode -> colormap


inline const Color* getDefaultColorMap(ColorDepth CD) noexcept { return default_colormaps[CD]; }

inline void resetColorMap(ColorDepth CD, Color* table) noexcept
{
	if (is_indexed_color(CD)) memcpy(table, default_colormaps[CD], sizeof(Color) << (1 << CD));
}


template<ColorDepth CD, typename = std::enable_if_t<is_indexed_color(CD)>>
const Color* getDefaultColorMap() noexcept
{
	return default_colormaps[CD];
}

template<ColorMode CM, typename = std::enable_if_t<is_indexed_color(CM)>>
const Color* getDefaultColorMap() noexcept
{
	return getDefaultColorMap<get_colordepth(CM)>();
}


template<ColorDepth CD>
void resetColorMap(Color* table) noexcept
{
	memcpy(table, default_colormaps[CD], sizeof(Color) << (1 << CD));
}

template<>
inline void resetColorMap<colordepth_16bpp>(Color*) noexcept
{}


} // namespace kio::Graphics
