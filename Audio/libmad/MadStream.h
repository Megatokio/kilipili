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
 * $Id: stream.h,v 1.20 2004/02/05 09:02:39 rob Exp $
 *
 *
 * c++ adaption:
 * Copyright (c) 2026 - 2026 kio@little-bat.de
 * GPL-2.0 license
 * https://opensource.org/license/gpl-2.0
 */

#pragma once
#include "mad_bitptr.h"

#define MAD_BUFFER_GUARD 8
#define MAD_BUFFER_MDLEN (511 + 2048 + MAD_BUFFER_GUARD)

enum mad_error {
	MAD_ERROR_NONE = 0x0000, /* no error */

	MAD_ERROR_BUFLEN = 0x0001, /* input buffer too small (or EOF) */
	MAD_ERROR_BUFPTR = 0x0002, /* invalid (null) buffer pointer */

	MAD_ERROR_NOMEM = 0x0031, /* not enough memory */

	MAD_ERROR_LOSTSYNC		= 0x0101, /* lost synchronization */
	MAD_ERROR_BADLAYER		= 0x0102, /* reserved header layer value */
	MAD_ERROR_BADBITRATE	= 0x0103, /* forbidden bitrate value */
	MAD_ERROR_BADSAMPLERATE = 0x0104, /* reserved sample frequency value */
	MAD_ERROR_BADEMPHASIS	= 0x0105, /* reserved emphasis value */

	MAD_ERROR_BADCRC		 = 0x0201, /* CRC check failed */
	MAD_ERROR_BADBITALLOC	 = 0x0211, /* forbidden bit allocation value */
	MAD_ERROR_BADSCALEFACTOR = 0x0221, /* bad scalefactor index */
	MAD_ERROR_BADMODE		 = 0x0222, /* bad bitrate/mode combination */
	MAD_ERROR_BADFRAMELEN	 = 0x0231, /* bad frame length */
	MAD_ERROR_BADBIGVALUES	 = 0x0232, /* bad big_values count */
	MAD_ERROR_BADBLOCKTYPE	 = 0x0233, /* reserved block_type */
	MAD_ERROR_BADSCFSI		 = 0x0234, /* bad scalefactor selection info */
	MAD_ERROR_BADDATAPTR	 = 0x0235, /* bad main_data_begin pointer */
	MAD_ERROR_BADPART3LEN	 = 0x0236, /* bad audio data length */
	MAD_ERROR_BADHUFFTABLE	 = 0x0237, /* bad Huffman table select */
	MAD_ERROR_BADHUFFDATA	 = 0x0238, /* Huffman data overrun */
	MAD_ERROR_BADSTEREO		 = 0x0239  /* incompatible block_type for JS */
};

inline constexpr bool is_recoverable(mad_error error) noexcept { return error & 0xff00; }

struct MadStream
{
	MadStream(int opts = 0) noexcept;
	~MadStream() noexcept;

	void set_options(int opts) noexcept { options = opts; }
	void set_buffer_pointers(const uchar*, ulong) noexcept;
	void skip(ulong len) noexcept { skiplen += len; } /* arrange to skip bytes before next frame */
	int	 find_next_sync() noexcept;
	cstr errorstr() const noexcept;

	const uchar* buffer;  /* input bitstream buffer */
	const uchar* bufend;  /* end of buffer */
	ulong		 skiplen; /* bytes to skip before next frame */

	int	  sync;		/* stream sync found */
	ulong freerate; /* free bitrate (fixed) */

	const uchar* this_frame; /* start of current frame */
	const uchar* next_frame; /* start of next frame */
	mad_bitptr	 ptr;		 /* current processing bit pointer */

	mad_bitptr anc_ptr;	   /* ancillary bits pointer */
	uint	   anc_bitlen; /* number of ancillary bits */

	uchar (*main_data)[MAD_BUFFER_MDLEN]; /* Layer III main_data() */
	uint md_len;						  /* bytes in main_data */

	int		  options; /* decoding options (see below) */
	mad_error error;   /* error code (see above) */
};

enum {
	MAD_OPTION_IGNORECRC	  = 0x0001, /* ignore CRC errors */
	MAD_OPTION_HALFSAMPLERATE = 0x0002	/* generate PCM at 1/2 sample rate */
};
