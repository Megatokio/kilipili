// Copyright (c) 1994 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include <cstdlib> // __CONCAT


// ######################################
// Initialization at start:
//   • during statics initialization
//   • only once
//   • beware of undefined sequence of inclusion!
//   • use:  -->  ON_INIT(function_to_call);
//   • use:  -->  ON_INIT([]{ init_foo(); init_bar(); });

struct on_init
{
	on_init(void (*f)()) { f(); }
};
#define ON_INIT(fu) static on_init __CONCAT(z, __LINE__)(fu)
