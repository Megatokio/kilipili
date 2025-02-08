// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "UniColorBackdrop.h"
#include "BitBlit.h"
#include "VideoBackend.h"

#define XRAM __attribute__((section(".scratch_x.UCBD" __XSTRING(__LINE__))))	 // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.UCBD" __XSTRING(__LINE__)))) // general ram


namespace kio::Video
{

using namespace Graphics;

UniColorBackdrop::UniColorBackdrop(Color color) noexcept :
	VideoPlane(nullptr, &render),
	color(Graphics::flood_filled_color<Graphics::colordepth_rgb>(color))
{}

void RAM UniColorBackdrop::render(VideoPlane* vp, int __unused row, uint32* fbu) noexcept
{
	UniColorBackdrop* me	= reinterpret_cast<UniColorBackdrop*>(vp);
	uint32			  color = me->color;
	volatile uint32*  fb	= fbu; // else the compiler may use memcpy() which is in rom!

	// a multiple of 8 bytes is always guaranteed:
	// note: worst case: width=200 & sizeof(Color)=1

	for (uint n = uint(vga_mode.width) / (8 / sizeof(Color)); n; n--)
	{
		*fb++ = color;
		*fb++ = color;
	}
}

} // namespace kio::Video
