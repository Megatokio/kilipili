// Copyright (c) 2023 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Graphics/graphics_types.h"

namespace kio::Graphics
{
using Video::Color;

static constexpr ColorDepth colordepth_rgb = sizeof(Color) == 1 ? colordepth_8bpp : colordepth_16bpp;

static constexpr ColorMode colormode_rgb	  = sizeof(Color) == 1 ? colormode_i8 : colormode_i16;
static constexpr ColorMode colormode_a1w1_rgb = sizeof(Color) == 1 ? colormode_a1w1_i8 : colormode_a1w1_i16;
static constexpr ColorMode colormode_a1w2_rgb = sizeof(Color) == 1 ? colormode_a1w2_i8 : colormode_a1w2_i16;
static constexpr ColorMode colormode_a1w4_rgb = sizeof(Color) == 1 ? colormode_a1w4_i8 : colormode_a1w4_i16;
static constexpr ColorMode colormode_a1w8_rgb = sizeof(Color) == 1 ? colormode_a1w8_i8 : colormode_a1w8_i16;
static constexpr ColorMode colormode_a2w1_rgb = sizeof(Color) == 1 ? colormode_a2w1_i8 : colormode_a2w1_i16;
static constexpr ColorMode colormode_a2w2_rgb = sizeof(Color) == 1 ? colormode_a2w2_i8 : colormode_a2w2_i16;
static constexpr ColorMode colormode_a2w4_rgb = sizeof(Color) == 1 ? colormode_a2w4_i8 : colormode_a2w4_i16;
static constexpr ColorMode colormode_a2w8_rgb = sizeof(Color) == 1 ? colormode_a2w8_i8 : colormode_a2w8_i16;

constexpr bool is_indexed_color(ColorDepth cd) noexcept { return cd < colordepth_rgb; }
constexpr bool is_indexed_color(ColorMode cm) noexcept { return is_indexed_color(get_colordepth(cm)); }
constexpr bool is_true_color(ColorDepth cd) noexcept { return cd >= colordepth_rgb; }
constexpr bool is_true_color(ColorMode cm) noexcept { return !is_indexed_color(cm); }

} // namespace kio::Graphics
