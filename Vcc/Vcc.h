// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Names.h"
#include "ObjCode.h"
#include "Var.h"
#include "idf_id.h"
#include "standard_types.h"
#include <utility>

namespace kio::Vcc
{


extern void setup(uint romsize, uint ramsize);
extern Var	execute(Var* ram, const uint16* ip, const uint16** rp, Var* sp);

class Vcc
{
	Vcc();

	void	setup(uint romsize, uint ramsize);
	ObjCode compile(cstr& source);
	ObjCode value(uint prio);
	void	compile_const();
	void	compile_enum();
	void	compile_struct();
	ObjCode compile_definition();
	ObjCode compile_block(uint indent);
	ObjCode compile(uint indent);

	IdfID next_word();
	IdfID next_word(Var*);
	IdfID next_word_as_operator();
	IdfID peek_word();
	void  putback_word();
	uint  get_indent();

	bool test_word(IdfID);
	void expect(IdfID);

	void disass(const ObjCode&, void (*)(cstr));
	void optimize(ObjCode&);
	void remove_labels(ObjCode&);

	uint16* rom;
	Var*	ram;

	uint ram_size;	 // allocated size (in int32)
	uint rom_size;	 // allocated size (in uint16)
	uint gvars_size; // end of used gvars, start of gap
	uint code_size;	 // end of used code, where to store new code

	cstr source; // source pointer

	Names						  names;
	HashMap<uint, struct Symbol*> symbols;	  // note: uint = idf_id
	Array<struct Signature>		  signatures; // for Type
	Array<struct Class>			  classes;	  // for Type

	uint   num_nested_loops	   = 0;
	uint   num_nested_switches = 0;
	bool   in_proc_def		   = false;
	uint16 num_labels		   = 0; // serial number for new labels
};

} // namespace kio::Vcc
