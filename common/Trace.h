// Copyright (c) 2022 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "glue.h"
#include "standard_types.h"

#if !defined OPTION_STACK_TRACE || OPTION_STACK_TRACE

namespace kio
{

class Trace
{
public:
	static constexpr uint maxdepth = 10;

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

	Trace(cstr name) noexcept { path[get_core_num()].push(name); }
	~Trace() noexcept { path[get_core_num()].pop(); }

	static void print(uint core);
};

  #define trace(name) Trace _trace(name)

} // namespace kio

#else

  #define trace(name) void(0)

#endif
