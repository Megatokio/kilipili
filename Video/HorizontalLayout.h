// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VideoPlane.h"


namespace kio ::Video
{


template<int num_planes>
class HorizontalLayout : public VideoPlane
{
public:
	HorizontalLayout(VideoPlane*, VideoPlane*, int w0);
	HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int w0, int w1);
	HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int w0, int w1, int w3);

protected:
	RCPtr<VideoPlane> planes[num_planes];
	int				  width[num_planes];

	static void do_render(VideoPlane*, int row, int width, uint32* fbu) noexcept;
	static void do_vblank(VideoPlane*) noexcept;
};


template<>
HorizontalLayout<2>::HorizontalLayout(VideoPlane* p0, VideoPlane* p1, int w0);
template<>
HorizontalLayout<3>::HorizontalLayout(VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, int w0, int w1);
template<>
HorizontalLayout<4>::HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int, int, int);


extern template class HorizontalLayout<2>;
extern template class HorizontalLayout<3>;
extern template class HorizontalLayout<4>;


HorizontalLayout(VideoPlane*, VideoPlane*, int) -> HorizontalLayout<2>;
HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int, int) -> HorizontalLayout<3>;
HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int, int, int) -> HorizontalLayout<4>;


} // namespace kio::Video
