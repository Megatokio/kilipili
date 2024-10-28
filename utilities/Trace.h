// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "standard_types.h"

#if defined DEBUG && !defined UNIT_TEST
  #include <pico.h>
  #include <pico/sync.h>

namespace kio
{

class Trace
{
public:
	static constexpr uint maxdepth = 8;

	struct Path
	{
		cstr procs[maxdepth];
		uint depth = 0;

		void push(cstr fu) noexcept
		{
			if (depth < maxdepth) procs[depth] = fu;
			__dmb();
			depth += 1;
		}
		void pop() noexcept { depth -= 1; }
	};

	static Path path[2];

	Trace(cstr func) noexcept { path[get_core_num()].push(func); }
	~Trace() noexcept { path[get_core_num()].pop(); }

	static void print(uint core);
};

  #define trace(func) Trace _trace(func)

extern int sm_print_trace() noexcept;

} // namespace kio

#else

namespace kio
{
inline void trace(cstr) {}
inline int	sm_print_trace() noexcept { return 0; }
} // namespace kio

#endif
