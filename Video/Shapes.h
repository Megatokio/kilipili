// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "kilipili_cdefs.h"

namespace kio ::Video
{
struct Shape
{
	// data = sequence of rows.
	// the data stream starts with Size and Hot to define the total size and the hot spot for this shape.
	// Each row starts with a HDR and then that number of colors follow.
	// After that there is the HDR of the next row.
	// If the next HDR is a CMD, then handle this CMD as part of the current line:
	// 	 END:  shape is finished, remove it from hotlist.
	// 	 SKIP: resume one more HDR at the current position: used to insert transparent space.

	const Color* pixels;
	Shape(const Color* c = nullptr) noexcept : pixels(c) {}

	struct Preamble
	{
		uint8 width;
		uint8 height;
		int8  hot_x;
		int8  hot_y;
	};

	struct PFX // raw pixels prefix
	{
		int8  dx;	 // initial offset
		uint8 width; // count of pixels that follow
	};

	enum CMD : uint16 { END = 0x0080, SKIP = 0x0180 }; // little endian

	bool is_cmd() const noexcept { return reinterpret_cast<const PFX*>(pixels)->dx == -128; }
	bool is_pfx() const noexcept { return reinterpret_cast<const PFX*>(pixels)->dx != -128; }
	bool is_end() const noexcept
	{
		return *reinterpret_cast<const CMD*>(pixels) == END;
	} // TODO alignment if sizeof(Color)==1
	bool is_skip() const noexcept { return *reinterpret_cast<const CMD*>(pixels) == SKIP; }

	const CMD&		cmd() const noexcept { return *reinterpret_cast<const CMD*>(pixels); }
	const PFX&		pfx() const noexcept { return *reinterpret_cast<const PFX*>(pixels); }
	const Preamble& preamble() const noexcept { return *reinterpret_cast<const Preamble*>(pixels); }

	int8  dx() const noexcept { return pfx().dx; }
	uint8 width() const noexcept { return pfx().width; }

	void skip_cmd() noexcept { pixels += sizeof(CMD) / sizeof(*pixels); }
	void skip_pfx() noexcept { pixels += sizeof(PFX) / sizeof(*pixels); }
	void skip_preamble() noexcept { pixels += sizeof(Preamble) / sizeof(*pixels); }

	void next_row() noexcept
	{
		while (1)
		{
			assert(is_pfx());
			pixels += sizeof(PFX) / sizeof(*pixels) + width();
			if (!is_skip()) break;
			skip_cmd();
		}
	}
};


} // namespace kio::Video
