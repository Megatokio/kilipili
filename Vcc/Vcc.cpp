// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Vcc.h"
#include "Array.h"
#include "Names.h"
#include "ObjCode.h"
#include "Opcodes.h"
#include "Signature.h"
#include "Symbol.h"
#include "Type.h"
#include "Var.h"
#include "algorithms.h"
#include "cstrings.h"
#include "idf_ids.h"
#include <math.h>
#include <pico/stdlib.h>

namespace kio::Vcc
{


// ################################# MEMORY MAP ########################################

/*
		ram:
			vstack[]	grows down
			<-->
			rstack[]	grows up
			<-->
			gvars[]		grow up
		
		rom:
			<-->
			new_code[]	grows up
			code[]		grows up
		
		heap:
			arrays 
			strings
*/


// ################################# DATA TYPE ########################################

enum EnumID : uint16 { EnumNotFound = 0xffffu };
enum StructID : uint16 { StructNotFound = 0xffffu };

static constexpr Type tVoid {};
static constexpr Type tUint8 {UINT8};
static constexpr Type tUint16 {UINT16};
static constexpr Type tUint {UINT};
static constexpr Type tUlong {ULONG};
static constexpr Type tInt8 {INT8};
static constexpr Type tInt16 {INT16};
static constexpr Type tInt {INT};
static constexpr Type tLong {LONG};
static constexpr Type tFloat {FLOAT};
static constexpr Type tDouble {DOUBLE};
static constexpr Type tString {STRING};
static constexpr Type tChar {UINT8, 1, 0, 0, tCHAR};
static constexpr Type tBool {UINT8, 1, 0, 0, tBOOL};


// struct info:

struct Class
{
	struct DataMember
	{
		Type  type;
		IdfID name;
		uint  offset;
	};
	struct MemberFunction
	{
		Type  type;
		IdfID name;
	};

	Array<DataMember>	  data_members;
	Array<MemberFunction> member_functions;
};


// ################################# OPERATOR PRIORITY ########################################

enum // Operator Priorities
{
	pAny   = 0,	   // whole expression: up to ')' or ','
	pComma = pAny, //
	pAssign,	   //
	pTriadic,	   // ?:
	pBoolean,	   // && ||
	pCmp,		   // comparisions:	 lowest priority
	pAdd,		   // add, sub
	pMul,		   // mul, div, rem
	pAnd,		   // bool/masks:		 higher than add/mult
	pShift,		   // rot/shift
	pUna		   // unary operator:	 highest priority
};


// ################################# INITIALIZE ########################################

void Vcc::setup(uint romsize, uint ramsize)
{
	names.init();

	ram		   = new Var[ramsize];
	rom		   = new uint16[romsize];
	ram_size   = ramsize;
	rom_size   = romsize;
	gvars_size = 0;
	code_size  = 0;

	symbols.add(tBOOL, new EnumDef(tBOOL, UINT8));
	symbols.add(tCHAR, new EnumDef(tCHAR, UINT8));
	symbols.add(tTRUE, new ConstDef(tBool, true));
	symbols.add(tFALSE, new ConstDef(tBool, false));
}


// ################################# COMPILE ########################################

bool Vcc::test_word(IdfID idf)
{
	if (next_word() == idf) return true;
	putback_word();
	return false;
}

void Vcc::expect(IdfID idf)
{
	if (test_word(idf)) return;
	else throw usingstr("expected '%s'", names[idf]);
}

ViSymbol* Vcc::value(uint prio)
{
	Var		  var;
	ViSymbol* v;
	IdfID	  idf = next_word(&var);

	switch (idf)
	{
	case tNL: throw "value expected";
	case t_LONG: TODO();
	case t_INT: v = newViSymbolIval(var, tInt); break;
	case t_CHAR: v = newViSymbolIval(var, tChar); break;
	case t_FLOAT: v = newViSymbolIval(var, tFloat); break;
	case t_STRING: v = newViSymbolIval(var, tString); break;

	case tRKauf:
		v = value(pAny);
		expect(tRKzu);
		break;

	case tNOT:
	{
		v		   = value(pUna)->deref();
		Type ztype = v->rtype;
		if (ztype.is_numeric())
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			static constexpr Opcode o[] = {NOP, NOT, NOT, NOT, NOTl, NOT, NOT, NOT, NOTl, NOTf, NOTd, NOTv};
			assert(ztype < NELEM(o));
			v = newViSymbolOpcode(o[ztype], tBool, v);
		}
		else if (ztype == tVoid) throw "not numeric";
		else // ptr == nullptr ?
		{
			assert(ztype.size_of() == sizeof(int));
			v = newViSymbolOpcode(NOT, tBool, v);
		}
		break;
	}
	case tCPL:
	{
		v		   = value(pUna)->deref();
		Type ztype = v->rtype;
		if (ztype.is_numeric())
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			static constexpr Opcode o[] = {NOP, CPL, CPL, CPL, CPLl, CPL, CPL, CPL, CPLl, NOP, NOP, CPLv};
			static constexpr Type t[] = {VOID, UINT, UINT, UINT, ULONG, UINT, UINT, UINT, ULONG, VOID, VOID, VARIADIC};

			assert(ztype < NELEM(o));
			if (o[ztype] == NOP) throw "not supported";
			v = newViSymbolOpcode(o[ztype], t[ztype], v);
			break;
		}
		else throw "not numeric";
	}
	case tSUB:
	{
		v		   = value(pUna)->deref();
		Type ztype = v->rtype;
		if (ztype.is_numeric())
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			static constexpr Opcode o[] = {NOP, NEG, NEG, NEG, NEGl, NEG, NEG, NEG, NEGl, NEGf, NEGd, NEGv};
			static constexpr Type	t[] = {VOID, INT, INT, INT, LONG, INT, INT, INT, LONG, FLOAT, DOUBLE, VARIADIC};

			assert(ztype < NELEM(o));
			if (o[ztype] == NOP) throw "not supported";
			v = newViSymbolOpcode(o[ztype], t[ztype], v);
			break;
		}
		else throw "not numeric";
	}
	case tADD:
	{
		v = value(pUna)->deref();
		if (!v->rtype.is_numeric()) throw "not numeric";
		break;
	}
	default:
	{
		Symbol* symbol = symbols[idf];
		if (!symbol) throw usingstr("'%s' not found", names[idf]);

		if (ConstDef* constdef = symbol->as_const_def())
		{
			assert(!constdef->rtype.is_vref);
			if (constdef->rtype.size_on_top() > 4) throw "TODO symbol size > 4";
			v = newViSymbolIval(constdef->value.i32, constdef->rtype);
		}
		else if (GVarDef* gvar = symbol->as_gvar_def())
		{
			assert(gvar->rtype.is_vref);
			v = newViSymbol(ViSymbol::GVAR, gvar, gvar->rtype);
		}
		else if (LVarDef* lvar = symbol->as_lvar_def())
		{
			assert(lvar->rtype.is_vref);
			v = newViSymbol(ViSymbol::LVAR, lvar, lvar->rtype);
		}
		else if (Callable* fu = symbol->as_callable())
		{
			assert(fu->rtype.basetype == PROC);
			assert(fu->rtype.info < signatures.count());
			Signature& signature = signatures[fu->rtype.info];

			if (auto* o = symbol->as_opcode_def()) v = newViSymbolOpcode(o->opcode, signature.rtype, signature.argc);
			else if (symbol->isa_inline()) v = newViSymbol(ViSymbol::INLINE, fu, signature.rtype, signature.argc);
			else if (symbol->isa_proc()) v = newViSymbol(ViSymbol::PROC, fu, signature.rtype, signature.argc);
			else IERR();

			for (uint i = 0; i < signature.argc; i++)
			{
				if (i) expect(tCOMMA);
				v->args[i] = value(pAny)->cast_to(signature.args[i]);
			}
			expect(tRKzu);
		}
		else IERR();
		break;
	}

		// expect operator

	o:
		IdfID oper = next_word_as_operator();

		switch (oper)
		{
		case tNL: return v;
		case tINCR: // ++
		case tDECR: // --
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			static constexpr Opcode oo[2][12] = {
				{NOP, INCRb, INCRs, INCR, INCRl, INCRb, INCRs, INCR, INCRl, INCRf, INCRd, INCRv},
				{NOP, DECRb, DECRs, DECR, DECRl, DECRb, DECRs, DECR, DECRl, DECRf, DECRd, DECRv}};

			if (!(v->rtype.is_vref)) throw "vref required";
			Type ztype = v->rtype.strip_enum();
			if (!ztype.is_numeric()) throw "numeric type required";

			Opcode o = oo[oper - tINCR][ztype];
			if (o == NOP) throw "not supported";
			v = newViSymbolOpcode(o, ztype);
			goto o;
		}
		case tSL:
		case tSR:
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			static constexpr Opcode oo[2][12] = {
				{NOP, SL, SL, SL, SLl, SL, SL, SL, SLl, SLf, SLd, SLv},
				{NOP, SR, SR, SR, SRl, SRu, SRu, SRu, SRul, SRf, SRd, SRv}};

			if (prio >= pShift) return v;
			v = v->deref();
			if (!v->rtype.is_numeric()) throw "numeric type required";
			Type rtype = v->rtype.arithmetic_result_type();

			Opcode o = oo[oper - tSL][rtype];
			if (o == NOP) throw "not supported";
			v = newViSymbolOpcode(o, rtype, v, value(pShift)->cast_to(INT));
			goto o;
		}
		case tAND:
		case tOR:
		case tXOR:
		case tMUL:
		case tDIV:
		case tMOD:
		case tADD: // TODO: + für strings
		case tSUB:
		{
			uint8 opri[] = {pAdd, pAdd, pMul, pMul, pMul, pAnd, pAnd, pAnd};

			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			static constexpr Opcode x		  = NOP;
			static constexpr Opcode oo[8][12] = {
				{x, ADD, ADD, ADD, ADDl, ADD, ADD, ADD, ADDl, ADDf, ADDd, ADDv},
				{x, SUB, SUB, SUB, SUBl, SUB, SUB, SUB, SUBl, SUBf, SUBd, SUBv},
				{x, MUL, MUL, MUL, MULl, MUL, MUL, MUL, MULl, MULf, MULd, MULv},
				{x, DIV, DIV, DIV, DIVl, DIVu, DIVu, DIVu, DIVl, DIVf, DIVd, DIVv},
				{x, MOD, MOD, MOD, MODl, MODu, MODu, MODu, MODul, x, x, MODv},
				{x, AND, AND, AND, ANDl, AND, AND, AND, ANDl, x, x, ANDv},
				{x, OR, OR, OR, ORl, OR, OR, OR, ORl, x, x, ORv},
				{x, XOR, XOR, XOR, XORl, XOR, XOR, XOR, XORl, x, x, XORv},
			};

			if (prio >= opri[oper - tADD]) return v;
			v = v->deref();
			if (!v->rtype.is_numeric()) throw "numeric type required";

			Type	  rtype = v->rtype;
			ViSymbol* v2	= value(opri[oper - tADD]);
			ViSymbol::cast_to_same(v, v2);

			// preserve smaller size and enum for &|^
			if (v->rtype.basetype != rtype.basetype || oper < tAND || oper > tXOR)
				rtype = rtype.arithmetic_result_type();

			Opcode o = oo[oper - tADD][rtype];
			if (o == NOP) throw "not supported";
			v = newViSymbolOpcode(o, rtype, v, v2);
			goto o;
		}
		case tEQ:
		case tNE: // TODO == != für strings
		case tLT:
		case tGT:
		case tLE:
		case tGE:
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			static constexpr Opcode oo[6][12] = {
				{NOP, EQ, EQ, EQ, EQl, EQ, EQ, EQ, EQl, EQf, EQd, EQv},
				{NOP, NE, NE, NE, NEl, NE, NE, NE, NEl, NEf, NEd, NEv},
				{NOP, GE, GE, GE, GEl, GEu, GEu, GEu, GEul, GEf, GEd, GEv},
				{NOP, LE, LE, LE, LEl, LEu, LEu, LEu, LEul, LEf, LEd, LEv},
				{NOP, GT, GT, GT, GTl, GTu, GTu, GTu, GTul, GTf, GTd, GTv},
				{NOP, LT, LT, LT, LTl, LTu, LTu, LTu, LTul, LTf, LTd, LTv},
			};

			if (prio >= pCmp) return v;
			v = v->deref();
			if (!v->rtype.is_numeric()) throw "numeric type required";

			ViSymbol* v2 = value(pCmp);
			ViSymbol::cast_to_same(v, v2);

			assert(v->rtype < 12);
			Opcode o = oo[oper - tEQ][v->rtype];
			if (o == NOP) throw "not supported";
			v = newViSymbolOpcode(o, tBool, v, v2);
			goto o;
		}
		case tANDAND:
		case tOROR:
		{
			if (prio >= pBoolean) return v;

			v			 = v->cast_to_bool();
			ViSymbol* v2 = value(pBoolean)->cast_to_bool();
			v			 = newViSymbol(ViSymbol::PRUNING_OPERATOR, oper, tBool, 2, v, v2);
			goto o;
		}
		case tQMARK:
		{
			if (prio >= pTriadic + 1) return v;

			ViSymbol* v1 = v->cast_to_bool();
			ViSymbol* v2 = value(pTriadic);
			expect(tCOLON);
			ViSymbol* v3 = value(pTriadic);
			ViSymbol::cast_to_same(v2, v3);

			v		   = newViSymbol(ViSymbol::PRUNING_OPERATOR, oper, v2->rtype, 3, v1, v2);
			v->args[2] = v3;
			goto o;
		}
		case tGL:
		{
			static constexpr Opcode oo[] = {NOP, POKE8, POKE16, NOP, POKE, NOP, NOP, NOP, POKEl};

			if (prio >= pAssign) return v;
			if (!v->rtype.is_vref) throw "vref required";
			Type ztype = v->rtype.strip_vref();
			uint sz	   = ztype.size_of();

			ViSymbol* v2 = value(pAssign);
			if (!ztype.is_integer() || !v2->rtype.is_integer() || ztype.size_on_top() != v2->rtype.size_on_top())
				v2 = v2->cast_to(ztype);

			assert(sz <= 8);
			Opcode o = oo[sz];
			if (o == NOP) throw "not supported";
			return newViSymbolOpcode(o, tVoid, v, v2);
		}
		case tADDGL: // TODO: += für strings
		case tSUBGL:
		case tMULGL:
		case tDIVGL:
		case tANDGL:
		case tORGL:
		case tXORGL:
		case tSLGL:
		case tSRGL:
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			// clang-format off
			static constexpr Opcode x		   = NOP;
			static constexpr Opcode oo[9][12] = {
				{x,	ADDGLb, ADDGLs, ADDGL,	ADDGLl,	ADDGLb, ADDGLs, ADDGL,	ADDGLl,	 ADDGLf, ADDGLd, ADDGLv},
				{x,	SUBGLb, SUBGLs, SUBGL,	SUBGLl, SUBGLb, SUBGLs, SUBGL,	SUBGLl,	 SUBGLf, SUBGLd, SUBGLv},
				{x,	x,		x,		MULGL,	MULGLl, x,		x,		MULGL,	MULGLl,	 MULGLf, MULGLd, MULGLv},
				{x,	x,		x,		DIVGL,	DIVGLl, x,		x,		DIVGLu, DIVGLlu, DIVGLf, DIVGLd, DIVGLv},
				{x,	ANDGLb, ANDGLs, ANDGL,	ANDGLl, ANDGLb,	ANDGLs,	ANDGL,	ANDGLl,	 x, x, ANDGLv },
				{x,	ORGLb,	ORGLs,	ORGL,	ORGLl,	ORGLb,  ORGLs,	ORGL,	ORGLl,	 x, x, ORGLv  },
				{x,	XORGLb, XORGLs, XORGL,	XORGLl, XORGLb,	XORGLs, XORGL,	XORGLl,	 x, x, XORGLv },
				{x,	x,		x,		SLGL,	SLGLl,	x,		x,		SLGL,	SLGLl,	 x, x, SLGLv  },
				{x,	x,		x,		SRGL,	SRGLl,	x,		x,		SRGLu,	SRGLlu,	 x, x, SRGLv  } };
			// clang-format on

			if (prio >= pAssign) return v;
			if (!v->rtype.is_vref) throw "vref required";
			Type ztype = v->rtype.strip_vref();
			if (!ztype.is_numeric()) throw "numeric type required";

			ViSymbol* v2 = value(pAssign);
			if (!ztype.is_integer() || !v2->rtype.is_integer() || ztype.size_on_top() != v2->rtype.size_on_top())
				v2 = v2->cast_to(ztype);

			assert(ztype < 12);
			Opcode o = oo[oper - tADDGL][ztype];
			if (o == NOP) throw "opcode not supported for data type";
			return newViSymbolOpcode(o, tVoid, v, v2);
		}
		case tEKauf:
		{
			static constexpr Opcode ati[] = {NOP, ATI8, ATI16, NOP, ATI, NOP, NOP, NOP, ATIl};

			if (!v->rtype.is_array()) throw "array required";

			do {
				if (v->rtype.dims == 0) throw "too many subscripts";
				ViSymbol* v2 = value(pAny)->cast_to(tUint);
				v			 = v->deref();
				Type itype	 = v->rtype.strip_dim();
				uint sz		 = itype.size_of();
				assert(sz <= 8);
				if (ati[sz] == NOP) throw "not supported";

				v = newViSymbolOpcode(ati[sz], itype.add_vref(), v, v2);
			}
			while (test_word(tCOMMA));

			expect(tEKzu);
			goto o;
		}
		case tRKauf: // proc ( arguments ... )
		{
			v->deref();
			if (!v->rtype.is_callable()) throw "not callable";
			assert(v->rtype.info < signatures.count());
			Signature& signature = signatures[v->rtype.info];

			ViSymbol* v2 = v;
			v			 = newViSymbolOpcode(CALL, signature.rtype, signature.argc + 1);
			v->args[0]	 = v2;

			for (uint i = 0; i < signature.argc; i++)
			{
				if (i) expect(tCOMMA);
				v->args[1 + i] = value(pAny)->cast_to(signature.args[i]);
			}
			expect(tRKzu);
			goto o;
		}
		case tDOT:
		{
			TODO();
		}
		default: break;
		}

		putback_word();
		return v;
	}
}


void Vcc::compile_const()
{
	// const name = value , ..

	do {
		IdfID name = next_word();
		if (!isaName(name)) throw "name expected";
		if (symbols[name] != nullptr) throw "name exists";
		expect(tGL);
		ViSymbol* v = value(pAny);
		if (!v->is_ival()) throw "immediate expression expected";
		symbols.add(name, new ConstDef(v->rtype, v->value()));
	}
	while (test_word(tCOMMA));
	expect(tNL);
}

void Vcc::compile_enum()
{
	// enum name =
	//		name [ = value ] , ..
	//		..

	// get enum id from enums++
	// store IdfID name in Type.info

	IdfID name = next_word();
	if (!isaName(name)) throw "name expected";
	if (symbols[name] != nullptr) throw "name exists";
	symbols.add(name, new EnumDef(name, INT));
	expect(tGL);

	int	 value = 0;
	Type type  = Type::make_enum(name, INT);

	do {
		name = next_word();
		if (!isaName(name)) throw "name expected";
		if (symbols[name] != nullptr) throw "name exists";
		if (test_word(tGL))
		{
			ObjCode v = this->value(pAny);
			if (!v.is_ival()) throw "immediate expression expected";
			if (!v.rtype.is_integer()) throw "int value expected";
			if (v.rtype.is_unsigned_int() && v.value() < 0) throw "value too large";
			value = v.value();
		}

		symbols.add(name, new ConstDef(type, value++));
	}
	while (test_word(tCOMMA));
	expect(tNL);
}

void Vcc::compile_struct()
{
	// type name =
	//		type name , ..
	//		name ( args -- rtype ) [ instructions ]
	//		..

	//TODO
}

ObjCode Vcc::compile_definition()
{
	// type name = value
	// type name , ..
	// type name ( args ) instructions

	TODO(); //
}

ObjCode Vcc::compile_block(uint indent)
{
	// either s.th. follows on the same line
	// or following lines are indented

	if (peek_word() == tEOF) throw "unexpected eof";
	if (test_word(tNL) && get_indent() <= indent) throw "indented block expected";
	else return compile(indent);
}

ObjCode Vcc::compile(uint indent)
{
	// compile everything with the current indent into one block

	indent += 1;

	ObjCode	   objcode;
	static Var var;

	//if (test_word(source, ":")) objcode.append(compile_definition(source));

	for (;;)
	{
		IdfID id = peek_word();

		switch (id)
		{
		case tEOF: return objcode;			// end of source
		case tCOLON: next_word(); continue; // empty statement
		case tNL:							// indented block may follow
		{
			uint new_indent = get_indent();
			if (new_indent <= indent) return objcode;
			if (test_word(tCOLON)) objcode.append(compile_definition());
			continue;
		}

			//case tCONST: TODO(); // define const value
			//case tENUM: TODO(); // define const values by enumeration
			//case tTYPE: TODO();	 // define new type (struct)
			//case tINT: TODO();	 // define int var or proc
			//case tSTR: TODO();	 // define str var or proc
			//case tFLOAT: TODO(); // define float var or proc
			//case tCHAR: TODO();	 // define char var or proc

		case tIF:
		{
			objcode.append(value(pAny));
			objcode.cast_to(INT);
			objcode.append_opcode(JZ, VOID);
			objcode.append_label_ref(num_labels);
			objcode.append(compile_block(indent));

			while (test_word(tELIF)) { TODO(); }
			if (test_word(tELSE)) TODO();
			TODO(); //store code
		}
		case tSWITCH:
		{
			ObjCode z = value(pAny);
			z.cast_to(INT);
			expect(tCOLON);
			while (test_word(tCASE)) TODO();
			if (test_word(tDEFAULT)) TODO();
			TODO(); //store code
		}
		case tBREAK:
		{
			if (num_nested_switches == 0) throw "not in switch";
			TODO();
		}
		case tGKauf:
		{
			while (!test_word(tGKzu)) objcode.append(compile(indent));
			continue;
		}
		case tFOR: TODO();
		case tDO:
		{
			TODO();
		}
		case tWHILE:
		{
			if (num_nested_loops == 0) throw "not in loop";
			ObjCode z = value(pAny);
			z.cast_to(INT);
			TODO();
		}
		case tUNTIL:
		{
			if (num_nested_loops == 0) throw "not in loop";
			ObjCode z = value(pAny);
			z.cast_to(INT);
			TODO();
		}
		case tEXIT:
		{
			if (num_nested_loops == 0) throw "not in loop";
			TODO();
		}
		case tNEXT:
		{
			if (num_nested_loops == 0) throw "not in loop";
			TODO();
		}
		case tRETURN:
		{
			if (!in_proc_def) throw "not in proc def";
			TODO(); //if (rtype != VOID) compile_value(rtype);
					//return tCOLON;
		}
		case t_IDF:
		{
			// proc name => call proc
			// var name => assign var
			// typename => define proc or var
			Symbol* word = symbols[id];
			if (word) TODO(); //compile_word(word);
			else throw "not found";
		}
		default: break;
		}
		throw "unexpected";
	}
}


constexpr char opcode_names[][14] = {
#define M(MNEMONIC, NAME, ARGS) NAME
#include "Opcodes.h"
};

enum OpcodeArgument : uint8 {
	NOARG,
	ARGi32,
	ARGu32,
	ARGi32_DISTi32,
	DESTu32,
	DISTi32,
};

constexpr uint8 opcode_arguments[] = {
#define M(MNEMONIC, NAME, ARGS) ARGS
#include "Opcodes.h"
};

void Vcc::disass(const ObjCode& objcode, void (*print)(cstr))
{
	for (uint i = 0; i < objcode.cnt; i++)
	{
		uint o = objcode[i];
		if (o >= 0x8000)
		{
			print(usingstr("Label %u:", o - 0x8000));
			continue;
		}

		assert(o < NELEM(opcode_names));
		assert(o < NELEM(opcode_arguments));
		cstr name = opcode_names[o];
		uint args = opcode_arguments[o];

		switch (args)
		{
		case NOARG: print(name); continue;
		case DISTi16: print(usingstr("%s L%i", name, int16(objcode[++i]))); continue;
		case ARGi16: print(usingstr("%s %i", name, int16(objcode[++i]))); continue;
		case ARGu16: print(usingstr("%s %u", name, uint16(objcode[++i]))); continue;
		case DESTu32:
		{
			uint lo = objcode[++i];
			print(usingstr("%s %i", name, lo + 0x10000 * objcode[++i]));
			continue;
		}
		case ARGi16_DISTi16:
		{
			int a1 = int16(objcode[++i]);
			print(usingstr("%s %i,%i", name, a1, int16(objcode[++i])));
			continue;
		}
		default: IERR();
		}
	}
}

void Vcc::optimize(ObjCode&) { TODO(); }

void Vcc::remove_labels(ObjCode& objcode)
{
	// for each position of a label def
	// remember position and shift following source

	Array<uint16*> label_positions;
	constexpr uint sizeof_args[] = {0, 1, 1, 2, 2, 1};

	uint16* q = objcode.code;
	uint16* z = q;
	uint16* e = q + objcode.cnt;
	while (q < e)
	{
		uint16 o = *q++;
		if (o < 0x8000)
		{
			assert(o < NELEM(opcode_arguments));
			*z++ = o;
			for (uint i = 0; i < sizeof_args[opcode_arguments[o]]; i++) *z++ = *q++;
		}
		else // label def:
		{
			uint label = o + 0x8000;
			label_positions.grow(label + 1);
			label_positions[label] = z;
		}
	}

	objcode.cnt = uint(z - objcode.code);

	// for each jump opcode
	// replace label ref with jump distance

	q = objcode.code;
	e = q + objcode.cnt;
	while (q < e)
	{
		uint16 o = *q++;
		assert(o < NELEM(opcode_arguments));
		uint arg_id = opcode_arguments[o];
		if (arg_id == DISTi16)
		{
			uint label = *q;
			assert(label < label_positions.count() && label_positions[label] != nullptr);
			assert(label_positions[label] >= objcode.code && label_positions[label] < objcode.code + objcode.cnt);
			int d = label_positions[label] - q;
			assert(d == int16(d));
			*q++ = uint16(d);
		}
		q += sizeof_args[arg_id];
	}
}

ObjCode Vcc::compile(cstr& source)
{
	this->source = source;
	ObjCode objcode;

	try
	{
		objcode = compile(0 /*indent*/);
		optimize(objcode);
		remove_labels(objcode);
	}
	catch (cstr e)
	{
		printf("error: %s", e);
	}
	catch (std::exception& e)
	{
		printf("error: %s", e.what());
	}
	catch (...)
	{
		printf("unknown exception");
	}
}

} // namespace kio::Vcc


/*








































*/
