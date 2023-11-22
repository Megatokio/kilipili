// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Var.h"
#include "idf_id.h"
#include "standard_types.h"
#include <utility>

namespace kio::Vcc
{

extern void setup(uint romsize, uint ramsize);
extern uint compile_instruction(cstr& source);
extern Var	execute(Var* ram, const uint16* ip, const uint16** rp, Var* sp);

extern IdfID next_word(cstr& source_ptr, Var*, bool expect_operator);
extern bool	 test_word(cstr& source_ptr, cstr word, bool expect_operator = false);
extern void	 putback_word();

} // namespace kio::Vcc
