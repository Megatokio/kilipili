// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Opcodes.h"
#include "Symbol.h"
#include "Type.h"
#include "Var.h"
#include "no_copy_move.h"
#include "standard_types.h"
#include <new>
#include <string.h>


namespace kio::Vcc
{


/*	——————————————————————————————
	Virtual intermediate opcode symbol:
	for symbol tree of an expression	
*/
struct ViSymbol
{
	enum ID : uint16 {
		OPCODE,
		IVAL,	// literal, const
		PROC,	// Symbol
		INLINE, // Symbol
		GVAR,	// Symbol
		LVAR,	// Symbol
		PRUNING_OPERATOR,
	};

	ID	   id;
	uint16 argc;
	Type   rtype;
	union
	{
		Symbol*	   symbol;
		InlineDef* inline_symbol; // INLINE
		ProcDef*   proc_symbol;	  // PROC
		GVarDef*   gvar_symbol;	  // GVAR
		LVarDef*   lvar_symbol;	  // LVAR
		Opcode	   opcode;		  // OPCODE
		Var		   ival;		  // IVAL
		IdfID	   name_id;		  // PRUNING_OPERATOR
	};
	ViSymbol* args[0];

	ViSymbol*	deref();
	ViSymbol*	cast_to_bool();
	bool		can_cast_without_conversion(Type ztype);
	ViSymbol*	cast_to(Type ztype, bool explicit_cast = false);
	static void cast_to_same(ViSymbol*, ViSymbol*);
	bool		is_ival();
	Var			value();


	~ViSymbol()
	{
		for (uint i = 0; i < argc; i++) delete args[i];
	}

private:
	NO_COPY_MOVE(ViSymbol);
	friend ViSymbol* newViSymbol(ID, Var, Type, uint, ViSymbol*, ViSymbol*);

	ViSymbol(ID id, Type rtype, Var data, uint argc) : id(id), argc(uint16(argc)), rtype(rtype), ival(data)
	{
		memset(args, 0, argc * sizeof(*args));
	}
};


inline ViSymbol*
newViSymbol(ViSymbol::ID id, Var o, Type rtype, uint argc, ViSymbol* arg1 = nullptr, ViSymbol* arg2 = nullptr)
{
	void*	  mem  = new char[sizeof(ViSymbol) + argc * sizeof(*ViSymbol::args)];
	ViSymbol* rval = new (mem) ViSymbol(id, rtype, o, argc);
	if (arg1) rval->args[0] = arg1;
	if (arg2) rval->args[0] = arg2;
	return rval;
}

inline ViSymbol* newViSymbol(ViSymbol::ID id, Symbol* symbol, Type rtype)
{
	return newViSymbol(id, Var(symbol), rtype, 0);
}

inline ViSymbol* newViSymbol(ViSymbol::ID id, Callable* symbol, Type rtype, uint argc)
{
	return newViSymbol(id, Var(symbol), rtype, argc);
}

inline ViSymbol* newViSymbolOpcode(Opcode o, Type rtype) { return newViSymbol(ViSymbol::OPCODE, o, rtype, 0); }

inline ViSymbol* newViSymbolOpcode(Opcode o, Type rtype, ViSymbol* arg1)
{
	return newViSymbol(ViSymbol::OPCODE, o, rtype, 1, arg1);
}

inline ViSymbol* newViSymbolOpcode(Opcode o, Type rtype, ViSymbol* arg1, ViSymbol* arg2)
{
	return newViSymbol(ViSymbol::OPCODE, o, rtype, 2, arg1, arg2);
}

inline ViSymbol* newViSymbolOpcode(Opcode o, Type rtype, uint argc)
{
	return newViSymbol(ViSymbol::OPCODE, o, rtype, argc);
}

inline ViSymbol* newViSymbolIval(Var ival, Type rtype) { return newViSymbol(ViSymbol::IVAL, ival, rtype, 0); }


} // namespace kio::Vcc


/*





















*/
