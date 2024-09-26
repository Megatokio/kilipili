// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Canvas.h"
#include "common/Array.h"

namespace kio::Graphics::Mock
{

class Pixmap : public Canvas
{
public:
	Pixmap(coord w, coord h, ColorMode CM = colormode_a1w8_rgb, AttrHeight AH = attrheight_12px);
	~Pixmap() override = default;

	Canvas* cloneWindow(coord x, coord y, coord w, coord h) throws override;
	void	set_pixel(coord x, coord y, uint color, uint ink = 0) noexcept override;
	uint	get_pixel(coord x, coord y, uint* ink) const noexcept override;
	uint	get_color(coord x, coord y) const noexcept override;
	uint	get_ink(coord x, coord y) const noexcept override;
	void	draw_hline_to(coord x, coord y, coord x2, uint color, uint ink) noexcept override;
	void	draw_vline_to(coord x, coord y, coord y2, uint color, uint ink) noexcept override;
	void	fillRect(coord x, coord y, coord w, coord h, uint color, uint ink = 0) noexcept override;
	void	xorRect(coord x, coord y, coord w, coord h, uint color) noexcept override;
	void	clear(uint color) noexcept override;
	void	copyRect(coord zx, coord zy, coord qx, coord qy, coord w, coord h) noexcept override;
	void	copyRect(coord zx, coord zy, const Canvas& src, coord qx, coord qy, coord w, coord h) noexcept override;
	void readBmp(coord x, coord y, uint8* bmp, int row_offs, coord w, coord h, uint color, bool set) noexcept override;
	void drawBmp(coord zx, coord zy, const uint8* bmp, int ro, coord w, coord h, uint c, uint i = 0) noexcept override;
	void drawChar(coord zx, coord zy, const uint8* bmp, coord h, uint color, uint = 0) noexcept override;

	mutable Array<cstr> log; // tempstr
};

} // namespace kio::Graphics::Mock
