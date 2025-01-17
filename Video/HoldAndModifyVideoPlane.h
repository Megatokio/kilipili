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
class HoldAndModifyVideoPlane : public VideoPlane
{
public:
	Id("HoldAndModify");

	using Pixmap   = Graphics::Pixmap<Graphics::colormode_i8>;
	using Color	   = Graphics::Color;
	using ColorMap = Graphics::ColorMap<Graphics::colordepth_8bpp>;

	HoldAndModifyVideoPlane(const Pixmap*, const ColorMap*, uint first_rel_code);
	virtual ~HoldAndModifyVideoPlane() noexcept override = default;

	virtual void setup(coord _width) override;
	virtual void teardown() noexcept override;
	virtual void vblank() noexcept override;
	virtual void renderScanline(int row, uint32* framebuffer) noexcept override;

	RCPtr<const Pixmap>	  pixmap;
	RCPtr<const ColorMap> colormap;
	uint				  first_rel_code;

	Color		 first_color;
	Color		 border_color;
	const uint8* pixels;	 // next position
	int			 next_row;	 // expected next row
	int			 vga_width;	 // of display/framebuffer, as set with setup()
	int			 vga_height; // of display

	int image_height = 999;
	int image_width; // displayed pixels
	int top_border	= 0;
	int left_border = 0;
	int right_border;
};


} // namespace kio::Video


/*



































*/
