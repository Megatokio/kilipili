// Copyright (c) 2007 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

// The LZW encoder and decoder were written
// by Gershon Elber and Eric S. Raymond
// as part of the GifLib package.
//
// To my best knowing the copyright never extended on the decoder.

#include "GifDecoder.h"
#include "Video/Color.h"
#include "basic_math.h"
#include <cstrings.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
namespace kio
{
extern void print_heap_free(int r = 0);
}
//#define IMAGE_LOADING  0 /* file_state = processing */
//#define IMAGE_SAVING   0 /* file_state = processing */
//#define IMAGE_COMPLETE 1 /* finished reading or writing */


namespace kio::Graphics
{
struct _Lzw
{
	int16 prefix;
	uchar first;
	uchar suffix;
};

struct _Gif
{
	int	   w, h;
	uchar* out;		   // output buffer (always 4 components)
	uchar* background; // The current "background" as far as a gif is concerned
	int	   flags, bgindex, ratio, transparent, eflags;
	_Lzw   codes[8192];
	uchar* color_table;
	int	   parse, step;
	int	   start_x, start_y;
	int	   max_x, max_y;
	int	   cur_x, cur_y;
	int	   line_size;
	int	   delay;
};

struct _IOcallbacks
{
	int (*read)(
		void* user, char* data, int size); // fill 'data' with 'size' bytes.  return number of bytes actually read
	void (*skip)(void* user, int n);	   // skip the next 'n' bytes, or 'unget' the last -n bytes if negative
	int (*eof)(void* user);				   // returns nonzero if we are at end of file/data
};

struct _Context
{
	_IOcallbacks io;
	void*		 io_user_data;

	int	  read_from_callbacks;
	int	  buflen;
	uchar buffer_start[128];
	int	  callback_already_read;

	uchar *img_buffer, *img_buffer_end;
	uchar *img_buffer_original, *img_buffer_original_end;
};

static void _refill_buffer(_Context* s)
{
	int n = (s->io.read)(s->io_user_data, ptr(s->buffer_start), s->buflen);
	s->callback_already_read += int(s->img_buffer - s->img_buffer_original);
	if (n == 0)
	{
		// at end of file, treat same as if from memory, but need to handle case
		// where s->img_buffer isn't pointing to safe memory, e.g. 0-byte file
		s->read_from_callbacks = 0;
		s->img_buffer		   = s->buffer_start;
		s->img_buffer_end	   = s->buffer_start + 1;
		*s->img_buffer		   = 0;
	}
	else
	{
		s->img_buffer	  = s->buffer_start;
		s->img_buffer_end = s->buffer_start + n;
	}
}
inline static uchar _get_u8(_Context* s)
{
	if (s->img_buffer < s->img_buffer_end) return *s->img_buffer++;
	if (s->read_from_callbacks)
	{
		_refill_buffer(s);
		return *s->img_buffer++;
	}
	return 0;
}
static void _skip(_Context* s, int n)
{
	if (n == 0) return; // already there!
	if (n < 0)
	{
		s->img_buffer = s->img_buffer_end;
		return;
	}
	if (s->io.read)
	{
		int blen = (int)(s->img_buffer_end - s->img_buffer);
		if (blen < n)
		{
			s->img_buffer = s->img_buffer_end;
			(s->io.skip)(s->io_user_data, n - blen);
			return;
		}
	}
	s->img_buffer += n;
}

static void stbi__out_gif_code(_Gif* g, uint16 code)
{
	uchar *p, *c;
	int	   idx;

	// recurse to decode the prefixes, since the linked-list is backwards,
	// and working backwards through an interleaved image would be nasty
	if (g->codes[code].prefix >= 0) stbi__out_gif_code(g, g->codes[code].prefix);

	if (g->cur_y >= g->max_y) return;

	idx = g->cur_x + g->cur_y;
	p	= &g->out[idx];

	c = &g->color_table[g->codes[code].suffix * 4];
	if (c[3] > 128)
	{ // don't render transparent pixels;
		p[0] = c[2];
		p[1] = c[1];
		p[2] = c[0];
		p[3] = c[3];
	}
	g->cur_x += 4;

	if (g->cur_x >= g->max_x)
	{
		g->cur_x = g->start_x;
		g->cur_y += g->step;

		while (g->cur_y >= g->max_y && g->parse > 0)
		{
			g->step	 = (1 << g->parse) * g->line_size;
			g->cur_y = g->start_y + (g->step >> 1);
			--g->parse;
		}
	}
}

static uchar* stbi__process_gif_raster(_Context* s, _Gif* g)
{
	uchar  lzw_cs;
	int32  len, init_code;
	uint32 first;
	int32  codesize, codemask, avail, oldcode, bits, valid_bits, clear;
	_Lzw*  p;

	lzw_cs = _get_u8(s);
	if (lzw_cs > 12) return nullptr;
	clear	   = 1 << lzw_cs;
	first	   = 1;
	codesize   = lzw_cs + 1;
	codemask   = (1 << codesize) - 1;
	bits	   = 0;
	valid_bits = 0;
	for (init_code = 0; init_code < clear; init_code++)
	{
		g->codes[init_code].prefix = -1;
		g->codes[init_code].first  = (uchar)init_code;
		g->codes[init_code].suffix = (uchar)init_code;
	}

	// support no starting clear code
	avail	= clear + 2;
	oldcode = -1;

	len = 0;
	for (;;)
	{
		if (valid_bits < codesize)
		{
			if (len == 0)
			{
				len = _get_u8(s); // start new block
				if (len == 0) return g->out;
			}
			--len;
			bits |= (int32)_get_u8(s) << valid_bits;
			valid_bits += 8;
		}
		else
		{
			int32 code = bits & codemask;
			bits >>= codesize;
			valid_bits -= codesize;
			if (code == clear)
			{ // clear code
				codesize = lzw_cs + 1;
				codemask = (1 << codesize) - 1;
				avail	 = clear + 2;
				oldcode	 = -1;
				first	 = 0;
			}
			else if (code == clear + 1)
			{ // end of stream code
				_skip(s, len);
				while ((len = _get_u8(s)) > 0) _skip(s, len);
				return g->out;
			}
			else if (code <= avail)
			{
				if (first) throw "Corrupt GIF: no clear code";

				if (oldcode >= 0)
				{
					p = &g->codes[avail++];
					if (avail > 8192) throw "Corrupt GIF: too many codes";

					p->prefix = int16(oldcode);
					p->first  = g->codes[oldcode].first;
					p->suffix = (code == avail) ? p->first : g->codes[code].first;
				}
				else if (code == avail) throw "Corrupt GIF: illegal code in raster";

				stbi__out_gif_code(g, (uint16)code);

				if ((avail & codemask) == 0 && avail <= 0x0FFF)
				{
					codesize++;
					codemask = (1 << codesize) - 1;
				}

				oldcode = code;
			}
			else throw "Corrupt GIF: illegal code in raster";
		}
	}
}


void GifDecoder::finish()
{
	//	Read to end of image, including the zero block.

	while ((bufsize = file->read_uchar())) { file->read(buf, bufsize); }
}

void GifDecoder::lz_initialize()
{
	// Initialize decoder for the next image

	//file_state      = IMAGE_LOADING;
	depth			  = file->read_uint8(); /* lzw_min */
	clear_code		  = 1 << depth;
	eof_code		  = clear_code + 1;
	running_code	  = eof_code + 1;
	running_bits	  = depth + 1;
	max_code_plus_one = 1 << running_bits;
	prev_code		  = NO_SUCH_CODE;
	stack_ptr		  = 0;
	shift_state		  = 0;
	position		  = 0;
	bufsize			  = 0;
	shift_data		  = 0;
	buf[0]			  = 0; // required?

	prefix = new uint16[LZ_SIZE];
	stack  = new uchar[LZ_SIZE];
	suffix = new uchar[LZ_SIZE];

	for (uint i = 0; i < LZ_SIZE; i++) { prefix[i] = NO_SUCH_CODE; }
}

inline uchar GifDecoder::read_gif_byte()
{
	// Read the next byte from a Gif file.

	if unlikely (position == bufsize) // internal buffer now empty?
	{								  // => read the next block
		bufsize = file->read_uchar();
		if (bufsize == 0)
		{
			throw "Unexpected final block"; // TODO
			file->setFpos(file->getFpos() - 1);
			return 0;
		}
		file->read(buf, bufsize);
		position = 0;
	}

	return buf[position++];
}

uint GifDecoder::read_gif_code()
{
	// Read the next Gif code word from the file.
	//
	// This function looks in the decoder to find out how many
	// bits to read, and uses a buffer in the decoder to remember
	// bits from the last byte input.

	static constexpr uint code_masks[] = {0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f,
										  0x007f, 0x00ff, 0x01ff, 0x03ff, 0x07ff, 0x0fff};

	while (shift_state < running_bits)
	{
		// Need more bytes from input file for next code:
		uchar next_byte = read_gif_byte();
		shift_data |= uint32(next_byte) << shift_state;
		shift_state += 8;
	}

	uint code = shift_data & code_masks[running_bits];

	shift_data >>= running_bits;
	shift_state -= running_bits;

	//	If code cannot fit into running_bits bits, we must raise its size.
	//	Note: codes above 4095 are used for signalling.
	if (++running_code > max_code_plus_one && running_bits < LZ_BITS)
	{
		max_code_plus_one <<= 1;
		running_bits++;
	}

	return code;
}

static uint trace_prefix(uint16* prefix, uint code, uint clear_code)
{
	// Trace the prefix-linked-list until we get a prefix which is a pixel value (less than clear_code).
	// Returns that pixel value.
	// If the picture is defective, we might loop here forever, so we limit the loops to the
	// maximum possible if the picture is okay, i.e. LZ_MAX_CODE times.

	uint i = 0;
	while (code > clear_code && i++ <= LZ_MAX_CODE) { code = prefix[code]; }
	return code;
}

void GifDecoder::lz_read_scanline(uchar* line, int length)
{
	// The LZ decompression routine:
	// Call this function once per scanline to fill in a picture.

	int	 stack_ptr = this->stack_ptr;
	uint prev_code = this->prev_code;
	uint current_code;
	uint current_prefix;

	int i = 0;
	while (stack_ptr != 0 && i < length) { line[i++] = stack[--stack_ptr]; } // Pop the stack
	while (i < length)
	{
		current_code = read_gif_code();

		if unlikely (current_code == eof_code) // unexpected EOF
		{									   // TODO: leftover from orig. code. check: ever possible?
			if (i != length - 1) return;
			i++;
			continue;
		}

		if unlikely (current_code == clear_code) // reset prefix table etc.
		{
			for (uint j = 0; j < LZ_SIZE; j++) { prefix[j] = NO_SUCH_CODE; }
			running_code	  = eof_code + 1;
			running_bits	  = depth + 1;
			max_code_plus_one = 1 << running_bits;
			prev_code = this->prev_code = NO_SUCH_CODE;
			continue;
		}

		// Regular code - if in pixel range simply add it to output pixel stream,
		// otherwise trace code-linked-list until the prefix is in pixel range.
		if (current_code < clear_code)
		{
			line[i++] = uchar(current_code); // Simple case.
		}
		else
		{
			//	This code needs to be traced:
			//	trace the linked list until the prefix is a pixel, while pushing the suffix pixels
			//	on to the stack. If finished, pop the stack to output the pixel values.
			if (uint(current_code) > LZ_MAX_CODE) throw "Corrupt gif file :1";
			if (prefix[current_code] == NO_SUCH_CODE)
			{
				// Only allowed if current_code is exactly the running code:
				// In that case current_code = XXXCode,
				// current_code or the prefix code is the last code and
				// the suffix char is exactly the prefix of last code!
				if (current_code != running_code - 2) throw "Corrupt gif file :2";
				current_prefix			 = prev_code;
				suffix[running_code - 2] = stack[stack_ptr++] = uchar(trace_prefix(prefix, prev_code, clear_code));
			}
			else { current_prefix = current_code; }

			// Now (if picture is okay) we should get no NO_SUCH_CODE during the trace.
			// As we might loop forever (if picture defect) we count the number of loops we trace
			// and stop if we get LZ_MAX_CODE. Obviously we cannot loop more than that.
			uint j = 0;
			while (j++ <= LZ_MAX_CODE && current_prefix > clear_code && current_prefix <= LZ_MAX_CODE)
			{
				stack[stack_ptr++] = suffix[current_prefix];
				current_prefix	   = prefix[current_prefix];
			}
			if (j >= LZ_MAX_CODE || current_prefix > LZ_MAX_CODE) throw "Corrupt gif file :3";

			// Push the last character on stack:
			stack[stack_ptr++] = uchar(current_prefix);

			// Now pop the entire stack into output:
			while (stack_ptr != 0 && i < length) { line[i++] = stack[--stack_ptr]; }
		}

		if (prev_code != NO_SUCH_CODE)
		{
			if ((running_code < 2) || (running_code > LZ_MAX_CODE + 2)) throw "Corrupt gif file :4";
			prefix[running_code - 2] = uint16(prev_code);

			if (current_code == running_code - 2)
			{
				// Only allowed if current_code is exactly the running code:
				// In that case current_code = XXXCode,
				// current_code or the prefix code is the last code and the suffix char
				//	is exactly the prefix of the last code!
				suffix[running_code - 2] = uchar(trace_prefix(prefix, prev_code, clear_code));
			}
			else { suffix[running_code - 2] = uchar(trace_prefix(prefix, current_code, clear_code)); }
		}
		prev_code = current_code;
	}

	this->prev_code = prev_code;
	this->stack_ptr = stack_ptr;
}

Video::Color* GifDecoder::read_cmap(uint8 bits, uint maxbits)
{
	if (!maxbits) maxbits = bits;
	assert(maxbits >= bits);

	uint8  rgb[3];
	Color* cmap = new Color[1u << maxbits];

	for (uint i = 0; i < 1 << bits; i++)
	{
		file->read(rgb, 3);
		cmap[i] = Color::fromRGB8(rgb[0], rgb[1], rgb[2]);
	}

	return cmap;
}

static void merge_cmaps(Color* z, uint16& zcnt, uint zmax, const Color* q, uint qcnt, int transp, uint8* lookup_tbl)
{
	assert(zmax <= 256);
	assert(zcnt <= zmax);
	assert(qcnt <= 256);
	assert(transp < int(qcnt));
	assert(q && z);

	for (uint zi, qi = 0; qi < qcnt; qi++)
	{
		if (int(qi) == transp) continue;

		for (zi = 0; zi < zcnt && z[zi] != q[qi]; zi++) {}
		if (zi == zcnt) // no exact match found?
		{
			if (zcnt < zmax) // append:
			{
				z[zcnt++].raw = q[qi].raw;
			}
			else // replace best match:
			{	 // this should never be neccessary in a proper file!
				int best = 99999;
				for (uint zi2 = 0; zi2 < zcnt; zi2++)
				{
					int d = z[zi2].distance(q[qi]);
					if (d >= best) continue;
					zi	 = zi2;
					best = d;
				}
				z[zi] = q[qi]; // replace the color
			}
		}
		lookup_tbl[qi] = uint8(zi);
	}

	if (transp >= 0) // find unused global color for transparency:
	{
		if (zcnt <= 255) lookup_tbl[transp] = 255;
		else // search for unused color:
		{
			uint32 bits[8] = {0};
			for (uint qi = 0; qi < qcnt; qi++)
				if (qi != uint8(transp)) bits[lookup_tbl[qi] / 32] |= 1u << (lookup_tbl[qi] % 32);
			for (uint i = 0; i < 8; i++)
				if (~bits[i] != 0)
				{
					lookup_tbl[transp] = uint8(i * 32 + msbit(~bits[i]));
					break;
				}
		}
	}
}

GifDecoder::GifDecoder(FilePtr file) : file(file)
{
	debugstr("GifDecode:ctor\n");
	if (!file) return; // isa_gif_file=false

	char bu[7] = "      ";
	file->read(bu, 6);
	if (!eq(bu, "GIF87a") && !eq(bu, "GIF89a")) return; // not a gif file

	image_width		 = file->read_uint16();
	image_height	 = file->read_uint16();
	uint8 flags		 = file->read_uint8();
	background_color = file->read_uint8();
	aspect			 = file->read_uint8(); // normally = 0

	if (image_width > 2 kB || image_width < 4) return;	 // sanity
	if (image_height > 2 kB || image_height < 1) return; // sanity
	global_cmap_bits = (flags & 7) + 1;
	total_color_bits = ((flags >> 4) & 7) + 1;

	if (flags & 0x80) // has global color table
	{
		if (total_color_bits < global_cmap_bits) total_color_bits = global_cmap_bits; // safety
		global_cmap		 = read_cmap(global_cmap_bits, total_color_bits);
		global_cmap_used = uint8(1u << global_cmap_bits);
	}
	else { global_cmap = new Color[1 << total_color_bits]; }

	isa_gif_file = true;

	debugstr("GifDecoder: width,height = %ux%u\n", image_width, image_height);
	debugstr("GifDecoder: total colors = %u\n", 1 << total_color_bits);
	if (flags & 0x80) debugstr("GifDecoder: global colors = %u\n", 1 << global_cmap_bits);
	else debugstr("GifDecoder: no global color map\n");
	if (aspect) debugstr("GifDecoder: aspect ratio = %u/64\n", aspect + 15);
}

GifDecoder::~GifDecoder()
{
	delete[] global_cmap;
	delete[] comment;
	delete[] pixels;
	delete[] stack;
	delete[] suffix;
	delete[] prefix;
}

int GifDecoder::decode_frame(store_scanline& fu)
{
	// gif_signature
	// opt. global cmap
	// chunks
	// gif_terminator
	//
	// chunk netscape looping animation
	// chunk comment
	// chunk animation control
	// sub_image
	//	 image_descr
	//   opt. cmap
	//   lz_data

	for (;;)
	{
		uint8 blocktype = file->read_uint8();

		if (blocktype == ',') // sub_image
		{
			if (disposal_method >= 2) // if caller didn't clear the disposal_method
			{
				disposal_method = 0;
				memset(buf, background_color, sizeof(buf));
				while (width)
				{
					int w = std::min(int(width), 256);
					for (int y = 0; y < height; y++) fu(xpos, ypos + y, w, pixels, global_cmap, -1);
					width -= w;
					xpos += w;
				}
			}

			xpos				 = file->read_uint16();
			ypos				 = file->read_uint16();
			width				 = file->read_uint16();
			height				 = file->read_uint16();
			uint8 flags			 = file->read_uint8();
			bool  has_local_cmap = flags & 0x80;
			bool  interleaved	 = flags & 0x40;
			uint8 cmap_bits		 = (flags & 7) + 1;

			if (xpos + width > image_width) throw "Image corrupt";
			if (ypos + height > image_height) throw "Image corrupt";

			uint8 local_to_global[256]; // map local to global color index
			if (has_local_cmap)
			{
				Color* cmap = read_cmap(cmap_bits);
				merge_cmaps(
					global_cmap, global_cmap_used, 1 << total_color_bits, cmap, 1 << cmap_bits, transparent_color,
					local_to_global);
				delete[] cmap;
			}

			pixels = new uint8[width];

			lz_initialize();

			static constexpr uint8 y0[4] = {0, 4, 2, 1};
			static constexpr uint8 dy[4] = {8, 8, 4, 2};

			int transp = transparent_color;
			if (transp >= 0 && has_local_cmap) transp = local_to_global[transp];
			transparent_color = -1;

			for (uint i = interleaved ? 0 : 3; i < 4; i++)
			{
				for (int y = interleaved ? y0[i] : 0, d = interleaved ? dy[i] : 1; y < height; y += d)
				{
					lz_read_scanline(pixels, width);
					if (has_local_cmap)
						for (uint i = 0; i < width; i++) pixels[i] = local_to_global[pixels[i]]; //
					fu(xpos, ypos + y, width, pixels, global_cmap, transp);
				}
			}

			finish();
			// after this "garbage" bytes are allowed up to next ',' ...
			delete[] pixels;
			delete[] stack;
			delete[] suffix;
			delete[] prefix;
			pixels = nullptr;
			stack  = nullptr;
			suffix = nullptr;
			prefix = nullptr;
		}

		else if (blocktype == '!') // extension block
		{
			blocktype = file->read_uint8();

			if (blocktype == 0xff) // looping animation
			{
				uint count = file->read_uint8();
				file->read(buf, count);
				if (count == 11) // "NETSCAPE2.0"
				{
					count = file->read_uint8();
					file->read(buf + 1, count);
					if (count == 3)
					{
						loop_count		= *uint16ptr(buf + 2); // lohi
						loop_reset_fpos = uint32(file->getFpos());
						if (!loop_count) loop_count = 0xffff; // forever
					}
				}
				finish();
			}
			else if (blocktype == 0xfe) // comment
			{
				if (!comment)
				{
					uint count = file->read_uint8();
					comment	   = newstr(count);
					file->read(comment, count);
				}
				finish();
			}
			else if (blocktype == 0xf9) // animation control
			{
				uint count = file->read_uint8();
				file->read(buf + 1, count);
				if (count == 4)
				{
					uint8  flags	  = buf[1];
					uint16 delay	  = *uint16ptr(buf + 2); // lohi
					uint8  transp	  = buf[4];
					disposal_method	  = (flags >> 2) & 3;
					transparent_color = flags & 1 ? transp : -1;

					finish();
					if (delay) return delay;
				}
			}
			else // unknown extension block
			{
				debugstr("gif: unknown extension block 0x%02X\n", blocktype);
				finish();
			}
		}

		else if (blocktype == ';') // end of gif file
		{
			if (loop_count == 0 || --loop_count == 0) return 0;
			file->setFpos(loop_reset_fpos);
			finish();
		}

		else // this may be "garbage" after last image block:
		{
			debugstr("gif: unknown block 0x%02X\n", blocktype);
		}
	}
}

int GifDecoder::decode_frame(Canvas& pm, int x0, int y0)
{
	// decode image up to the next animation control block or loop end
	// does not draw the transparent pixels => dest pixmap must be cleared ahead
	// this version is for true color dest pixmap

	store_scanline fu = [&](int x, int y, int w, uchar* pixels, Video::Color* cmap, int transp) {
		y += y0;
		if (uint(y) >= uint(pm.height)) return;
		x += x0;
		if (x < 0) { pixels -= x, w += x, x = 0; }
		if (x + w > pm.width) w = pm.width - x;
		if (w <= 0) return;

		while (w--)
		{
			uint8 pixel = *pixels++;
			if (pixel != transp) pm.set_pixel(x, y, cmap[pixel]);
			x++;
		}
	};

	return decode_frame(fu);
}

int GifDecoder::decode_frame(Canvas& dest, Color* cmap_out, int x0, int y0)
{
	// decode image up to the next animation control block or loop end
	// does not draw the transparent pixels => dest pixmap must be cleared ahead
	// pixmap must be an indexed color pixmap of the same depth as the gif image

	store_scanline fu = [&](int x, int y, int w, uchar* pixels, Video::Color* __unused cmap, int transp) {
		y += y0;
		if (uint(y) >= uint(dest.height)) return;
		x += x0;
		if (x < 0) { pixels -= x, w += x, x = 0; }
		if (x + w > dest.width) w = dest.width - x;
		if (w <= 0) return;

		while (w--)
		{
			uint8 pixel = *pixels++;
			if (pixel != transp) dest.set_pixel(x, y, pixel);
			x++;
		}
	};

	int rval = decode_frame(fu);
	if (cmap_out) memcpy(cmap_out, global_cmap, sizeof(Color) << min(dest.bits_per_color(), total_color_bits));
	return rval;
}

int GifDecoder::decode_frame(Canvas& pm, Color* colormap, int* transp_color, int x0, int y0)
{
	// decode image up to the next animation control block or loop end
	// draws the transparent pixels & returns the transparent color index
	// pixmap must be an indexed color pixmap of the same depth as the gif image

	store_scanline fu = [&](int x, int y, int w, uchar* pixels, Video::Color* __unused cmap, int transp) {
		y += y0;
		if (uint(y) >= uint(pm.height)) return;
		x += x0;
		if (x < 0) { pixels -= x, w += x, x = 0; }
		if (x + w > pm.width) w = pm.width - x;
		if (w <= 0) return;

		if (transp_color) *transp_color = transp;

		while (w--)
		{
			uint8 pixel = *pixels++;
			pm.set_pixel(x, y, pixel);
			x++;
		}
	};

	int rval = decode_frame(fu);
	if (colormap) memcpy(colormap, global_cmap, sizeof(Color) << min(pm.bits_per_color(), total_color_bits));
	return rval;
}

void GifDecoder::decode_image(Canvas& dest, int x0, int y0)
{
	// decode image up to the last frame
	// does not draw the transparent pixels => dest pixmap must be cleared ahead
	// pixmap should be a true color pixmap

	while (decode_frame(dest, x0, y0)) { loop_count = 0; }
}

void GifDecoder::decode_image(Canvas& dest, Color* cmap, int x0, int y0)
{
	// decode image up to the last frame
	// does not draw the transparent pixels => dest pixmap must be cleared ahead
	// pixmap should be an indexed color pixmap of at least the same depth as the gif image

	while (decode_frame(dest, cmap, x0, y0)) { loop_count = 0; }
}

void GifDecoder::decode_image(Canvas& dest, Color* cmap, int* transp_color, int x0, int y0)
{
	// decode image up to the last frame
	// draws the transparent pixels & returns the transparent color index
	// pixmap should be an indexed color pixmap of at least the same depth as the gif image
	// note: if a gif contains multiple transparent frames then the transp_color may be wrong.

	while (decode_frame(dest, cmap, transp_color, x0, y0)) { loop_count = 0; }
}


} // namespace kio::Graphics

/*




































*/
