// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "standard_types.h"

namespace kio::Devices
{
class File;
}

namespace kio::Video
{

struct HamImageInfo
{
	using Color = Graphics::Color;
	using File	= Devices::File;

	uint32 magic;
	char   rgbstr[4];
	uint16 width, height;
	uint8  rbits, gbits, bbits, ibits;
	uint8  rshift, gshift, bshift, ishift;
	uint16 num_abs_codes;
	uint16 num_rel_codes;
	Color  cmap[256]; // converted if needed

	bool is_native_color = 0; // file's cmap[] is in our native Color
	char _padding[3];

	HamImageInfo(File*);

private:
	Color convert_abs_color(int qcolor);
	Color convert_rel_color(int qcolor);
};


} // namespace kio::Video
