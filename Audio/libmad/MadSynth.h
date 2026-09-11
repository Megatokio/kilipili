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
 * $Id: synth.h,v 1.15 2004/01/23 09:41:33 rob Exp $
 */

#pragma once
#include "MadFrame.h"
#include "mad_types.h"

struct MadPcmBuffer
{
	unsigned int   samplerate;		 /* sampling frequency (Hz) */
	unsigned short channels;		 /* number of channels */
	unsigned short length;			 /* number of samples per channel */
	mad_fixed_t	   samples[2][1152]; /* PCM output samples [ch][sample] */
};

struct MadSynth
{
	MadSynth();
	~MadSynth()				  = default;
	MadSynth(const MadSynth&) = delete; // wg. rc

	mad_fixed_t filter[2][2][2][16][8]; /* polyphase filterbank outputs */
										/* [ch][eo][peo][s][v] */

	unsigned int phase;	 /* current processing phase */
	int			 rc = 0; // RCPtr<>

	struct MadPcmBuffer pcm; /* PCM output */
};

/* single channel PCM selector */
enum { MAD_PCM_CHANNEL_SINGLE = 0 };

/* dual channel PCM selector */
enum { MAD_PCM_CHANNEL_DUAL_1 = 0, MAD_PCM_CHANNEL_DUAL_2 = 1 };

/* stereo PCM selector */
enum { MAD_PCM_CHANNEL_STEREO_LEFT = 0, MAD_PCM_CHANNEL_STEREO_RIGHT = 1 };

void MadSynth_mute(struct MadSynth*);

void MadSynth_frame(struct MadSynth*, struct MadFrame const*);
