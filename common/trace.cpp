// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "trace.h"
#include "basic_math.h"
#include <stdio.h>

#if !defined KILIPILI_OPTION_STACK_TRACE || KILIPILI_OPTION_STACK_TRACE

namespace kilipili
{

  #ifdef PICO
Trace traces[2];
  #else
thread_local Trace my_trace;
  #endif


void Trace::print()
{
	for (uint i = 0; i < min(depth, maxdepth); i++)
	{
		printf("core%u: %u: %s\n", get_core_num(), i, names[i]); //
	}
}

} // namespace kilipili

#endif
