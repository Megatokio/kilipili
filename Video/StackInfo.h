// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "kilipili_common.h"


namespace kio::Video
{

//#ifdef DEBUG

constexpr int		 maxstackdepth = 8;
extern volatile cstr stackinfo[maxstackdepth];
extern volatile int	 stackdepth;

class StackInfo
{
public:
	StackInfo(cstr func) noexcept
	{
		if (get_core_num() == 1)
		{
			if (stackdepth < maxstackdepth) stackinfo[stackdepth] = func;
			stackdepth = stackdepth + 1;
		}
	}
	~StackInfo()
	{
		if (get_core_num() == 1) stackdepth = stackdepth - 1;
	}
};

#define debuginfo()    \
  StackInfo _stackinfo \
  {                    \
	__func__           \
  }

//#else
//#define debuginfo() (void)0
//#endif


} // namespace kio::Video
