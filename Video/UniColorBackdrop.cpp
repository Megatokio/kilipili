// Copyright (c) 2023 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "UniColorBackdrop.h"
#include "BitBlit.h"


namespace kilipili::Video
{

using namespace Graphics;

UniColorBackdrop::UniColorBackdrop(Color color) noexcept :
	VideoPlane(&vblank, &render),
	color(Graphics::flood_filled_color<Graphics::colordepth_rgb>(color))
{}

void __section(RAM ".ucbd") UniColorBackdrop::vblank(VideoPlane*) noexcept {}

void __section(XRAM ".ucbd") UniColorBackdrop::render(VideoPlane* vp, int __unused row, int width, uint32* fbu) noexcept
{
	UniColorBackdrop* me	= reinterpret_cast<UniColorBackdrop*>(vp);
	uint32			  color = me->color;
	volatile uint32*  fb	= fbu; // else the compiler may use memcpy() which is in rom!

	// a multiple of 8 bytes is always guaranteed:
	// note: worst case: width=200 & sizeof(Color)=1

	for (uint n = uint(width) / (8 / sizeof(Color)); n; n--)
	{
		*fb++ = color;
		*fb++ = color;
	}
}

} // namespace kilipili::Video
