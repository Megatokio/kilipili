// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "MockPixmap.h"
#include "common/cstrings.h"

namespace kio::Graphics::Mock
{

Pixmap::Pixmap(coord w, coord h, ColorMode CM, AttrHeight AH) : Canvas(w, h, CM, AH, false)
{
	log.append(usingstr("%s(%i,%i,%s,%s)", __func__, w, h, tostr(CM), tostr(AH)));
}

Canvas* Pixmap::cloneWindow(coord x, coord y, coord w, coord h) throws
{
	log.append(usingstr("%s(%i,%i,%i,%i)", __func__, x, y, w, h));

	Canvas* window = new Pixmap(w, h, colormode, attrheight);

	// checks as in Pixmap<>:
	assert(x >= 0 && w >= 0 && x + w <= width);
	assert(y >= 0 && h >= 0 && y + h <= height);
	assert((x << colordepth()) % 8 == 0);

	return window;
}


void Pixmap::set_pixel(coord x, coord y, uint color, uint ink) noexcept
{
	log.append(usingstr("%s(%i,%i,%u,%u)", __func__, x, y, color, ink));
}

uint Pixmap::get_pixel(coord x, coord y, uint* ink) const noexcept
{
	log.append(usingstr("%s(%i,%i,uint*)", __func__, x, y)); //

	if (ink) *ink = 0;
	panic("TODO: Pixmap::get_pixel()");
	//return 0;
}

uint Pixmap::get_color(coord x, coord y) const noexcept
{
	log.append(usingstr("%s(%i,%i)", __func__, x, y)); //

	panic("TODO: Pixmap::get_color()");
	//return 0;
}

uint Pixmap::get_ink(coord x, coord y) const noexcept
{
	log.append(usingstr("%s(%i,%i)", __func__, x, y)); //

	panic("TODO: Pixmap::get_ink()");
	//return 0;
}

void Pixmap::draw_hline_to(coord x, coord y, coord x2, uint color, uint ink) noexcept
{
	log.append(usingstr("%s(%i,%i,%i,%u,%u)", __func__, x, y, x2, color, ink));
}

void Pixmap::draw_vline_to(coord x, coord y, coord y2, uint color, uint ink) noexcept
{
	log.append(usingstr("%s(%i,%i,%i,%u,%u)", __func__, x, y, y2, color, ink));
}

void Pixmap::fillRect(coord x, coord y, coord w, coord h, uint color, uint ink) noexcept
{
	log.append(usingstr("%s(%i,%i,%i,%i,%u,%u)", __func__, x, y, w, h, color, ink));
}

void Pixmap::xorRect(coord x, coord y, coord w, coord h, uint color) noexcept
{
	log.append(usingstr("%s(%i,%i,%i,%i,%u)", __func__, x, y, w, h, color));
}

void Pixmap::clear(uint color) noexcept
{
	log.append(usingstr("%s(%u)", __func__, color)); //
}

void Pixmap::copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept
{
	log.append(usingstr("%s(%i,%i,%i,%i,%i,%i)", __func__, zx, zy, qx, qy, w, h));
}

void Pixmap::copyRect(coord zx, coord zy, const Canvas&, coord qx, coord qy, coord w, coord h) noexcept
{
	log.append(usingstr("%s(%i,%i,Canvas,%i,%i,%i,%i)", __func__, zx, zy, qx, qy, w, h));
}

void Pixmap::readBmp(coord x, coord y, uint8*, int row_offs, coord w, coord h, uint color, bool set) noexcept
{
	log.append(usingstr("%s(%i,%i,uint8*,%i,%i,%i,%u,%u)", __func__, row_offs, x, y, w, h, color, set));
}

void Pixmap::drawBmp(coord zx, coord zy, const uint8*, int ro, coord w, coord h, uint color, uint ink) noexcept
{
	log.append(usingstr("%s(%i,%i,bmp,%i,%i,%i,%u,%u)", __func__, ro, zx, zy, w, h, color, ink));
}

void Pixmap::drawChar(coord zx, coord zy, const uint8*, coord h, uint color, uint ink) noexcept
{
	log.append(usingstr("%s(%i,%i,bmp,%i,%u,%u)", __func__, zx, zy, h, color, ink));
}


} // namespace kio::Graphics::Mock
