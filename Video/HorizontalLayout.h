// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VideoPlane.h"


namespace kio::Video
{

template<int num_planes>
class HorizontalLayout;


template<>
class HorizontalLayout<2> : public VideoPlane
{
public:
	HorizontalLayout(VideoPlane*, VideoPlane*, int w0);

protected:
	struct Plane
	{
		VideoPlanePtr vp;
		int			  width;
	};

	Plane planes[2];

	static void do_render(VideoPlane*, int row, int width, uint32* fbu) noexcept;
	static void do_vblank(VideoPlane*) noexcept;
};


template<>
class HorizontalLayout<3> : public HorizontalLayout<2>
{
public:
	HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int w0, int w1);

protected:
	Plane more_planes[1];
};


template<>
class HorizontalLayout<4> : public HorizontalLayout<3>
{
public:
	HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int w0, int w1, int w3);

protected:
	Plane more_planes[1];
};


static_assert(sizeof(HorizontalLayout<3>) == sizeof(HorizontalLayout<2>) + 2 * sizeof(ptr));
static_assert(sizeof(HorizontalLayout<4>) == sizeof(HorizontalLayout<3>) + 2 * sizeof(ptr));

extern template class HorizontalLayout<2>;
extern template class HorizontalLayout<3>;
extern template class HorizontalLayout<4>;


HorizontalLayout(VideoPlane*, VideoPlane*, int) -> HorizontalLayout<2>;
HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int, int) -> HorizontalLayout<3>;
HorizontalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int, int, int) -> HorizontalLayout<4>;


} // namespace kio::Video
