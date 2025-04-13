// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Trace.h"
#include "basic_math.h"
#include <stdio.h>

#if !defined OPTION_STACK_TRACE || OPTION_STACK_TRACE

namespace kio
{

Trace::Path Trace::path[2];

void Trace::print(uint core)
{
	Path& stack = path[core];
	for (uint i = 0; i < min(stack.depth, maxdepth); i++)
	{
		printf("core%u: %u: %s\n", core, i, stack.procs[i]); //
	}
}

} // namespace kio

#endif
