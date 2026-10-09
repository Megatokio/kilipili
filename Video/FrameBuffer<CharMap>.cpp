// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer<CharMap>.h"
#include "common/cdefs.h"

namespace kilipili::Video
{
using namespace Graphics;

// ______________________________________________________________________________
// scanline renderers go into XRAM
// vblank callbacks go into normal RAM
// you can override this by providing the instantiation in your program!

#define define template void __section

define(XRAM ".FB_CM1") FrameBuffer<CharMap<0, 0, 1, 1, 1>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM1") FrameBuffer<CharMap<0, 0, 1, 1, 1>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM2") FrameBuffer<CharMap<4, 4, 0, 0, 0>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM2") FrameBuffer<CharMap<4, 4, 0, 0, 0>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM3") FrameBuffer<CharMap<4, 3, 1, 0, 0>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM3") FrameBuffer<CharMap<4, 3, 1, 0, 0>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM4") FrameBuffer<CharMap<4, 3, 0, 0, 1>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM4") FrameBuffer<CharMap<4, 3, 0, 0, 1>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM5") FrameBuffer<CharMap<4, 2, 1, 1, 0>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM5") FrameBuffer<CharMap<4, 2, 1, 1, 0>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM6") FrameBuffer<CharMap<4, 2, 1, 0, 1>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM6") FrameBuffer<CharMap<4, 2, 1, 0, 1>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM7") FrameBuffer<CharMap<3, 3, 1, 1, 0>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM7") FrameBuffer<CharMap<3, 3, 1, 1, 0>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM8") FrameBuffer<CharMap<3, 3, 1, 0, 1>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM8") FrameBuffer<CharMap<3, 3, 1, 0, 1>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM9") FrameBuffer<CharMap<2, 2, 1, 1, 1>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM9") FrameBuffer<CharMap<2, 2, 1, 1, 1>>::do_vblank(VideoPlane*) noexcept;
define(XRAM ".FB_CM10") FrameBuffer<CharMap<3, 2, 1, 1, 1>>::do_render(VideoPlane*, int, int, uint32*) noexcept;
define(RAM ".FB_CM10") FrameBuffer<CharMap<3, 2, 1, 1, 1>>::do_vblank(VideoPlane*) noexcept;


} // namespace kilipili::Video


/*





































*/
