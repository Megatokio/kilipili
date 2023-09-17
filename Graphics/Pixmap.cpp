// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Pixmap.h"


namespace kio::Graphics
{


// instantiate everything.
// the linker will know what we need:

template class Pixmap<colormode_i1>;
template class Pixmap<colormode_i2>;
template class Pixmap<colormode_i4>;
template class Pixmap<colormode_i8>;
template class Pixmap<colormode_rgb>;


} // namespace kio::Graphics
