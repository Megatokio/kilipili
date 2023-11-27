// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Opcodes.h"
#include "Var.h"


namespace kio::Vcc
{

/*
	VxOpcodes are the address of tiny chunks of machine code
*/
using VxOpcode	  = const void*;
using VxOpcodePtr = VxOpcode*;

/*
	table of VxOpcodes.
	this table maps the enumeration from "Opcodes.h" to VxOpcodes.
	the table must be populated by a call to execute(NULL,NULL,NULL,NULL).
	this table is used by the Vcc compiler to generate vx object code.
*/
extern VxOpcode vx_opcodes[num_VxOpcodes];

/*
	exit flags for the vx runner on core 0 and 1.
	if the flag is set then the vx runner on this core will save it's state and exit.
	the flag is tested in JR opcodes so it will take a short time.
	buggy code may actually fail to react to the flag. but buggy code may kill the system anyway.
*/
extern bool vx_exit[2];

/*
	execute VxOpcodes.
	runs until vx_exit[core] is set or EXIT is executed.
	then it saves the current state and returns.
	special calls:
		- execute(NULL,NULL,NULL,NULL):  populate the global array `vx_opcodes[]`.
		- execute(globals,NULL,NULL,sp): resume from previous state.
	arguments:
		- globals:	address of the global variables
		- ip:		start address
		- rp:		return stack pointer    (grows up)
		- sp:		variables stack pointer	(grows down)	
	returns:
		returns the `sp` register. on the stack there are:
		- top register
		- rp register
		on the return stack there are:
		- ip register.
		a possible return value in the `top` register can be accessed at the returned address.
		pass the returned `sp` to the next call to resume at the interrupted position.
		trying to resume after opcode EXIT was executed will immediately execute EXIT again.
		if the runner can return for `vx_exit` and for EXIT, then you should set a flag 
		in globals[] to determine exit condition. Another possibility is to examine the 
		value of the `sp` to determine stack depth which should only contain the saved `top` 
		and `rp` register or the value of the `ip` register whether it points to an EXIT opcode.
*/
extern Var* execute(Var* globals, const VxOpcode* ip, const VxOpcode** rp, Var* sp);

/*
	allocate a stack and execute VxOpcodes.
			convenience method.
			runs until EXIT is executed.
	returns: the TOP register.
	note:	vx_exit can only be used to bail out a stuck job.
			on return the stack and all state is lost!
	note:	the `stack_size` is measured in words:
			100 is tight but enough for most simple jobs.
			1000 gives a decent depth for nested subroutines and recursion.
*/
extern Var* execute(Var* globals, const VxOpcode* ip, uint stack_size = 1000, uint32 timeout_ms = 0);

} // namespace kio::Vcc
