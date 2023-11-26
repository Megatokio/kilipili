// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Signature.h"
#include "Type.h"
#include "Var.h"

namespace kio::Vcc
{

enum Opcode : unsigned short;

struct Symbol
{
	enum Flags {
		PROC,
		INLINE,
		OPCODE,
		GVAR,
		LVAR,
		CONST,
		ENUM,
	};

	Flags wtype;
	Type  rtype;

	bool is_callable() const noexcept { return wtype <= OPCODE; }
	bool isa_proc() const noexcept { return wtype == PROC; }
	bool isa_inline() const noexcept { return wtype == INLINE; }
	bool isa_opcode() const noexcept { return wtype == OPCODE; }
	bool isa_gvar() const noexcept { return wtype == GVAR; }
	bool isa_lvar() const noexcept { return wtype == LVAR; }
	bool isa_const() const noexcept { return wtype == CONST; }

	struct Callable*  as_callable() { return is_callable() ? reinterpret_cast<Callable*>(this) : nullptr; }
	struct ProcDef*	  as_proc_def() { return isa_proc() ? reinterpret_cast<ProcDef*>(this) : nullptr; }
	struct OpcodeDef* as_opcode_def() { return isa_opcode() ? reinterpret_cast<OpcodeDef*>(this) : nullptr; }
	struct InlineDef* as_inline_def() { return isa_inline() ? reinterpret_cast<InlineDef*>(this) : nullptr; }
	struct ConstDef*  as_const_def() { return isa_const() ? reinterpret_cast<ConstDef*>(this) : nullptr; }
	struct GVarDef*	  as_gvar_def() { return isa_gvar() ? reinterpret_cast<GVarDef*>(this) : nullptr; }
	struct LVarDef*	  as_lvar_def() { return isa_lvar() ? reinterpret_cast<LVarDef*>(this) : nullptr; }

	Symbol(Flags f, Type t) : wtype(f), rtype(t) {}
};

struct Callable : public Symbol
{
	Callable(Flags f, SigID sid) : Symbol(f, Type::make_proc(sid)) {}
};
struct OpcodeDef : public Callable
{
	Opcode opcode;
	OpcodeDef(Opcode o, SigID sid) : Callable(OPCODE, sid), opcode(o) {}
};
struct ProcDef : public Callable
{
	uint16 offset; // in rom[]
	ProcDef(uint16 offs, SigID sid) : Callable(PROC, sid), offset(offs) {}
};
struct InlineDef : public Callable
{
	uint16* code; // allocated, takes ownership
	uint16	count;
	InlineDef(uint16* code, uint16 cnt, SigID sid) : Callable(INLINE, sid), code(code), count(cnt) {}
};
struct ConstDef : public Symbol
{
	Var value;
	ConstDef(Type t, Var v) : Symbol(CONST, t), value(v) {}
};
struct GVarDef : public Symbol
{
	uint16 offset; // in ram[]
	GVarDef(uint16 offs, Type t) : Symbol(GVAR, t), offset(offs) {}
};
struct LVarDef : public Symbol
{
	uint16 offset;
	LVarDef(uint16 offs, Type t) : Symbol(GVAR, t), offset(offs) {}
};
struct EnumDef : public Symbol
{
	EnumDef(IdfID name, BaseType bt) : Symbol(ENUM, Type::make_enum(name, bt)) {}
};


} // namespace kio::Vcc
