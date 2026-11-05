// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Sprite.h"
#include "common/SpinLock.h"

namespace kilipili::Video
{

volatile int hot_row = 0;
SpinLock	 sprites_spinlock(SpinLock::UNUSED);

} // namespace kilipili::Video
