// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Interp.h"

namespace kilipili::Video
{

void initializeInterpolators() noexcept
{
	assert(get_core_num() == 1);
	constexpr uint lane0 = 0;

	interp0->base[lane0] = 0; // interp0.lane0: add nothing
	if (ip0_mode != InterpMode::any) interp0->setup(ip0_mode);

	interp1->base[lane0] = 0; // interp1.lane0: add nothing
	if (ip1_mode != InterpMode::any) interp1->setup(ip1_mode);
}

} // namespace kilipili::Video
