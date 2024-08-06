// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "Pixmap.h"
#include "VideoPlane.h"

namespace kio::Video
{

/*
	A VideoPlane which displays a half-size FrameBuffer with simulated increased color depth.
*/

class FrameBufferHQi8 : public VideoPlane
{
public:
	using Pixmap = Graphics::Pixmap<Graphics::colormode_i8>;

	RCPtr<Pixmap> pixmap;
	Color		  cmaps[2][256 * 2]; // 2 2-pixel-colormaps for even and odd row
	uint		  width;

	FrameBufferHQi8(RCPtr<Pixmap> px, uint8 colormap_rgb888[256][3]);
	~FrameBufferHQi8() override = default;

	virtual void setup(coord width) override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* buffer) noexcept override;

	void update_colormap(uint8 colormap_rgb888[256][3]);
};


} // namespace kio::Video
