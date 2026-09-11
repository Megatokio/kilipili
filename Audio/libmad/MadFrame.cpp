/*
 * libmad - MPEG audio decoder library
 * Copyright (C) 2000-2004 Underbit Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: frame.c,v 1.29 2004/02/04 22:59:19 rob Exp $
 *
 *
 * c++ adaption:
 * Copyright (c) 2026 - 2026 kio@little-bat.de
 * GPL-2.0 license
 * https://opensource.org/license/gpl-2.0
 */

#include "MadFrame.h"
#include "MadStream.h"
#include "global.h"
#include "mad_bitptr.h"
#include "timer.h"
#include <cstring>
#include <stdlib.h>

static constexpr ulong bitrate_table[5][15] = {
	/* MPEG-1 */
	{0, 32000, 64000, 96000, 128000, 160000, 192000, 224000, /* Layer I   */
	 256000, 288000, 320000, 352000, 384000, 416000, 448000},
	{0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, /* Layer II  */
	 128000, 160000, 192000, 224000, 256000, 320000, 384000},
	{0, 32000, 40000, 48000, 56000, 64000, 80000, 96000, /* Layer III */
	 112000, 128000, 160000, 192000, 224000, 256000, 320000},

	/* MPEG-2 LSF */
	{0, 32000, 48000, 56000, 64000, 80000, 96000, 112000, /* Layer I   */
	 128000, 144000, 160000, 176000, 192000, 224000, 256000},
	{0, 8000, 16000, 24000, 32000, 40000, 48000, 56000,	  /* Layers    */
	 64000, 80000, 96000, 112000, 128000, 144000, 160000} /* II & III  */
};

static constexpr uint samplerate_table[3] = {44100, 48000, 32000};

/*
 * NAME:	header->init()
 * DESCRIPTION:	initialize header struct
 */
void MadHeader::init() noexcept
{
	layer		   = mad_layer(0);
	mode		   = mad_mode(0);
	mode_extension = 0;
	emphasis	   = mad_emphasis(0);
	bitrate		   = 0;
	samplerate	   = 0;
	crc_check	   = 0;
	crc_target	   = 0;
	flags		   = 0;
	private_bits   = 0;
	duration	   = mad_timer_zero;
}

/*
 * NAME:	frame->init()
 * DESCRIPTION:	initialize frame struct
 */
MadFrame::MadFrame()
{
	header.init();

	options = 0;
	overlap = nullptr;
	tmp		= nullptr;
	xr		= nullptr;

	// zero all subband values so the frame becomes silent:
	memset(sbsample, 0, sizeof(sbsample));
}

/*
 * NAME:	frame->finish()
 * DESCRIPTION:	deallocate any dynamic memory associated with frame
 */
MadFrame::~MadFrame()
{
	header.finish();

	free(overlap);
	free(tmp);
	free(xr);
}

/*
 * NAME:	decode_header()
 * DESCRIPTION:	read header data and following CRC word
 */
static int decode_header(MadHeader* header, MadStream* stream)
{
	uint index;

	header->flags		 = 0;
	header->private_bits = 0;

	/* header() */

	/* syncword */
	stream->ptr.skip(11);

	/* MPEG 2.5 indicator (really part of syncword) */
	if (stream->ptr.read(1) == 0) header->flags |= MAD_FLAG_MPEG_2_5_EXT;

	/* ID */
	if (stream->ptr.read(1) == 0) header->flags |= MAD_FLAG_LSF_EXT;
	else if (header->flags & MAD_FLAG_MPEG_2_5_EXT)
	{
		stream->error = MAD_ERROR_LOSTSYNC;
		return -1;
	}

	/* layer */
	header->layer = mad_layer(4 - stream->ptr.read(2));

	if (int(header->layer) == 4)
	{
		stream->error = MAD_ERROR_BADLAYER;
		return -1;
	}

	/* protection_bit */
	if (stream->ptr.read(1) == 0)
	{
		header->flags |= MAD_FLAG_PROTECTION;
		header->crc_check = stream->ptr.crc(16, 0xffff);
	}

	/* bitrate_index */
	index = stream->ptr.read(4);

	if (index == 15)
	{
		stream->error = MAD_ERROR_BADBITRATE;
		return -1;
	}

	if (header->flags & MAD_FLAG_LSF_EXT) header->bitrate = bitrate_table[3 + (header->layer >> 1)][index];
	else header->bitrate = bitrate_table[header->layer - 1][index];

	/* sampling_frequency */
	index = stream->ptr.read(2);

	if (index == 3)
	{
		stream->error = MAD_ERROR_BADSAMPLERATE;
		return -1;
	}

	header->samplerate = samplerate_table[index];

	if (header->flags & MAD_FLAG_LSF_EXT)
	{
		header->samplerate /= 2;

		if (header->flags & MAD_FLAG_MPEG_2_5_EXT) header->samplerate /= 2;
	}

	/* padding_bit */
	if (stream->ptr.read(1)) header->flags |= MAD_FLAG_PADDING;

	/* private_bit */
	if (stream->ptr.read(1)) header->private_bits |= MAD_PRIVATE_HEADER;

	/* mode */
	header->mode = mad_mode(3 - stream->ptr.read(2));

	/* mode_extension */
	header->mode_extension = stream->ptr.read(2);

	/* copyright */
	if (stream->ptr.read(1)) header->flags |= MAD_FLAG_COPYRIGHT;

	/* original/copy */
	if (stream->ptr.read(1)) header->flags |= MAD_FLAG_ORIGINAL;

	/* emphasis */
	header->emphasis = mad_emphasis(stream->ptr.read(2));

#if defined(OPT_STRICT)
	/*
   * ISO/IEC 11172-3 says this is a reserved emphasis value, but
   * streams exist which use it anyway. Since the value is not important
   * to the decoder proper, we allow it unless OPT_STRICT is defined.
   */
	if (header->emphasis == MAD_EMPHASIS_RESERVED)
	{
		stream->error = MAD_ERROR_BADEMPHASIS;
		return -1;
	}
#endif

	/* error_check() */

	/* crc_check */
	if (header->flags & MAD_FLAG_PROTECTION) header->crc_target = stream->ptr.read(16);

	return 0;
}

/*
 * NAME:	free_bitrate()
 * DESCRIPTION:	attempt to discover the bitstream's free bitrate
 */
static int free_bitrate(MadStream* stream, const MadHeader* header)
{
	mad_bitptr	 keep_ptr;
	ulong		 rate = 0;
	uint		 pad_slot, slots_per_frame;
	const uchar* ptr = 0;

	keep_ptr = stream->ptr;

	pad_slot		= (header->flags & MAD_FLAG_PADDING) ? 1 : 0;
	slots_per_frame = (header->layer == MAD_LAYER_III && (header->flags & MAD_FLAG_LSF_EXT)) ? 72 : 144;

	while (MadStream_sync(stream) == 0)
	{
		struct MadStream peek_stream(*stream);
		struct MadHeader peek_header(*header);

		if (decode_header(&peek_header, &peek_stream) == 0 && peek_header.layer == header->layer &&
			peek_header.samplerate == header->samplerate)
		{
			uint N;

			ptr = stream->ptr.nextbyte();

			N = ptr - stream->this_frame;

			if (header->layer == MAD_LAYER_I) { rate = (ulong)header->samplerate * (N - 4 * pad_slot + 4) / 48 / 1000; }
			else { rate = (ulong)header->samplerate * (N - pad_slot + 1) / slots_per_frame / 1000; }

			if (rate >= 8) break;
		}

		stream->ptr.skip(8);
	}

	stream->ptr = keep_ptr;

	if (rate < 8 || (header->layer == MAD_LAYER_III && rate > 640))
	{
		stream->error = MAD_ERROR_LOSTSYNC;
		return -1;
	}

	stream->freerate = rate * 1000;

	return 0;
}

/*
 * NAME:	header->decode()
 * DESCRIPTION:	read the next frame header from the stream
 */
int MadHeader::decode(MadStream* stream)
{
	const uchar *ptr, *end;
	uint		 pad_slot, N;

	ptr = stream->next_frame;
	end = stream->bufend;

	if (ptr == 0)
	{
		stream->error = MAD_ERROR_BUFPTR;
		goto fail;
	}

	/* stream skip */
	if (stream->skiplen)
	{
		if (!stream->sync) ptr = stream->this_frame;

		if (end - ptr < stream->skiplen)
		{
			stream->skiplen -= end - ptr;
			stream->next_frame = end;

			stream->error = MAD_ERROR_BUFLEN;
			goto fail;
		}

		ptr += stream->skiplen;
		stream->skiplen = 0;

		stream->sync = 1;
	}

sync:
	/* synchronize */
	if (stream->sync)
	{
		if (end - ptr < MAD_BUFFER_GUARD)
		{
			stream->next_frame = ptr;

			stream->error = MAD_ERROR_BUFLEN;
			goto fail;
		}
		else if (!(ptr[0] == 0xff && (ptr[1] & 0xe0) == 0xe0))
		{
			/* mark point where frame sync word was expected */
			stream->this_frame = ptr;
			stream->next_frame = ptr + 1;

			stream->error = MAD_ERROR_LOSTSYNC;
			goto fail;
		}
	}
	else
	{
		stream->ptr.init(ptr);

		if (MadStream_sync(stream) == -1)
		{
			if (end - stream->next_frame >= MAD_BUFFER_GUARD) stream->next_frame = end - MAD_BUFFER_GUARD;

			stream->error = MAD_ERROR_BUFLEN;
			goto fail;
		}

		ptr = stream->ptr.nextbyte();
	}

	/* begin processing */
	stream->this_frame = ptr;
	stream->next_frame = ptr + 1; /* possibly bogus sync word */

	stream->ptr.init(stream->this_frame);

	if (decode_header(this, stream) == -1) goto fail;

	/* calculate frame duration */
	mad_timer_set(&this->duration, 0, 32 * MAD_NSBSAMPLES(this), this->samplerate);

	/* calculate free bit rate */
	if (this->bitrate == 0)
	{
		if ((stream->freerate == 0 || !stream->sync || (this->layer == MAD_LAYER_III && stream->freerate > 640000)) &&
			free_bitrate(stream, this) == -1)
			goto fail;

		this->bitrate = stream->freerate;
		this->flags |= MAD_FLAG_FREEFORMAT;
	}

	/* calculate beginning of next frame */
	pad_slot = (this->flags & MAD_FLAG_PADDING) ? 1 : 0;

	if (this->layer == MAD_LAYER_I) N = ((12 * this->bitrate / this->samplerate) + pad_slot) * 4;
	else
	{
		uint slots_per_frame;

		slots_per_frame = (this->layer == MAD_LAYER_III && (this->flags & MAD_FLAG_LSF_EXT)) ? 72 : 144;

		N = (slots_per_frame * this->bitrate / this->samplerate) + pad_slot;
	}

	/* verify there is enough data left in buffer to decode this frame */
	if (N + MAD_BUFFER_GUARD > end - stream->this_frame)
	{
		stream->next_frame = stream->this_frame;

		stream->error = MAD_ERROR_BUFLEN;
		goto fail;
	}

	stream->next_frame = stream->this_frame + N;

	if (!stream->sync)
	{
		/* check that a valid frame header follows this frame */

		ptr = stream->next_frame;
		if (!(ptr[0] == 0xff && (ptr[1] & 0xe0) == 0xe0))
		{
			ptr = stream->next_frame = stream->this_frame + 1;
			goto sync;
		}

		stream->sync = 1;
	}

	this->flags |= MAD_FLAG_INCOMPLETE;

	return 0;

fail:
	stream->sync = 0;

	return -1;
}

/*
 * NAME:	frame->decode()
 * DESCRIPTION:	decode a single frame from a bitstream
 */
int MadFrame::decode(MadStream* stream)
{
	this->options = stream->options;

	/* header() */
	/* error_check() */

	if (!(this->header.flags & MAD_FLAG_INCOMPLETE) && this->header.decode(stream) == -1)
	{
		stream->anc_bitlen = 0;
		return -1; // fail
	}

	/* audio_data() */

	this->header.flags &= ~MAD_FLAG_INCOMPLETE;

	int layer  = this->header.layer;
	int result = layer == 3 ? this->decode_layer_III(stream) :
				 layer == 2 ? this->decode_layer_II(stream) :
							  this->decode_layer_I(stream);

	if (result == -1)
	{
		if (!MAD_RECOVERABLE(stream->error)) stream->next_frame = stream->this_frame;
		stream->anc_bitlen = 0;
		return -1; // fail
	}

	/* ancillary_data() */

	if (this->header.layer != MAD_LAYER_III)
	{
		mad_bitptr next_frame;

		next_frame.init(stream->next_frame);

		stream->anc_ptr	   = stream->ptr;
		stream->anc_bitlen = mad_bit_length(&stream->ptr, &next_frame);

		next_frame.finish();
	}

	return 0; // success
}

/*






































*/
