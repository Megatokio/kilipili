// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "ColorMap.h"
#include "Pixmap.h"
#include "VideoPlane.h"

namespace kio::Video
{

/*
	The HoldAndModifyVideoPlane is an 8 bit indexed color FrameBuffer
	which uses part of it's colormap for relative color offset codes.
	This is only a useful mode if sizeof(Color) != 1.
	RGB images can be encoded for this color mode with desktop_tools/rsrc_writer.
*/
class HamImageVideoPlane : public VideoPlane
{
public:
	Id("HoldAndModify");

	using Pixmap   = Graphics::Pixmap<Graphics::colormode_i8>;
	using Color	   = Graphics::Color;
	using ColorMap = Graphics::ColorMap<Graphics::colordepth_8bpp>;

	HamImageVideoPlane(const Pixmap*, const ColorMap*, uint first_rel_code);
	virtual ~HamImageVideoPlane() noexcept override = default;

	virtual void setup() override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* framebuffer) noexcept override;

	void setupNextImage(int width, int height, uint first_rel_code);

	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;
	uint				  first_rel_code;

	Color		 first_color;
	Color		 border_color;
	const uint8* pixels;   // next position
	int			 next_row; // expected next row

	int image_height = 999;
	int image_width; // displayed pixels
	int row_offset;
	int top_border	= 0;
	int left_border = 0;
	int right_border;

private:
	void _setup(int w, int h, uint num_abs_codes);
};


} // namespace kio::Video


/*



































*/
