// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "../Devices/File.h"
#include "Graphics/Canvas.h"
#include "Graphics/Color.h"
#include "standard_types.h"
#include <functional>

namespace kio::Graphics
{

/*	Image file format:

	colors are 1, 2 or 3 bytes: grey, hw_color or RGB
	pixels are 1, 2 or 3 bytes: color or color index
	if has_transp = 1 then pixel = 0 -> transparent
	if has_transp && sizeof_clut = 256 then the clut is reordered
		so that the darkest color value is at position 0.
	if has_transp && use_hw_color && Color has no free bits -> use closest_to_black() for black
		
file:	
	uint32 	magic				0xd7e3bc09
	uint8	colormodel
	uint2	width
	uint2	height
	uint8 	sizeof_clut - 1		only if has_clut = 1
	uint8	clut[]				only if has_clut = 1
	uint8	pixels[]			width*height*pixel_size
	
	uint8 colormodel = 0b0000tcmm
		mm	00 = grey8				
			01 = rgb888				
			10 = hw_color
			11 = invalid
		c	1  = has_cmap
		t	1  = has_transp
*/

class ImageDecoder
{
public:
	uint8 rc = 0; // --> can use RCPtr

	using File	  = Devices::File;
	using FilePtr = Devices::FilePtr;

	using store_scanline = std::function<void(int x, int y, int w, uint8* pixeldata)>;

	static constexpr uint32 magic = 0xd7e3bc09;

	/* ctor: create pixmap with screen_width, screen_height, 
			 either colormode_rgb or indexed color with large enough size.
			 test isa_img_file() before proceeding.
	*/
	ImageDecoder(FilePtr file);
	~ImageDecoder();

	void decodeImage(store_scanline&, int x0 = 0, int y0 = 0);
	void decodeImage(Canvas&, int x0 = 0, int y0 = 0);

	bool isa_img_file() const noexcept { return colormodel < no_img_file; }

	enum ColorModel : uint8 { grey, rgb, hw_color, no_img_file };
	ColorModel colormodel = no_img_file;

	bool has_transparency = false; // transparent pixels are 0xff or 0xffff
	bool has_cmap		  = false;

	int image_width	 = 0;
	int image_height = 0;

	Color* global_cmap	   = nullptr; // allocated
	uint8* global_cmap_rgb = nullptr; // allocated, only if colormode = rgb
	uint16 cmapsize		   = 0;
	uint16 pixelsize	   = 0;
	uint   buffersize	   = 0;
	uint8* scanlinebuffer  = nullptr;

private:
	FilePtr file;

	using store_scanline_internal = void(Canvas&, int x, int y, int w, uint8* pixels, bool transp);
	int	 decode_image(Canvas&, store_scanline_internal& store, int x0 = 0, int y0 = 0);
	void read_cmap();
};


} // namespace kio::Graphics
