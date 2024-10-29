// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ImageDecoder.h"
#include "Pixmap.h"

namespace kio::Graphics
{

ImageDecoder::ImageDecoder(FilePtr file) : file(file)
{
	constexpr uint headersize = 9;
	if (file->getSize() - file->getFpos() < headersize + 1) return; // avoid eof error
	if (file->read_uint32() != magic) return;
	uint8 colormodel = file->read_uint8();
	image_width		 = file->read_uint16();
	image_height	 = file->read_uint16();
	has_cmap		 = colormodel & 4;
	has_transparency = colormodel & 8;
	colormodel &= 0xf3;
	if (colormodel >= no_img_file) return;		// invalid colormodel
	if (has_cmap && colormodel == grey) return; // invalid combination

	uint16 colorsize = colormodel == grey ? 1 : colormodel == rgb ? 3 : sizeof(Color);
	pixelsize		 = has_cmap ? 1 : colorsize;
	buffersize		 = uint16(image_width * pixelsize);
	if (has_cmap) cmapsize = file->read_uint8() + 1;

	if (file->getSize() - file->getFpos() < cmapsize * colorsize + buffersize * uint(image_height)) return;

	this->colormodel = ColorModel(colormodel); // -> isa_img_file
}

ImageDecoder::~ImageDecoder()
{
	assert(rc == 0);
	delete[] global_cmap;
	delete[] global_cmap_rgb;
	delete[] scanlinebuffer;
}

void ImageDecoder::read_cmap()
{
	global_cmap = new Color[cmapsize];
	if (colormodel == hw_color) file->read(global_cmap, cmapsize * sizeof(Color));
	else // colormodel = rgb
	{
		global_cmap_rgb = new uchar[cmapsize * 3];
		file->read(global_cmap_rgb, cmapsize * 3);
		uint8* p = global_cmap_rgb;
		for (uint i = 0; i < cmapsize; i++) { global_cmap[i] = Color::fromRGB8(p[i * 3], p[i * 3 + 1], p[i * 3 + 2]); }
	}
}

static void store_scanline_cmap(Canvas& pm, int x, int y, int w, uint8* pixels, bool has_transparency) noexcept
{
	// the pm must be indexed_color, but the pixel size may be less than 8 bit.
	// if the pixel size is to small then wrong colors for the cut off indexes are displayed.
	// TODO support direct_color Pixmap

	if (pm.colormode == colormode_i8 && !has_transparency)
	{
		Pixmap<colormode_i8> row(w, 1, pixels, 0);
		pm.copyRect(x, y, row);
	}
	else
	{
		uint transp = has_transparency ? 0x00 : 0x100;
		for (int i = 0; i < w; i++)
		{
			uint ci = pixels[i];
			if (ci != transp) pm.set_pixel(x + i, y, ci);
		}
	}
};

static void store_scanline_hw_color(Canvas& pm, int x, int y, int w, uint8* pixels, bool has_transparency) noexcept
{
	// the pixmap must be direct color with pixels the same size as Color.
	assert(pm.colormode == colormode_rgb);

	if (!has_transparency)
	{
		Pixmap<colormode_rgb> row(w, 1, pixels, 0);
		pm.copyRect(x, y, row);
	}
	else
	{
		Color*		   hw_pixels = reinterpret_cast<Color*>(pixels);
		constexpr uint transp	 = 0x00;
		for (int i = 0; i < w; i++)
		{
			Color c = hw_pixels[i];
			if (c.raw != transp) pm.set_pixel(x + i, y, c);
		}
	}
};

static void store_scanline_rgb(Canvas& pm, int x, int y, int w, uint8* pixels, bool has_transparency) noexcept
{
	// the pixmap must be direct color with pixels the same size as Color.
	assert(pm.colormode == colormode_rgb);

	for (int i = 0; i < w; i++)
	{
		uint  transp = has_transparency ? 0x00 : 0x100;
		uint8 r		 = pixels[3 * i];
		uint8 g		 = pixels[3 * i + 1];
		uint8 b		 = pixels[3 * i + 2];
		if ((r | g | b) != transp) pm.set_pixel(x + i, y, Color::fromRGB8(r, g, b));
	}
};

static void store_scanline_grey(Canvas& pm, int x, int y, int w, uint8* pixels, bool has_transparency) noexcept
{
	// the pixmap must be direct color with pixels the same size as Color.
	assert(pm.colormode == colormode_rgb);

	for (int i = 0; i < w; i++)
	{
		uint  transp = has_transparency ? 0x00 : 0x100;
		uint8 c		 = pixels[i];
		if (c != transp) pm.set_pixel(x + i, y, Color::fromGrey8(c));
	}
};

int ImageDecoder::decode_image(Canvas& pm, store_scanline_internal& store, int x0, int y0)
{
	int	   w	  = image_width;
	uint8* pixels = scanlinebuffer;
	if (x0 < 0) { pixels -= x0 * pixelsize, w += x0, x0 = 0; }
	if (x0 + w > pm.width) w = pm.width - x0;
	if (w <= 0) return 0;

	for (int y = 0; y < image_height; y++)
	{
		file->read(scanlinebuffer, buffersize);
		if (uint(y0 + y) < uint(pm.height)) store(pm, x0, y0 + y, w, pixels, has_transparency);
	}

	return 0;
}

void ImageDecoder::decodeImage(Canvas& pm, int x0, int y0)
{
	if (has_cmap) read_cmap();
	scanlinebuffer = new uint8[buffersize];

	if (has_cmap)
	{
		assert(is_indexed_color(pm.colormode)); // else TODO
		assert(pixelsize == 1);
		decode_image(pm, store_scanline_cmap, x0, y0);
	}
	else
	{
		assert(pm.colormode == colormode_rgb); // else TODO

		if (colormodel == hw_color)
		{
			assert(pixelsize == sizeof(Color));
			decode_image(pm, store_scanline_hw_color, x0, y0);
		}
		else if (colormodel == rgb)
		{
			assert(pixelsize == 3);
			decode_image(pm, store_scanline_rgb, x0, y0);
		}
		else // grey
		{
			assert(pixelsize == 1);
			decode_image(pm, store_scanline_grey, x0, y0);
		}
	}
}

void ImageDecoder::decodeImage(store_scanline& store, int x0, int y0)
{
	// decode the image using a custom scanline storage function.
	// this can be used to load into unusual Pixmaps or HQ rendering.
	// the image is placed at position x0,y0 in the Pixmap

	if (has_cmap) read_cmap();
	scanlinebuffer = new uint8[buffersize];

	for (int y = 0; y < image_height; y++)
	{
		file->read(scanlinebuffer, buffersize);
		store(x0, y0 + y, image_width, scanlinebuffer);
	}
}


} // namespace kio::Graphics


/*































*/
