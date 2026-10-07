// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Font.h"
#include <cstring>

namespace kilipili::Graphics
{


static constexpr uint8 latin1_data[12][256] = {
#include "rsrc/latin-1_12x8.h"
};

static constexpr uint8 ascii_inverse_data[12][256] = {
#include "rsrc/ascii_12x8_inverse.h"
};

static constexpr uint8 ascii_bold_data[12][256] = {
#include "rsrc/ascii_12x8_bold.h"
};

static constexpr uint8 graphics_data[12][256] = {
#include "rsrc/graphics_12x8.h"
};

constexpr Font latin1_12x8 {"latin-1_12x8", latin1_data[0]};
constexpr Font ascii_12x8_inverse {"ascii_12x8_inverse", ascii_inverse_data[0]};
constexpr Font ascii_12x8_bold {"ascii_12x8_bold", ascii_bold_data[0]};
constexpr Font graphics_12x8 {"graphics_12x8", graphics_data[0]};


Font::Font(
	cstr name, const uint8* data, int row_offset,		 //
	uint8 first_glyph, uint8 last_glyph,				 //
	uint8 char_width, uint8 char_height, uint8 baseline, //
	bool fixed_width, bool copy) :
	data(data),
	name(name),
	row_offset(last_glyph - first_glyph + 1),
	first_glyph(first_glyph),
	last_glyph(last_glyph),
	char_width(char_width),
	char_height(char_height),
	baseline(baseline),
	fixed_width(fixed_width)
{
	assert(data);
	assert(char_width == 8);

	if (copy)
	{
		uint8* z	   = new uint8[uint(this->row_offset * char_height)];
		this->data	   = z;
		allocated	   = true;
		const uint8* q = data;
		for (int row = 0; row < char_height; row++)
		{
			memcpy(z, q, uint(this->row_offset));
			z += this->row_offset;
			q += row_offset;
		}
	}
}

Font::Font(const Font& q) :
	Font(
		q.name, q.data, q.row_offset,										  //
		q.first_glyph, q.last_glyph, q.char_width, q.char_height, q.baseline, //
		q.fixed_width, true)

{}


} // namespace kilipili::Graphics


/*





























*/
