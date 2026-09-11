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
 * $Id: frame.h,v 1.20 2004/01/23 09:41:32 rob Exp $
 *
 *
 * c++ adaption:
 * Copyright (c) 2026 - 2026 kio@little-bat.de
 * GPL-2.0 license
 * https://opensource.org/license/gpl-2.0
 */

#pragma once
#include "MadStream.h"
#include "mad_types.h"
#include "timer.h"

enum mad_layer {
	MAD_LAYER_I	  = 1, /* Layer I */
	MAD_LAYER_II  = 2, /* Layer II */
	MAD_LAYER_III = 3  /* Layer III */
};

enum mad_mode {
	MAD_MODE_SINGLE_CHANNEL = 0, /* single channel */
	MAD_MODE_DUAL_CHANNEL	= 1, /* dual channel */
	MAD_MODE_JOINT_STEREO	= 2, /* joint (MS/intensity) stereo */
	MAD_MODE_STEREO			= 3	 /* normal LR stereo */
};

enum mad_emphasis {
	MAD_EMPHASIS_NONE		= 0, /* no emphasis */
	MAD_EMPHASIS_50_15_US	= 1, /* 50/15 microseconds emphasis */
	MAD_EMPHASIS_CCITT_J_17 = 3, /* CCITT J.17 emphasis */
	MAD_EMPHASIS_RESERVED	= 2	 /* unknown emphasis */
};

struct MadHeader
{
	void init() noexcept;
	void finish() noexcept {}
	int	 decode(MadStream*);

	mad_layer	 layer;			 /* audio layer (1, 2, or 3) */
	mad_mode	 mode;			 /* channel mode (see above) */
	int			 mode_extension; /* additional mode info */
	mad_emphasis emphasis;		 /* de-emphasis to use (see above) */

	ulong bitrate;	  /* stream bitrate (bps) */
	uint  samplerate; /* sampling frequency (Hz) */

	ushort crc_check;  /* frame CRC accumulator */
	ushort crc_target; /* final target CRC checksum */

	int flags;		  /* flags (see below) */
	int private_bits; /* private bits (see below) */

	mad_timer_t duration; /* audio playing time of frame */
};

struct MadFrame
{
	MadFrame();
	~MadFrame();
	MadFrame(const MadFrame&) = delete; // wg. rc

	int decode(MadStream*);

	MadHeader header; /* MPEG audio header */

	int options; /* decoding options (from stream) */
	int rc = 0;	 /* RCPtr<> */

	mad_fixed_t sbsample[2][36][32];   /* synthesis subband filter samples */
	mad_fixed_t (*overlap)[2][32][18]; /* Layer III block overlap data */
	mad_fixed_t (*tmp)[32][3][6];	   /* III_reorder() */
	mad_fixed_t (*xr)[2][576];		   /* III_decode() */

private:
	int		  decode_layer_I(MadStream*);	// layer12.cpp
	int		  decode_layer_II(MadStream*);	// layer12.cpp
	int		  decode_layer_III(MadStream*); // layer3.cpp
	mad_error III_decode(mad_bitptr* ptr, struct SideInfo* si, uint nch);
	void	  III_reorder(mad_fixed_t xr[576], const struct Channel*, const uchar sfbwidth[39]);
};

#define MAD_NCHANNELS(header) ((header)->mode ? 2 : 1)
#define MAD_NSBSAMPLES(header)        \
	((header)->layer == MAD_LAYER_I ? \
		 12 :                         \
		 (((header)->layer == MAD_LAYER_III && ((header)->flags & MAD_FLAG_LSF_EXT)) ? 18 : 36))

enum {
	MAD_FLAG_NPRIVATE_III = 0x0007, /* number of Layer III private bits */
	MAD_FLAG_INCOMPLETE	  = 0x0008, /* header but not data is decoded */

	MAD_FLAG_PROTECTION = 0x0010, /* frame has CRC protection */
	MAD_FLAG_COPYRIGHT	= 0x0020, /* frame is copyright */
	MAD_FLAG_ORIGINAL	= 0x0040, /* frame is original (else copy) */
	MAD_FLAG_PADDING	= 0x0080, /* frame has additional slot */

	MAD_FLAG_I_STEREO	= 0x0100, /* uses intensity joint stereo */
	MAD_FLAG_MS_STEREO	= 0x0200, /* uses middle/side joint stereo */
	MAD_FLAG_FREEFORMAT = 0x0400, /* uses free format bitrate */

	MAD_FLAG_LSF_EXT	  = 0x1000, /* lower sampling freq. extension */
	MAD_FLAG_MC_EXT		  = 0x2000, /* multichannel audio extension */
	MAD_FLAG_MPEG_2_5_EXT = 0x4000	/* MPEG 2.5 (unofficial) extension */
};

enum {
	MAD_PRIVATE_HEADER = 0x0100, /* header private bit */
	MAD_PRIVATE_III	   = 0x001f	 /* Layer III private bits (up to 5) */
};
