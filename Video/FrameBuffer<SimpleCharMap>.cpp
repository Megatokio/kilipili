// Copyright (c) 2026 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "FrameBuffer<SimpleCharMap>.h"
#include "common/cdefs.h"

namespace kilipili::Video
{
using namespace Graphics;

// ______________________________________________________________________________
// scanline renderers go into XRAM
// vblank callbacks go into normal RAM
// you can override this by providing an instantiation in your program!

// gcc ignores attributes in templates!
// we must specify the section in every instantiation!
// we cannot just instantiate the class,
// we must instantiate every single function!
// at least there are only 2 versions of the SimpleCharMap...

// clang-format off

template void __section(XRAM ".FB_SCM1") FrameBuffer<SimpleCharMap, Fast>::do_render(VideoPlane*, int, int, uint32*) noexcept;
template void __section(RAM  ".FB_SCM1") FrameBuffer<SimpleCharMap, Fast>::do_vblank(VideoPlane*) noexcept;

template void __section(XRAM ".FB_SCM2") FrameBuffer<SimpleCharMap, Small>::do_render(VideoPlane*, int, int, uint32*) noexcept;
template void __section(RAM  ".FB_SCM2") FrameBuffer<SimpleCharMap, Small>::do_vblank(VideoPlane*) noexcept;

}



 
