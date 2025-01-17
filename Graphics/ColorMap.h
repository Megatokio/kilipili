// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "graphics_types.h"

namespace kio::Graphics
{

// ---------------------- system Color tables --------------------------

extern const Color zx_colors[16];
extern const Color vga4_colors[16];
extern const Color vga8_colors[256];
extern const Color default_i1_colors[2];
extern const Color default_i2_colors[4];
extern const Color default_i4_colors[16];
extern const Color default_i8_colors[256];

extern const Color* const default_colors[5]; // ColorMode -> Color[]

template<ColorMode CM>
extern const Color* const system_colors = default_colors[CM];


// ---------------------- struct ColorMap --------------------------

template<ColorDepth CD>
struct ColorMap;

template<>
struct ColorMap<colordepth_16bpp>
{
	mutable uint16 rc = 0;
	Id("ColorMap");
	static constexpr Color* colors = nullptr;

	ColorMap(const Color* = nullptr) noexcept {}
	void reset(const Color* = nullptr) noexcept {}
};

template<>
struct ColorMap<colordepth_1bpp> : public ColorMap<colordepth_16bpp>
{
	Color  colors[2];
	Color& operator[](int i) { return colors[i]; }
	Color  operator[](int i) const { return colors[i]; }

	ColorMap(const Color* colors = default_i1_colors) noexcept;
	void reset(const Color* = default_i1_colors) noexcept;
};

template<>
struct ColorMap<colordepth_2bpp> : public ColorMap<colordepth_1bpp>
{
	Color more_colors[4 - 2];

	ColorMap(const Color* colors = default_i2_colors) noexcept;
	void reset(const Color* = default_i2_colors) noexcept;
};

template<>
struct ColorMap<colordepth_4bpp> : public ColorMap<colordepth_2bpp>
{
	Color more_colors[16 - 4];

	ColorMap(const Color* colors = default_i4_colors) noexcept;
	void reset(const Color* = default_i4_colors) noexcept;
};

template<>
struct ColorMap<colordepth_8bpp> : public ColorMap<colordepth_4bpp>
{
	Color more_colors[256 - 16];

	ColorMap(const Color* colors = default_i8_colors) noexcept;
	void reset(const Color* = default_i8_colors) noexcept;

	template<ColorDepth CD2>
	ColorMap<CD2>* as() noexcept // easy access system_colormap as smaller cmap
	{
		return this;
	}
	template<ColorMode CM2>
	ColorMap<get_colordepth(CM2)>* as() noexcept // easy access system_colormap as smaller cmap
	{
		return this;
	}
};

extern ColorMap<colordepth_8bpp> system_colormap;


} // namespace kio::Graphics


/*
































*/
