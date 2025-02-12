// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VideoPlane.h"


namespace kio ::Video
{


template<int num_planes>
class VerticalLayout : public VideoPlane
{
public:
	VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int h0, int h1);
	VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int h0, int h1, int h3);

protected:
	RCPtr<VideoPlane> planes[num_planes];
	int				  bottom[num_planes];
	int				  idx;

	static void do_render(VideoPlane*, int row, int width, uint32* fbu) noexcept;
	static void do_vblank(VideoPlane*) noexcept;
};


template<>
class VerticalLayout<2> : public VideoPlane
{
public:
	VerticalLayout(VideoPlane*, VideoPlane*, int height);

protected:
	RCPtr<VideoPlane> plane1, plane2;
	int				  height;

	static void do_render(VideoPlane*, int row, int width, uint32* fbu) noexcept;
	static void do_vblank(VideoPlane*) noexcept;
};


template<>
VerticalLayout<3>::VerticalLayout(VideoPlane* p0, VideoPlane* p1, VideoPlane* p2, int h0, int h1);
template<>
VerticalLayout<4>::VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int, int, int);


extern template class VerticalLayout<2>;
extern template class VerticalLayout<3>;
extern template class VerticalLayout<4>;


VerticalLayout(VideoPlane*, VideoPlane*, int) -> VerticalLayout<2>;
VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int, int) -> VerticalLayout<3>;
VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int, int, int) -> VerticalLayout<4>;

} // namespace kio::Video
