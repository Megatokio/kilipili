// Copyright (c) 2023 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Color.h"
#include "VideoPlane.h"

namespace kio::Video
{

class UniColorBackdrop : public VideoPlane
{
public:
	UniColorBackdrop(Graphics::Color color) noexcept;

private:
	static void render(VideoPlane*, int row, int width, uint32* fbu) noexcept;

	uint32 color;
};

} // namespace kio::Video
