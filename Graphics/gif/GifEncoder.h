#pragma once
// Copyright (c) 2007 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


// The LZW encoder and decoder were written
// by Gershon Elber and Eric S. Raymond
// as part of the GifLib package.
//
// To my best knowing the copyright on the algorithm has expired.


#include "../Devices/File.h"
#include "Colormap.h"
#include "Pixelmap.h"


namespace kio::Graphics
{

#define HT_SIZE		8192   /* 13 bit hash table size */
#define HT_KEY_MASK 0x1FFF /* 13 bit key mask */

#define LZ_MAX_CODE 4095 /* Largest 12 bit code */
#define LZ_BITS		12

#define FLUSH_OUTPUT 4096 /* Impossible code = flush */
#define FIRST_CODE	 4097 /* Impossible code = first */
#define NO_SUCH_CODE 4098 /* Impossible code = empty */

#define IMAGE_LOADING  0 /* file_state = processing */
#define IMAGE_SAVING   0 /* file_state = processing */
#define IMAGE_COMPLETE 1 /* finished reading or writing */

#define LOHI(N) uint8(N), uint8((N) >> 8)


/* misc. block flags:
*/
enum // Screen Descriptor:
{
	has_global_cmap	 = 1 << 7,
	glob_cmap_sorted = 1 << 3,
	img_cmap_bits	 = 1 << 4, // bits 4-6
	glob_cmap_bits	 = 1 << 0  // bits 0-2
};
enum // Sub Image Descriptor:
{
	has_local_cmap	= 1 << 7,
	loc_cmap_sorted = 1 << 5,
	rows_interlaced = 1 << 6,
	loc_cmap_bits	= 1 << 0 // bits 0-2
};
enum // Graphic Control Block:  ((Gif89a++))
{
	no_disposal		 = 0 << 2, // don't restore: do as you like..
	keep_new_pixels	 = 1 << 2, // don't restore: keep new pixels
	restore_bgcolor	 = 2 << 2, // restore to background color
	restore_pixels	 = 3 << 2, // restore previous pixels
	no_transparency	 = 0x00,
	has_transparency = 0x01
};


class GifEncoder
{
	using File	  = Devices::File;
	using FilePtr = Devices::FilePtr;

	FilePtr	 fd;
	Colormap global_cmap; // if not used, then global_cmap.used = 0

	int	   depth, clear_code, eof_code, running_code, running_bits, max_code_plus_one;
	int	   current_code, shift_state, file_state, bufsize;
	uint32 shift_data;
	uint8  buf[256];

	uint32 hash_table[HT_SIZE];
	void   clear_hash_table();
	void   add_hash_key(uint32 key, int code);
	int	   lookup_hash_key(uint32 key);

	void write_gif_code(int) throws;
	void write_gif_byte(int) throws;


public:
	GifEncoder()  = default;
	~GifEncoder() = default;

	/*	open file and write gif file header
	*/
	void setFile(FilePtr fd) { this->fd = fd; }
	bool imageInProgress() const { return fd; }

	/*	Clear global cmap and write logical screen descriptor:
		aspect ratio: If the value of the field is not 0,
		this approximation of the aspect ratio is computed based on the formula:
				Aspect Ratio = (pixel_width / pixel_height + 15) / 64
	*/
	void writeScreenDescriptor(uint16 w, uint16 h, uint16 colors, uint8 aspect = 0) throws
	{
		assert(colors > 0);
		global_cmap.purgeCmap();
		uint8 flags = uint8((msbit(uint8(colors - 1))) << 4);
		uint8 bu[]	= {LOHI(w), LOHI(h), flags, 0 /*bgcolor*/, aspect};
		fd->write(ptr(bu), sizeof(bu));
	}

	/*	Set global cmap, write logical screen descriptor and global cmap:
	*/
	void writeScreenDescriptor(uint16 w, uint16 h, const Colormap& cmap, uint8 aspect = 0) throws
	{
		assert(cmap.usedColors() > 0);
		global_cmap = cmap;
		uchar bits	= uchar(cmap.usedBits() - 1);
		uchar flags = has_global_cmap + (bits << 4) + (bits << 0);
		uchar bu[]	= {LOHI(w), LOHI(h), flags, 0 /*bgcolor*/, aspect};
		fd->write(ptr(bu), sizeof(bu));
		writeColormap(cmap);
	}

	/*	FinishImage()  (if not yet done),
		write comment block with creator message,
		write trailer byte and close file
	*/
	void closeFile() throws;


	/*	Compression of image pixel stream:
	*/
	void startImage(int bits_per_pixel) throws;
	void writePixelRow(cuptr pixel, uint w) throws;
	void writePixelRect(cuptr pixel, uint w, uint h, uint dy = 0) throws;
	void finishImage() throws;
	void writePixelmap(int bits_per_pixel, const Pixelmap& pixel) throws;


	/*	Write different kinds of blocks:
	*/
	void writeGif87aHeader() throws { fd->write("GIF87a", 6); }
	void writeGif89aHeader() throws { fd->write("GIF89a", 6); }
	void writeGifTrailer() throws { fd->write<uchar>(0x3B); }


	/*	Write descriptor for following sub image:
	*/
	void writeImageDescriptor(const Pixelmap& pm, uint8 flags = 0);
	void writeImageDescriptor(const Pixelmap& pm, const Colormap& cmap);

	/*	Write colormap
		Colormap size must be 2^n colors (1≤n≤8)
		((not tested. will silently write nothing for cmap_size==0.))
	*/
	void writeColormap(const Colormap& cmap) throws;

	/*	Enable looping animations.								GIF89a++
	*/
	void writeLoopingAnimationExtension(uint16 max_loops = 0) throws
	{
		// clang-format off
		uint8 bu[] = { 0x21, 0xff, 11, 'N','E','T','S','C','A','P','E','2','.','0',
					   3 /*sub block size*/, 1, LOHI(max_loops), 0 };
		// clang-format on
		fd->write(ptr(bu), sizeof(bu));
	}

	/*	Write Comment Block	GIF89a++
	*/
	void writeCommentBlock(cstr comment) throws
	{
		uint  n	   = min(255u, uint(strlen(comment)));
		uchar bu[] = {0x21, 0xfe, uint8(n)};
		fd->write(ptr(bu), 3);
		fd->write(comment, n + 1);
	}

	/*	Write Graphic Control Block: GIF89++
		Animation delay and transparent color.
	*/
	void writeGraphicControlBlock(uint16 delay /*1/100sec*/, int transp_index = Colormap::unset) throws
	{
		uchar flags = keep_new_pixels + (transp_index != Colormap::unset);
		uchar bu[]	= {0x21, 0xf9, 4, flags, LOHI(delay), uint8(transp_index), 0};
		fd->write(ptr(bu), sizeof(bu));
	}

	/*	Write sub image to gif file
		use global_cmap or include a local table
	*/
	void writeImage(Pixelmap& pm, const Colormap& cm) throws;
	void writeImage(Pixelmap& pm) throws { writeImage(pm, global_cmap); }
};

} // namespace kio::Graphics

/*






























*/
