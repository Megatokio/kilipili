// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "Pixmap_wAttr.h"

namespace kio::Graphics
{


// instantiate everything.
// the linker will know what we need:

template class Pixmap<colormode_a1w1_i4>;
template class Pixmap<colormode_a1w1_i8>;
template class Pixmap<colormode_a1w1_rgb>;
template class Pixmap<colormode_a1w2_i4>;
template class Pixmap<colormode_a1w2_i8>;
template class Pixmap<colormode_a1w2_rgb>;
template class Pixmap<colormode_a1w4_i4>;
template class Pixmap<colormode_a1w4_i8>;
template class Pixmap<colormode_a1w4_rgb>;
template class Pixmap<colormode_a1w8_i4>;
template class Pixmap<colormode_a1w8_i8>;
template class Pixmap<colormode_a1w8_rgb>;
template class Pixmap<colormode_a2w1_i4>;
template class Pixmap<colormode_a2w1_i8>;
template class Pixmap<colormode_a2w1_rgb>;
template class Pixmap<colormode_a2w2_i4>;
template class Pixmap<colormode_a2w2_i8>;
template class Pixmap<colormode_a2w2_rgb>;
template class Pixmap<colormode_a2w4_i4>;
template class Pixmap<colormode_a2w4_i8>;
template class Pixmap<colormode_a2w4_rgb>;
template class Pixmap<colormode_a2w8_i4>;
template class Pixmap<colormode_a2w8_i8>;
template class Pixmap<colormode_a2w8_rgb>;


} // namespace kio::Graphics
