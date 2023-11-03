// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "StackInfo.h"
#include "sm_utilities.h"
#include <stdio.h>
#include <string.h>

namespace kio
{

#undef stackinfo
#ifdef DEBUG

cstr StackInfo::stackinfo[maxstackdepth];
int	 StackInfo::stackdepth = 0;

int sm_print_stackinfo() noexcept
{
	BEGIN
	{
		static uint32 timeout = time_us_32();

		for (;;)
		{
			{
				cstr stackinfo[StackInfo::maxstackdepth];
				int	 n = StackInfo::stackdepth;
				memcpy(stackinfo, StackInfo::stackinfo, sizeof(stackinfo));
				for (int i = 0; i < n; i++)
				{
					printf("core1:%u: %s\n", i, stackinfo[i]); //
				}
			}
			SLEEP_MS(10 * 1000);
		}
	}
	FINISH
}

#else
int sm_print_stackinfo() noexcept { return 0; }
#endif


} // namespace kio
