// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "HorizontalLayout.h"
#include "Graphics/Color.h"
#include "VideoController.h"
#include "basic_math.h"

#define XRAM __attribute__((section(".scratch_x.HL" __XSTRING(__LINE__))))	   // the 4k page with the core1 stack
#define RAM	 __attribute__((section(".time_critical.HL" __XSTRING(__LINE__)))) // general ram


namespace kio ::Video
{

constexpr int ss = msbit(sizeof(Graphics::Color));
constexpr int zz = 2 - ss;

template<>
HorizontalLayout<2>::HorizontalLayout(VideoPlane* p1, VideoPlane* p2, int w0) :
	VideoPlane(&do_vblank, &do_render),
	planes {p1, p2},
	width {w0 >> zz << zz, 8000}
{
	assert(p1 && p2 && w0 >= 0);
}

template<>
HorizontalLayout<3>::HorizontalLayout(VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, int w0, int w1) :
	VideoPlane(&do_vblank, &do_render),
	planes {p0, p1, p2},
	width {w0 >> zz << zz, w1 >> zz << zz, 9999}
{
	assert(p0 && p1 && p2 && w0 >= 0 && w1 >= 0);
}

template<>
HorizontalLayout<4>::HorizontalLayout(
	VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, VideoPlane* p3, int w0, int w1, int w2) :
	VideoPlane(&do_vblank, &do_render),
	planes {p0, p1, p2, p3},
	width {w0 >> zz << zz, w1 >> zz << zz, w2 >> zz << zz, 9999}
{
	assert(p0 && p1 && p2 && w0 >= 0 && w1 >= 0 && w2 >= 0);
}


template<int N>
void RAM HorizontalLayout<N>::do_vblank(VideoPlane* vp) noexcept
{
	HorizontalLayout* me = reinterpret_cast<HorizontalLayout*>(vp);
	for (int i = 0; i < N; i++) VideoController::vblank(me->planes[i]);
}

template<int N>
void RAM HorizontalLayout<N>::do_render(VideoPlane* vp, int row, int width, uint32* fbu) noexcept
{
	HorizontalLayout* me = reinterpret_cast<HorizontalLayout*>(vp);

	for (int i = 0; i < N - 1; i++)
	{
		int w = min(me->width[i], width);
		VideoController::render(me->planes[i], row, w, fbu);
		width -= w;
		fbu += w >> zz;
	}
	VideoController::render(me->planes[N - 1], row, width, fbu);
}


template class HorizontalLayout<2>;
template class HorizontalLayout<3>;
template class HorizontalLayout<4>;


} // namespace kio::Video

/*

























*/
