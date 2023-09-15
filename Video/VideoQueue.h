// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "BucketList.h"
#include "Scanline.h"
#include "scanvideo_options.h"
#include <atomic>

namespace kipili::Video
{

/*
	class VideoQueue is used for sending scanlines to the scanline __isr().
*/

class VideoQueue : BucketList<Scanline, PICO_SCANVIDEO_SCANLINE_BUFFER_COUNT>
{
public:
	using T = Scanline;

	using BucketList::buckets;
	using BucketList::MASK;
	using BucketList::SIZE;

	// Are there scanlines available for the __isr?
	uint full_avail() const noexcept { return hs_avail(); }

	// Get next scanline for __isr:
	T& get_full() noexcept { return hs_get(); }

	// Push back free scanline from __isr:
	void push_free() noexcept { hs_push(); /*__sev();*/ }
	void push_free(T& s) noexcept { hs_push(s); /*__sev();*/ } // with assert


	// Are there free scanlines available for the video controller?
	uint free_avail() const noexcept { return ls_avail(); }

	// Get next free scanline:
	T& get_free() noexcept { return ls_get(); }		   // get next
	T& get_free(uint i) noexcept { return ls_get(i); } // get ahead

	// Push back full scanline:
	void push_full() noexcept { ls_push(); }
	void push_full(T& s) noexcept { ls_push(s); } // with assert


	// clear uphill list.
	// all scanlines are now in list of free scanlines.
	// call only while the __isr is deactivated.
	void reset() noexcept { lsi = hsi; }

	// wait for uphill list to drain.
	// call only on same core as __isr while __isr is active:
	void drain() noexcept
	{
		while (full_avail()) __wfi();
	}
};


extern VideoQueue video_queue;

} // namespace kipili::Video
