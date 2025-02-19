// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HorizontalLayout.h"
#include "Graphics/Color.h"
#include "basic_math.h"
#include <hardware/gpio.h>

#define XRAM __attribute__((section(".scratch_x.HL" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.HL" __XSTRING(__LINE__)))) // general ram


namespace kio ::Video
{

constexpr int ss = msbit(sizeof(Graphics::Color));
constexpr int zz = 2 - ss;

constexpr int stopper = 8000;

HorizontalLayout<2>::HorizontalLayout(VideoPlane* p0, VideoPlane* p1, int w0) :
	VideoPlane(&do_vblank, &do_render),
	planes {{p0, w0 >> zz << zz}, {p1, stopper}}
{
	assert(p0 && p1 && w0 >= 0);
}

HorizontalLayout<3>::HorizontalLayout(VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, int w0, int w1) :
	HorizontalLayout<2>(p0, p1, w0),
	more_planes {{p2, stopper}}
{
	planes[1].width = w1 >> zz << zz;
	assert(p0 && p1 && p2 && w0 >= 0 && w1 >= 0);
}

HorizontalLayout<4>::HorizontalLayout(
	VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, VideoPlane* p3, int w0, int w1, int w2) :
	HorizontalLayout<3>(p0, p1, p2, w0, w1),
	more_planes {{p3, stopper}}
{
	planes[2].width = w2 >> zz << zz;
	assert(p0 && p1 && p2 && w0 >= 0 && w1 >= 0 && w2 >= 0);
}


void RAM HorizontalLayout<2>::do_vblank(VideoPlane* vp) noexcept
{
	HorizontalLayout* me = reinterpret_cast<HorizontalLayout*>(vp);

	//gpio_set_mask(1 << PICO_DEFAULT_LED_PIN);
	for (Plane* pp = me->planes;; pp++)
	{
		VideoPlane* vp = pp->vp;
		vp->vblank_fu(vp);
		if (pp->width == stopper) break;
	}
	//gpio_set_mask(1 << PICO_DEFAULT_LED_PIN);
}

void RAM HorizontalLayout<2>::do_render(VideoPlane* vp, int row, int width, uint32* fbu) noexcept
{
	HorizontalLayout* me = reinterpret_cast<HorizontalLayout*>(vp);

	//gpio_set_mask(1 << PICO_DEFAULT_LED_PIN);
	for (Plane* pp = me->planes;; pp++)
	{
		VideoPlane* vp = pp->vp;

		int w = pp->width;
		if (w > width) w = width;

		vp = pp->vp;
		vp->render_fu(vp, row, w, fbu);

		width -= w;
		fbu += w >> zz;

		if (width == 0) break;
	}
	//gpio_clr_mask(1 << PICO_DEFAULT_LED_PIN);
}


template class HorizontalLayout<2>;
template class HorizontalLayout<3>;
template class HorizontalLayout<4>;


} // namespace kio::Video

/*

























*/
