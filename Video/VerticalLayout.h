// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VideoPlane.h"


namespace kio ::Video
{

template<int num_planes>
class VerticalLayout;


template<>
class VerticalLayout<2> : public VideoPlane
{
public:
	VerticalLayout(VideoPlane*, VideoPlane*, int height);

protected:
	VerticalLayout(VideoPlane*, VideoPlane*, int h0, int h1);

	static void do_render(VideoPlane*, int row, int width, uint32* fbu) noexcept;
	static void do_vblank(VideoPlane*) noexcept;

	struct Plane
	{
		VideoPlanePtr vp;
		int			  height;
	};

	int	  idx = 0;
	int	  top = 0;
	Plane planes[2];
};


template<>
class VerticalLayout<3> : public VerticalLayout<2>
{
public:
	VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int h0, int h1);

protected:
	VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int h0, int h1, int h2);
	Plane more_planes;
};


template<>
class VerticalLayout<4> : public VerticalLayout<3>
{
public:
	VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int h0, int h1, int h3);

protected:
	Plane more_planes;
};


static_assert(sizeof(VerticalLayout<3>) == sizeof(VerticalLayout<2>) + 2 * sizeof(ptr));
static_assert(sizeof(VerticalLayout<4>) == sizeof(VerticalLayout<3>) + 2 * sizeof(ptr));

extern template class VerticalLayout<2>;
extern template class VerticalLayout<3>;
extern template class VerticalLayout<4>;


VerticalLayout(VideoPlane*, VideoPlane*, int) -> VerticalLayout<2>;
VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, int, int) -> VerticalLayout<3>;
VerticalLayout(VideoPlane*, VideoPlane*, VideoPlane*, VideoPlane*, int, int, int) -> VerticalLayout<4>;

} // namespace kio::Video
