// Copyright (c) 2022 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "common/glue.h"
#include "common/standard_types.h"

#if !defined KILIPILI_OPTION_STACK_TRACE || KILIPILI_OPTION_STACK_TRACE

  #ifndef KILIPILI_STACK_TRACE_MAX_DEPTH
	#define KILIPILI_STACK_TRACE_MAX_DEPTH 8
  #endif


namespace kilipili
{
struct Trace
{
	static constexpr uint maxdepth = KILIPILI_STACK_TRACE_MAX_DEPTH;

	cstr names[maxdepth];
	uint depth = 0;

	void push(cstr fu) noexcept
	{
		if (depth < maxdepth) names[depth] = fu;
		depth += 1;
	}
	void pop() noexcept { depth -= 1; }
	void print();
};

  #ifdef PICO
extern Trace traces[2];
	#define my_trace traces[get_core_num()]
  #else
extern thread_local Trace my_trace;
  #endif

struct Tracer
{
	Tracer(cstr fu) { my_trace.push(fu); }
	~Tracer() { my_trace.pop(); }
};

  #define trace(name)	kilipili::Tracer _trace(name)
  #define print_trace() my_trace.print()

} // namespace kilipili

#else

  #define trace(name)	void(0)
  #define print_trace() void(0)

#endif
