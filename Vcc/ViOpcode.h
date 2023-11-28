// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Opcodes.h"
#include "Type.h"
#include "Var.h"
#include "no_copy_move.h"
#include "standard_types.h"
#include <new>


namespace kio::Vcc
{

/*	——————————————————————————————
	Virtual intermediate opcode:
	for opcode tree of an expression	
*/
struct ViOpcode
{
	Opcode opcode;
	uint8  argc; // ival + stack
	bool   has_ival;
	Type   rtype;
	union
	{
		Var		  ival;
		ViOpcode* arg1 = nullptr;
		ViOpcode* args[1];
	};

	struct ViOpcodeIval*  asViOpcodeIval();	 // arg1 = ival
	struct ViOpcode1Arg*  asViOpcode1Arg();	 // arg1 != ival
	struct ViOpcode2Args* asViOpcode2Args(); // arg1 may be an ival
	struct ViOpcode3Args* asViOpcode3Args(); // arg1 may be an ival

	~ViOpcode()
	{
		uint i = has_ival;
		while (i < argc) delete args[i++];
	}

protected:
	NO_COPY_MOVE(ViOpcode);
	ViOpcode(Opcode o, Type rtype, uint8 argc, bool f, Var ival) :
		opcode(o),
		argc(argc),
		has_ival(f),
		rtype(rtype),
		ival(ival)
	{}
};


// ========================= SUBCLASSES ==============================


struct ViOpcodeNoArg : public ViOpcode
{
	ViOpcodeNoArg(Opcode o, Type rtype) : ViOpcode(o, rtype, 0, 0, 0) {}
};

struct ViOpcodeIval : public ViOpcode
{
	ViOpcodeIval(Opcode o, Type rtype, Var ival) : ViOpcode(o, rtype, 1, 1, ival) {}
};

struct ViOpcode1Arg : public ViOpcode
{
	ViOpcode1Arg(Opcode o, Type rtype, ViOpcode* arg1) : ViOpcode(o, rtype, 1, 0, arg1) {}
	ViOpcode1Arg(Opcode o, Type rtype, Var ival) : ViOpcode(o, rtype, 1, 0, ival) {}

protected:
	ViOpcode1Arg(Opcode o, Type rtype, uint argc, bool f, Var arg1) : ViOpcode(o, rtype, argc, f, arg1) {}
};

struct ViOpcode2Args : public ViOpcode1Arg
{
	ViOpcode* arg2;

	ViOpcode2Args(Opcode o, Type rt, ViOpcode* arg1, ViOpcode* arg2) : ViOpcode1Arg(o, rt, 2, 0, arg1), arg2(arg2) {}
	ViOpcode2Args(Opcode o, Type rtype, Var ival, ViOpcode* arg2) : ViOpcode1Arg(o, rtype, 2, 1, ival), arg2(arg2) {}

protected:
	ViOpcode2Args(Opcode o, Type rt, uint n, bool f, Var a1, ViOpcode* a2) : ViOpcode1Arg(o, rt, n, f, a1), arg2(a2) {}
};

struct ViOpcode3Args : public ViOpcode2Args
{
	ViOpcode* arg3;

	ViOpcode3Args(Opcode o, Type rtype, ViOpcode* arg1, ViOpcode* arg2, ViOpcode* arg3) :
		ViOpcode2Args(o, rtype, 3, 0, arg1, arg2),
		arg3(arg3)
	{}

	ViOpcode3Args(Opcode o, Type rtype, Var ival, ViOpcode* arg2, ViOpcode* arg3) :
		ViOpcode2Args(o, rtype, 3, 1, ival, arg2),
		arg3(arg3)
	{}
};


ViOpcodeIval*  ViOpcode::asViOpcodeIval() { return has_ival ? static_cast<ViOpcodeIval*>(this) : nullptr; }
ViOpcode1Arg*  ViOpcode::asViOpcode1Arg() { return argc && !has_ival ? static_cast<ViOpcode1Arg*>(this) : nullptr; }
ViOpcode2Args* ViOpcode::asViOpcode2Args() { return argc >= 2 ? static_cast<ViOpcode2Args*>(this) : nullptr; }
ViOpcode3Args* ViOpcode::asViOpcode3Args() { return argc >= 3 ? static_cast<ViOpcode3Args*>(this) : nullptr; }

} // namespace kio::Vcc


/*





















*/
