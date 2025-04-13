// Copyright (c) 2024 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "stack_guard.h"
#include "cdefs.h"
#include "standard_types.h"
#include <pico.h>
#include <string.h>

extern char __scratch_x_end__;
extern char __scratch_y_end__;

namespace kio
{

static size_t stack_bottom(uint core) noexcept
{
	//instead of #include "utilities.h":
	return size_t(core == 0 ? &__scratch_y_end__ : &__scratch_x_end__);
}

void init_stack_guard()
{
	// fill free stack of current core with magic number:

	uint tmp = 0xa5a5a5a5;
	ptr	 e	 = ptr(&tmp);
	ptr	 p	 = ptr(stack_bottom(get_core_num()));
	assert(uint(e - p) < 4 kB); // stack pages are 4k

	// don't use memset because the call to memset may push some values which are then overwritten!
	for (; p < e; p += 4) { *reinterpret_cast<volatile uint*>(p) = 0xe5e5e5e5; }
}

void test_stack_guard(uint core)
{
	// test whether a core overflowed the stack recently:

	ptr				 p			= ptr(stack_bottom(core));
	constexpr uint32 deadbeef[] = {0xe5e5e5e5, 0xe5e5e5e5, 0xe5e5e5e5, 0xe5e5e5e5};
	if (memcmp(p, deadbeef, sizeof(deadbeef))) panic("core %u: stack overflow", core);
}

uint calc_stack_guard_min_free(uint core)
{
	// calculate the minimum amount of free memory on stack at any time
	// since the stack guard was initialized:

	ptr	 p = ptr(stack_bottom(core));
	uint n = 0;
	while (*p++ == 0xe5) { n++; }
	return n;
}

} // namespace kio
