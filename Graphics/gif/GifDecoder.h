#pragma once
// Copyright (c) 2007 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


/*	The LZW encoder and decoder were written
	by Gershon Elber and Eric S. Raymond
	as part of the GifLib package.

	To my best knowing the copyright never extended on the decoder.
*/

#include "../Devices/File.h"
#include "Canvas.h"
#include "Video/Color.h"
#include "Video/video_types.h"
#include "standard_types.h"
#include <functional>

namespace kio::Graphics
{
static constexpr int  LZ_BITS	   = 12;
static constexpr uint LZ_SIZE	   = 1 << LZ_BITS;
static constexpr uint LZ_MAX_CODE  = 4095; // Largest 12 bit code
static constexpr uint FLUSH_OUTPUT = 4096; // Impossible code = flush
static constexpr uint FIRST_CODE   = 4097; // Impossible code = first
static constexpr uint NO_SUCH_CODE = 4098; // Impossible code = empty


using store_scanline = std::function<void(int x, int y, int w, uchar* pixels, Video::Color* cmap, int transp_color)>;


class GifDecoder
{
public:
	using File	  = Devices::File;
	using FilePtr = Devices::FilePtr;
	using Color	  = Video::Color;

	/* ctor: after this check `isa_gif_file`.
	   create pixmap with screen_width, screen_height and total_color_bits.
	   clear image to global_cmap[background_color].
	*/
	GifDecoder(FilePtr file);
	~GifDecoder();

	/* decode image (one frame) and call `store_scanline` to store the pixels.
	   decoded scanlines may be partial and come in random order.
	   return delay 1/100s until next frame or 0 if this was the last frame.
	   if not the last frame inspect
		- loop_count:	   is the image looping? (caller may reset the counter to prevent looping)
		- disposal_method: what to do with the current frame before calling `decode_frame` again:
						   2: clear to background color.  
						   3: restore previous contents. (good luck with that)
						   bounding box = xpos, ypos, width, height.
						   set disposal = 0 afterwards else decode_frame() will clear it on next call
	*/
	int	 decode_frame(store_scanline&);
	int	 decode_frame(Canvas&, int x0 = 0, int y0 = 0);											// true color pixmap
	void decode_image(Canvas&, int x0 = 0, int y0 = 0);											// true color pixmap
	int	 decode_frame(Canvas&, Color* cmap_out, int x0 = 0, int y0 = 0);						// index color pixmap
	void decode_image(Canvas&, Color* cmap_out, int x0 = 0, int y0 = 0);						// index color pixmap
	int	 decode_frame(Canvas&, Color* cmap_out, int* transp_color_out, int x0 = 0, int y0 = 0); // index color pixmap
	void decode_image(Canvas&, Color* cmap_out, int* transp_color_out, int x0 = 0, int y0 = 0); // index color pixmap

	uint16 image_width		 = 0;
	uint16 image_height		 = 0;
	bool   isa_gif_file		 = false;
	uint8  background_color	 = 0;
	uint8  total_color_bits	 = 0;
	uint8  global_cmap_bits	 = 0;
	Color* global_cmap		 = nullptr;
	uint8* pixels			 = nullptr;
	str	   comment			 = nullptr; // if a comment was found.
	uint32 loop_reset_fpos	 = 0;		// if looping extension found
	uint16 loop_count		 = 0;		// if looping extension found
	int16  transparent_color = -1;		// from animation control block
	uint16 global_cmap_used	 = 0;
	uint8  disposal_method	 = 0;
	char   aspect			 = 0;
	uint16 xpos, ypos, width, height;

private:
	void lz_initialize();
	void lz_read_scanline(uchar* scanline, int length);
	void finish();

	FilePtr file;
	uint	clear_code, eof_code, running_code, prev_code, max_code_plus_one;
	int		depth, stack_ptr, shift_state, running_bits;
	uint	position, bufsize;
	uint32	shift_data;

	uchar	buf[256];
	uchar*	stack  = nullptr; //[LZ_SIZE];
	uchar*	suffix = nullptr; //[LZ_SIZE];
	uint16* prefix = nullptr; //[LZ_SIZE];

	uint   read_gif_code();
	uchar  read_gif_byte();
	Color* read_cmap(uint8 bits, uint maxbits = 0);
};


} // namespace kio::Graphics


/*


































*/
