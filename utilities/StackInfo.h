// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"
#include <pico/platform.h>
#include <pico/sync.h>

namespace kio
{

#ifdef DEBUG

class StackInfo
{
public:
	static constexpr int maxstackdepth = 8;
	static cstr			 stackinfo[maxstackdepth];
	static int			 stackdepth;

	StackInfo(cstr func) noexcept
	{
		if (get_core_num() == 1)
		{
			if (stackdepth < maxstackdepth) stackinfo[stackdepth] = func;
			__dmb();
			stackdepth += 1;
		}
	}
	~StackInfo()
	{
		if (get_core_num() == 1) stackdepth -= 1;
	}
};

  #define stackinfo()    \
	StackInfo _stackinfo \
	{                    \
	  __func__           \
	}

#else
  #define stackinfo() (void)0
#endif

extern int sm_print_stackinfo() noexcept;


} // namespace kio
