// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/cdefs.h"
#include "common/standard_types.h"


namespace kilipili::Graphics
{

class Font
{
	//NO_COPY_MOVE(Font);

public:
	/*	construct a new font from prepared data[].
	*/
	Font(
		cstr name, const uint8* data, int row_offset, uint8 first_glyph, uint8 last_glyph, //
		uint8 char_width, uint8 char_height, uint8 baseline, bool fixed_width, bool copy = true);

	/*	construct const or static Font with default parameters.
		Font must be static and may be const or in flash.
		rc is set to -1  --> d'tor will never be called.
	*/
	constexpr Font(cstr name, const uint8* data) noexcept : //
		data(data),
		name(name),
		rc(-1)
	{}

	/* create copy in RAM:
	*/
	explicit Font(const Font&);

	constexpr ~Font() noexcept;
	Font& operator=(const Font&) = delete;

	const uint8* data		 = nullptr;
	cstr		 name		 = nullptr;
	int			 row_offset	 = 256;
	uint8		 first_glyph = 0;
	uint8		 last_glyph	 = 255;
	uint8		 char_width	 = 8;
	uint8		 char_height = 12;
	uint8		 baseline	 = 8;
	bool		 fixed_width = true;
	bool		 allocated	 = false; // => delete[] data in d'tor
	mutable int8 rc			 = 0;	  // -1 if in rom
};

extern const Font latin1_12x8;
extern const Font ascii_12x8_inverse;
extern const Font ascii_12x8_bold;
extern const Font graphics_12x8;


constexpr Font::~Font() noexcept
{
	// note: compiler calls d'tor for constexpr fonts
	// => general test for rc==0 is not possible.

	if (allocated)
	{
		assert(rc == 0);
		delete[] data;
	}
}

} // namespace kilipili::Graphics


/*























*/
