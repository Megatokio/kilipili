// Copyright (c) 2024 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "ColorMap.h"
#include "Pixmap.h"
#include "VideoPlane.h"

namespace kilipili
{

/*
	The HoldAndModifyVideoPlane is an 8 bit indexed color FrameBuffer
	which uses part of it's colormap for relative color offset codes.
	This is only a useful mode if sizeof(Color) > 1.
	RGB images can be encoded for this color mode with desktop_tools/rsrc_writer.
*/
class HamImageVideoPlane final : public VideoPlane
{
public:
	HamImageVideoPlane(const Pixmap_i8*, uint16 first_rel_code);

	void setupNextImage(int row_offset, uint16 first_rel_code);

	RCPtr<const Pixmap_i8>	 pixmap;
	RCPtr<const ColorMap_i8> colormap;

	int			 row_offset;	  //
	const uint8* pixels;		  // next position
	const Color* colors;		  //
	const Color* first_rel_color; //
	uint16		 first_rel_code;  //
	Color		 first_color;	  // initial color at start of next row

private:
	static void do_render(VideoPlane*, int row, int width, uint32* fbu) noexcept;
	static void do_vblank(VideoPlane*) noexcept;
};


} // namespace kilipili


/*



































*/
