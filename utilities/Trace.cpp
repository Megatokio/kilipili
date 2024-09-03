// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#undef stackinfo
#if defined DEBUG && !defined UNIT_TEST
  
#include "Trace.h"
#include "basic_math.h"
#include "sm_utilities.h"
#include <stdio.h>

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

int sm_print_trace() noexcept
{
	BEGIN
	{
		static uint32 timeout = time_us_32();

		for (;;)
		{
			Trace::print(1);
			SLEEP_MS(10 * 1000);
		}
	}
	FINISH
}


} // namespace kio

#endif
