// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "VgaMode.h"
#include "kilipili_cdefs.h"
#include "utilities.h"

namespace kio::Video
{

extern VgaMode vga_mode; // VGAMode in use

extern uint32		 px_per_scanline;	  // pixel per scanline
extern uint32		 cc_per_scanline;	  //
extern uint32		 px_per_frame;		  // pixel per frame
extern uint32		 cc_per_frame;		  //
extern uint			 cc_per_px;			  // cpu clock cycles per pixel octet
extern uint			 cc_per_us;			  // cpu clock cycles per microsecond
extern volatile bool in_vblank;			  // set while in vblank (set and reset ~2 scanlines early)
extern volatile int	 line_at_frame_start; // rolling line number at start of current frame
extern uint32		 time_us_at_frame_start;

/** get currently displayed line number.
	can be less than 0 (-1 or -2) immediately before frame start.
	is greater or equal to `vga_mode.height` in vblank after the active display area.
*/
inline int current_scanline() noexcept;


class VideoBackend
{
public:
	static void initialize() noexcept; // panics
	static void start(const VgaMode&) throws;
	static void stop() noexcept;
};


inline void waitForVBlank() noexcept
{
	while (!in_vblank) { wfe(); }
}

inline void waitForScanline(int scanline) noexcept
{
	// TODO: we no longer get events for every scanline!
	//		 we could setup a timer
	while (current_scanline() - scanline < 0) { wfe(); }
}


inline int current_scanline() noexcept
{
	uint time_us_in_frame = time_us_32() - time_us_at_frame_start;
	uint cc_in_frame	  = time_us_in_frame * cc_per_us;
	return int(cc_in_frame) / int(cc_per_scanline);
}

} // namespace kio::Video
