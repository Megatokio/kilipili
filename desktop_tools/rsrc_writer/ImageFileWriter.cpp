// Copyright (c) 2024 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ImageFileWriter.h"
#include "CompressedRsrcFileWriter.h"
#include "cstrings.h"
#include PICO_BOARD
#include "Graphics/Color.h"
#include "common/Array.h"
//#include"common/basic_math.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_FAILURE_USERMSG 1
#include "extern/stb/stb_image.h"

#ifndef RSRC_MAX_IMG_WIDTH
  #define RSRC_MAX_IMG_WIDTH 1024
#endif
#ifndef RSRC_MAX_IMG_HEIGHT
  #define RSRC_MAX_IMG_HEIGHT RSRC_MAX_IMG_WIDTH
#endif


namespace kio
{
using namespace Graphics;

ImageFileWriter::~ImageFileWriter()
{
	if (file) fclose(file);
	if (rsrc_writer) delete rsrc_writer;
}

static inline bool is_transparent(uint8 c) { return c < 128; }
static inline bool is_opaque(uint8 c) { return c >= 128; }

void ImageFileWriter::scan_img_data()
{
	// create clut[] (or bail out if too many colors and direct color required)
	// detect use of transparency
	// colors in clut are 0x00bbggrr or 0xgg

	has_cmap		 = false;
	has_transparency = false;
	int	 num_pixels	 = image_width * image_height;
	bool with_alpha	 = with_transparency && (num_channels & 1) == 0;

	if (num_channels <= 2)
	{
		colormodel	 = grey;
		sizeof_color = 1;
		sizeof_pixel = 1;

		if (with_alpha) // has_transp ?
		{
			for (uchar* p = data; p <= data + num_pixels * 2; p += 2)
			{
				if (is_opaque(p[1])) continue;
				has_transparency = true;
				return;
			}
		}
		return;
	}

	cmap.purge();
	cmap.growmax(256 + 1);

	for (uchar* p = data; cmap.count() <= 256 && p < data + num_pixels * num_channels; p += num_channels)
	{
		if (with_alpha && is_transparent(p[3]))
		{
			has_transparency = true;
			continue;
		}
		int32 pixel = (p[0] << 16) + (p[1] << 8) + (p[2] << 0); // r,g,b
		cmap.appendifnew(pixel);
	}

	has_cmap = cmap.count() <= 256;
	if (!has_cmap) cmap.purge();

	colormodel	 = use_hw_color ? hw_color : rgb;
	sizeof_color = use_hw_color ? sizeof(Color) : 3;
	sizeof_pixel = has_cmap ? 1 : sizeof_color;

	if (has_transparency)
	{
		if (cmap.count() < 256)
		{
			cmap.append(cmap[0]);
			cmap[0] = int32(0xff000000); // black, but not black
		}
		else // clut full => use darkest color for transp. ((must be a png, can't be gif))
		{
			uint		transp_index = 0;
			int			dist		 = 999999;
			static auto brightness	 = [](int32 c) {
				  return ((c >> 16) & 0xff) * 4 + ((c >> 8) & 0xff) * 5 + (c & 0xff) * 3;
			};
			for (uint i = 0; i < cmap.count(); i++)
			{
				int c = cmap[i];
				int d = brightness(c);
				if (d < dist) transp_index = i, dist = d;
			}
			std::swap(cmap[transp_index], cmap[0]);
		}
	}
}

void ImageFileWriter::importFile(cstr infile)
{
	data = stbi_load(infile, &image_width, &image_height, &num_channels, 0);
	if (!data) throw stbi_failure_reason();
	if (image_width > RSRC_MAX_IMG_WIDTH || image_height > RSRC_MAX_IMG_HEIGHT) throw "image too big";
	scan_img_data();
	assert(colormodel <= 3);
	assert(!(colormodel == grey && has_cmap));
	assert(!has_transparency || with_transparency);
}

uint32 ImageFileWriter::exportImgFile(cstr outfile)
{
	file = fopen(outfile, "wb");
	if (!file) throw "could not open output file";

	// store header
	uint32 n = htole32(magic);
	fwrite(&n, 4, 1, file);
	uint8 cm = uint8(colormodel + (has_cmap << 2) + (has_transparency << 3));
	fputc(cm, file);
	uint16 sz[2] = {htole16(image_width), htole16(image_height)};
	fwrite(&sz, 2, 2, file);
	if (has_cmap) fputc(int(cmap.count()) - 1, file);
	for (uint i = 0; i < cmap.count(); i++) { store_cmap_color(cmap[i]); }

	uint8* p = data;
	for (int i = 0; i < image_width * image_height; i++)
	{
		store_pixel(p);
		p += num_channels;
	}

	uint32 fsize = uint32(ftell(file));
	fclose(file);
	file = nullptr;
	return fsize;
}

uint32 ImageFileWriter::exportRsrcFile(cstr hdr_fpath, cstr rsrc_fname, uint8 wsize, uint8 lsize)
{
	rsrc_writer = new CompressedRsrcFileWriter(hdr_fpath, rsrc_fname, wsize, lsize);

	// store header
	rsrc_writer->store(magic);
	uint8 cm = uint8(colormodel + (has_cmap << 2) + (has_transparency << 3));
	rsrc_writer->store_byte(cm);
	uint16 sz[2] = {htole16(image_width), htole16(image_height)};
	rsrc_writer->store(&sz, 4);
	if (has_cmap) rsrc_writer->store_byte(uint8(cmap.count() - 1));
	for (uint i = 0; i < cmap.count(); i++) { store_cmap_color(cmap[i]); }

	uint8* p = data;
	for (int i = 0; i < image_width * image_height; i++)
	{
		store_pixel(p);
		p += num_channels;
	}

	uint32 fsize = rsrc_writer->close();
	delete rsrc_writer;
	rsrc_writer = nullptr;
	return fsize;
}

void ImageFileWriter::store_byte(uint8 b)
{
	if (file) fputc(b, file);
	else rsrc_writer->store_byte(b);
}

void ImageFileWriter::store(const void* p, uint cnt)
{
	if (file) fwrite(p, 1, cnt, file);
	else rsrc_writer->store(p, cnt);
}

void ImageFileWriter::store(Color color)
{
	if constexpr (sizeof(Color) == 1)
	{
		store_byte(uint8(color.raw)); //
	}
	else
	{
		uint16 n = htole16(color.raw);
		store(&n, 2);
	}
}

void ImageFileWriter::store_cmap_color(int32 c)
{
	if (colormodel == rgb)
	{
		uint32 n = htobe32(c << 8); // high byte first, highest byte = red
		store(&n, 3);
	}
	else // hw_color
	{
		assert(colormodel == hw_color);
		store(Color::fromRGB8(uint32(c)));
	}
}

//Graphics::Color make_opaque(Graphics::Color c)
//{
//	static_assert(Color::gbits >= Color::rbits && Color::rbits >= Color::bbits);
//	if constexpr (Color::ibits) c.raw -= 1 << Color::ishift;					  // has common low bits => nip i
//	else if constexpr (Color::bbits == Color::gbits) c.raw -= 1 << Color::bshift; // all same size => nip blue
//	else if constexpr (Color::rbits == Color::gbits) c.raw -= 1 << Color::rshift; // else r==g => nip red
//	else c.raw -= 1 << Color::gshift;											  // g is largest => nip green
//	return c;
//}

static constexpr bool			has_free_bits() noexcept { return Color::total_colorbits < sizeof(Color) * 8; }
static constexpr __unused Color non_zero_black() noexcept { return ~Color::total_colormask; } // if free bits available
static constexpr __unused Color closest_to_black() noexcept
{
	static_assert(Color::gbits >= Color::rbits && Color::rbits >= Color::bbits);
	if constexpr (Color::ibits) return 1 << Color::ishift;				   // have common low bits => lowest ibit
	if constexpr (Color::bbits == Color::gbits) return 1 << Color::bshift; // all same size => lowest blue bit
	if constexpr (Color::rbits == Color::gbits) return 1 << Color::rshift; // red == green => lowest red bit
	else return 1 << Color::gshift;										   // green is largest => lowest green bit
}


void ImageFileWriter::store_pixel(uint8* p)
{
	if (num_channels <= 2) // grey
	{
		assert(colormodel == grey);
		assert(sizeof_pixel == 1);
		assert(!has_cmap);

		uint8 n = p[0];
		if (has_transparency) n = is_transparent(p[1]) ? 0 : n == 0 ? 1 : n; // grey=1 -> closest to black
		store_byte(n);
	}
	else // rgb or hw_color, clut or no clut:
	{
		if (has_transparency && is_transparent(p[3])) // transparent pixel:
		{
			// transparency is indicated by a value of 0 in all modes:
			// color_index=0, grey=0, rgb=(0,0,0), Color(0)
			int32 transp_pixel = 0;
			store(&transp_pixel, sizeof_pixel);
		}
		else // opaque pixel:
		{
			uint8 r = p[0];
			uint8 g = p[1];
			uint8 b = p[2];
			if (has_cmap)
			{
				int32 color = (r << 16) + (g << 8) + (b << 0);
				uint  i		= cmap.indexof(color);
				assert(i < cmap.count()); // all pixels' colors must be in clut[]
				store_byte(uint8(i));
			}
			else if (colormodel == rgb) // opaque pixel, rgb, no cmap
			{
				if unlikely (has_transparency && (r + g + b == 0)) p[2] = 1; // blue=1 -> closest_to_black
				store(p, 3);
			}
			else // opaque pixel, hw_color, no cmap
			{
				Color c = Color::fromRGB8(r, g, b);
				if (c.raw != 0 || !has_transparency) store(c);
				else if constexpr (has_free_bits()) store(non_zero_black());
				else store(closest_to_black());
			}
		}
	}
}


} // namespace kio


/*




























*/