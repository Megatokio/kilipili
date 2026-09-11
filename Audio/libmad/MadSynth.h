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
 *
 *
 * c++ adaption:
 * Copyright (c) 2026 - 2026 kio@little-bat.de
 * GPL-2.0 license
 * https://opensource.org/license/gpl-2.0
 */

#pragma once
#include "MadFrame.h"
#include "mad_types.h"

struct MadPcmBuffer
{
	uint		samplerate;		  /* sampling frequency (Hz) */
	ushort		channels;		  /* number of channels */
	ushort		length;			  /* number of samples per channel */
	mad_fixed_t samples[2][1152]; /* PCM output samples [ch][sample] */
};

struct MadSynth
{
	MadSynth() noexcept;
	~MadSynth() noexcept	  = default;
	MadSynth(const MadSynth&) = delete; // wg. rc

	void synthesize_pcm(const MadFrame*) noexcept;

	mad_fixed_t filter[2][2][2][16][8]; /* polyphase filterbank outputs */
										/* [ch][eo][peo][s][v] */

	uint phase;	 /* current processing phase */
	int	 rc = 0; // RCPtr<>

	MadPcmBuffer pcm; /* PCM output */

private:
	void synth_full(const MadFrame* frame, uint nch, uint ns) noexcept;
	void synth_half(const MadFrame* frame, uint nch, uint ns) noexcept;
};

/* single channel PCM selector */
enum { MAD_PCM_CHANNEL_SINGLE = 0 };

/* dual channel PCM selector */
enum { MAD_PCM_CHANNEL_DUAL_1 = 0, MAD_PCM_CHANNEL_DUAL_2 = 1 };

/* stereo PCM selector */
enum { MAD_PCM_CHANNEL_STEREO_LEFT = 0, MAD_PCM_CHANNEL_STEREO_RIGHT = 1 };
