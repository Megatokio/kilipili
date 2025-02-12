// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VerticalLayout.h"
#include "VideoController.h"

#define XRAM __attribute__((section(".scratch_x.VL" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.VL" __XSTRING(__LINE__)))) // general ram


namespace kio ::Video
{

VerticalLayout<2>::VerticalLayout(VideoPlane* p1, VideoPlane* p2, int height) :
	VideoPlane(&do_vblank, &do_render),
	plane1(p1),
	plane2(p2),
	height(height)
{
	assert(p1 && p2 && height >= -1000);
}

template<>
VerticalLayout<3>::VerticalLayout(VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, int h0, int h1) :
	VideoPlane(&do_vblank, &do_render),
	planes {p0, p1, p2},
	bottom {h0, h0 + h1, 9999},
	idx(0)
{
	assert(p0 && p1 && p2 && h0 >= -1000 && h1 >= -1000);
}

template<>
VerticalLayout<4>::VerticalLayout(
	VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, VideoPlane* p3, int h0, int h1, int h2) :
	VideoPlane(&do_vblank, &do_render),
	planes {p0, p1, p2, p3},
	bottom {h0, h0 + h1, h0 + h1 + h2, 9999},
	idx(0)
{
	assert(p0 && p1 && p2 && h0 >= -1000 && h1 >= -1000 && h2 >= -1000);
}


void RAM VerticalLayout<2>::do_vblank(VideoPlane* vp) noexcept
{
	VerticalLayout* me = reinterpret_cast<VerticalLayout*>(vp);
	VideoController::vblank(me->plane1);
}

template<int N>
void RAM VerticalLayout<N>::do_vblank(VideoPlane* vp) noexcept
{
	VerticalLayout* me = reinterpret_cast<VerticalLayout*>(vp);
	me->idx			   = 0;
	VideoController::vblank(me->planes[0]);
}

void RAM VerticalLayout<2>::do_render(VideoPlane* vp, int row, int width, uint32* fbu) noexcept
{
	VerticalLayout* me = reinterpret_cast<VerticalLayout*>(vp);

	int height = me->height;
	if (row < height) return VideoController::render(me->plane1, row, width, fbu);

	row -= height;
	if unlikely (row == 0) VideoController::vblank(me->plane2);

	VideoController::render(me->plane2, row, width, fbu);
}

template<int N>
void RAM VerticalLayout<N>::do_render(VideoPlane* vp, int row, int width, uint32* fbu) noexcept
{
	VerticalLayout* me = reinterpret_cast<VerticalLayout*>(vp);

	int i	   = me->idx;
	int bottom = me->bottom[i];

	if unlikely (row >= bottom)
	{
		do bottom = me->bottom[++i];
		while unlikely(row >= bottom);

		me->idx = i;
		VideoController::vblank(me->planes[i]);
	}

	int top = i ? me->bottom[i - 1] : 0;
	VideoController::render(me->planes[i], row - top, width, fbu);
}


template class VerticalLayout<2>;
template class VerticalLayout<3>;
template class VerticalLayout<4>;

} // namespace kio::Video

/*


































*/
