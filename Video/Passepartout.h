// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VideoPlane.h"


namespace kio::Video
{

class Passepartout final : public VideoPlane
{
public:
	Passepartout(RCPtr<VideoPlane>, int inner_width, int inner_height);
	Passepartout(RCPtr<VideoPlane>, int width, int height, int inner_width, int inner_height);

	virtual void setup() override;
	virtual void teardown() noexcept override;

	void setInnerSize(int inner_width, int inner_height) noexcept;
	void setSize(int width, int height) noexcept;
	void setSize(int width, int height, int inner_width, int inner_height) noexcept;

private:
	RCPtr<VideoPlane> vp;

	uint16 width, height;
	int	   inner_width; // measured in uint32
	int	   inner_height;
	int	   top;

	static void do_render(VideoPlane*, int row, int width, uint32* fbu) noexcept;
	static void do_vblank(VideoPlane*) noexcept;
};


} // namespace kio::Video

/*














*/
