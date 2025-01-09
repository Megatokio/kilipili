// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Devices/File.h"
#include "Graphics/Color.h"
#include "common/Array.h"
#include "common/standard_types.h"
#include <cstdio>

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

namespace kio
{

class ImageFileWriter
{
public:
	using FilePtr = Devices::FilePtr;
	using File	  = Devices::File;

	static constexpr uint32 magic = 0xd7e3bc09;

	ImageFileWriter(bool use_hw_color, bool with_transparency = true) :
		use_hw_color(use_hw_color),
		with_transparency(with_transparency)
	{}
	~ImageFileWriter();
	void   importFile(cstr infile);
	uint32 exportImgFile(FilePtr);
	uint32 exportImgFile(cstr fpath, uint8 wbits, uint8 lbits);
	uint32 exportRsrcFile(cstr hdr_fpath, cstr rsrc_fpath, uint8 wbits, uint8 lbits);

	const bool use_hw_color;
	const bool with_transparency;

	int	   num_channels = 0;
	int	   image_width	= 0;
	int	   image_height = 0;
	uint8* data			= nullptr;

	enum ColorModel { grey, rgb, hw_color };
	ColorModel colormodel;
	bool	   has_cmap;
	bool	   has_transparency;

	uint sizeof_pixel = 0; // 1,2,3
	uint sizeof_color = 0; // 1,2

	Array<uint32> cmap;

private:
	void scan_img_data();
	void store_cmap_color(File*, uint32); // clut
	void store_pixel(File*, uint8*);

	//void store_byte(File*, uint8);
	//void store(File*, const void*, uint cnt);
	void store(File*, Graphics::Color);
};

} // namespace kio


inline cstr tostr(kio::ImageFileWriter::ColorModel c)
{
	if (c == kio::ImageFileWriter::ColorModel::grey) return "grey";
	if (c == kio::ImageFileWriter::ColorModel::rgb) return "rgb";
	else return "hwcolor";
}


/*



































*/
