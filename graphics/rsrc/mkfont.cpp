// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include <cstdint>
#include <cstdio>

using uchar	 = unsigned char;
using uint32 = uint32_t;
using uint8	 = uint8_t;


// printing options:
// line length: 708, ~750, 1284 or 2820 characters!
#define hex	 1
#define dec	 0
#define bin	 0
#define step 1 // 1 or 4:hex


// font data:
static constexpr uchar font[] = {
//#include "glyphs_12x8/latin1_12x8_source.h"
//#include "glyphs_12x8/ascii_12x8_bold_source.h"
//#include "glyphs_12x8/ascii_12x8_inverse_source.h"
#include "glyphs_12x8/graphics_12x8_source.h"
};

static_assert(sizeof(font) == num_codes * char_height);


int print_bytes(const uchar* data)
{
	if (step == 1 && hex) return printf("0x%02x,", *data);
	if (step == 1 && bin) return printf("0b%08b,", *data);
	if (step == 1 && dec) return printf("%u,", *data);

	uchar cx[4];
	cx[0] = data[0];
	cx[1] = data[char_height * 1];
	cx[2] = data[char_height * 2];
	cx[3] = data[char_height * 3];

	uint32 n = *reinterpret_cast<uint32*>(cx);
	return printf("0x%08x,", n);
}

int main()
{
	printf("// clang-format off\n");

	for (int row = 0; row < char_height; row++)
	{
		printf("{");
		for (int c = 0; c < num_codes; c += step) { print_bytes(font + row + c * char_height); }
		printf("},\n");
	}

	printf("// clang format on\n");
	return 0;
}
