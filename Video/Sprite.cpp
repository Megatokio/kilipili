// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Sprite.h"

namespace kio::Video
{

volatile int hot_row		  = 0;
spin_lock_t* sprites_spinlock = nullptr;

} // namespace kio::Video
