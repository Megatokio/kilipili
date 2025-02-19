// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VerticalLayout.h"
#include <cstdio>
#include <hardware/gpio.h>

#define XRAM __attribute__((section(".scratch_x.VL" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.VL" __XSTRING(__LINE__)))) // general ram


namespace kio ::Video
{

constexpr int stopper = 8000;


VerticalLayout<2>::VerticalLayout(VideoPlane* p0, VideoPlane* p1, int h0, int h1) :
	VideoPlane(&do_vblank, &do_render),
	planes {{p0, h0}, {p1, h1}}
{}

VerticalLayout<2>::VerticalLayout(VideoPlane* p0, VideoPlane* p1, int h0) : VerticalLayout<2>(p0, p1, h0, stopper)
{
	assert(p0 && p1 && h0 >= -1000);
}

VerticalLayout<3>::VerticalLayout(VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, int h0, int h1, int h2) :
	VerticalLayout<2>(p0, p1, h0, h1),
	more_planes {p2, h2}
{}

VerticalLayout<3>::VerticalLayout(VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, int h0, int h1) :
	VerticalLayout<3>(p0, p1, p2, h0, h1, stopper)
{
	assert(p0 && p1 && p2 && h0 >= -1000 && h1 >= -1000);
}

VerticalLayout<4>::VerticalLayout(
	VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, VideoPlane* p3, int h0, int h1, int h2) :
	VerticalLayout<3>(p0, p1, p2, h0, h1, h2),
	more_planes {p3, stopper}
{
	assert(p0 && p1 && p2 && h0 >= -1000 && h1 >= -1000 && h2 >= -1000);
}


void RAM VerticalLayout<2>::do_vblank(VideoPlane* vp) noexcept
{
	VerticalLayout* me = reinterpret_cast<VerticalLayout*>(vp);

	me->idx = 0;
	me->top = 0;

	for (Plane* pp = me->planes;; pp++)
	{
		VideoPlane* vp = pp->vp;
		vp->vblank_fu(vp);
		if (pp->height == stopper) break;
	}
}

void RAM VerticalLayout<2>::do_render(VideoPlane* vp, int row, int width, uint32* fbu) noexcept
{
	VerticalLayout* me = reinterpret_cast<VerticalLayout*>(vp);

	int	   i   = me->idx;
	int	   top = me->top;
	Plane* pp  = &me->planes[i];

	if unlikely (row - top == pp->height)
	{
		me->top = top += pp->height;
		me->idx = i += 1;
		pp += 1;
	}

	vp = pp->vp;
	vp->render_fu(vp, row - top, width, fbu);
}


template class VerticalLayout<2>;
template class VerticalLayout<3>;
template class VerticalLayout<4>;

} // namespace kio::Video

/*


































*/
