// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Passepartout.h"
#include "Graphics/Color.h"
#include "VideoBackend.h"
#include "VideoController.h"

#define XRAM __attribute__((section(".scratch_x.PPT" __XSTRING(__LINE__))))		// the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.PPT" __XSTRING(__LINE__)))) // general ram


namespace kio ::Video
{
//void RAM memcpy_in_ram(void* z, const void* q, size_t n) noexcept
//{
//	size_t qa = size_t(q);
//	size_t za = size_t(z);
//
//	if (n >= 8)
//	{
//		for (; za & 3; za++, qa++, n--) { *ptr(za) = *reinterpret_cast<volatile const char*>(qa); }
//
//		if ((qa & 3) == 0)
//			for (; n >= 4; za += 4, qa += 4, n -= 4)
//			{
//				*uint32ptr(za) = *reinterpret_cast<volatile const uint32*>(qa); //
//			}
//
//		else if ((qa & 1) == 0)
//			for (; n >= 4; za += 4, qa += 4, n -= 4) // little endian!
//			{
//				*uint32ptr(za) = *reinterpret_cast<volatile const uint16*>(qa) +
//								 *reinterpret_cast<volatile const uint16*>(qa + 2) * 0x10000u;
//			}
//	}
//
//	for (; n; za++, qa++, n--) { *ptr(za) = *reinterpret_cast<volatile const char*>(qa); }
//}

using namespace Graphics;

constexpr int ss = sizeof(Color) == 1 ? 2 : sizeof(Color) == 2 ? 1 : 0;

void RAM clear_row(volatile uint32* z, int w) noexcept
{
	while (--w >= 0) { *z++ = 0x00; }
}

Passepartout::Passepartout(RCPtr<VideoPlane> vp, int __unused width, int height, int inner_width, int inner_height) :
	VideoPlane(&do_vblank, &do_render),
	vp(std::move(vp))
{
	setSize(width, height, inner_width, inner_height);
}

Passepartout::Passepartout(RCPtr<VideoPlane> vp, int inner_width, int inner_height) :
	VideoPlane(&do_vblank, &do_render),
	vp(std::move(vp))
{
	setSize(screen_width(), screen_height(), inner_width, inner_height);
}

void Passepartout::setup() { vp->setup(); }
void Passepartout::teardown() noexcept { vp->teardown(); }

void RAM Passepartout::do_render(VideoPlane* vp, int row, int width, uint32* fbu) noexcept
{
	width >>= ss; // measured in uint32

	Passepartout* me = reinterpret_cast<Passepartout*>(vp);
	assert(width >= me->inner_width);

	if (uint(row - me->top) < uint(me->inner_height))
	{
		int inner_width		= me->inner_width;		  // measured in uint32
		int left_plus_right = (width - inner_width);  // measured in uint32
		int left			= left_plus_right >> 1;	  // measured in uint32
		int right			= left_plus_right - left; // measured in uint32

		clear_row(fbu, left);
		clear_row(fbu + left + inner_width, right);
		VideoController::render(me->vp, row, inner_width << ss, fbu + left);
	}
	else
	{
		clear_row(fbu, width); //
	}
}

void RAM Passepartout::do_vblank(VideoPlane* vp) noexcept
{
	Passepartout* me = reinterpret_cast<Passepartout*>(vp);

	VideoController::vblank(me->vp);
}

void Passepartout::setInnerSize(int inner_width, int inner_height) noexcept
{
	this->inner_width  = min(inner_width, width) >> ss;
	this->inner_height = min(inner_height, height);
	this->top		   = (height - this->inner_height) / 2;
}

void Passepartout::setSize(int width, int height, int inner_width, int inner_height) noexcept
{
	this->width	 = uint16(width);
	this->height = uint16(height);
	setInnerSize(inner_width, inner_height);
}

void Passepartout::setSize(int width, int height) noexcept
{
	setSize(width, height, inner_width << ss, inner_height); //
}


} // namespace kio::Video


/*
























*/
