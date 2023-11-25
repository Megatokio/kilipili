// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "Vcc.h"
#include "Var.h"
#include "algorithms.h"
#include "cstrings.h"
#include "idf_ids.h"
#include "no_copy_move.h"
#include "opcodes.h"
#include <atomic>
#include <math.h>
#include <pico/stdlib.h>

namespace kio::Vcc
{


// ############################## NAMES ###################################

static constexpr cstr idfs[] = {
#define M(ID, STR) STR
#include "idf_id.h"
};

struct Names : public HashMap<cstr, IdfID>
{
	using super = HashMap<cstr, IdfID>;
	static void init_names();

	Names() { init_names(); }
	uint16 add(cstr s)
	{
		IdfID n = IdfID(count());
		if (super::get(s, n) == n) super::add(s, n);
		return uint16(n);
	}
	using super::operator[];
	cstr		 operator[](uint id) { return idfs[id]; }
};

static Names names;

void Names::init_names()
{
	names.purge();
	names.grow(512);

	for (uint i = 0; i < NELEM(idfs); i++) { names.add(idfs[i]); }
	assert(names.count() == NELEM(idfs)); // no doublettes!
	assert(names["dup"] == tDUP);
}


// ################################# data TYPE ########################################

enum Type : uint8 {
	VOID,	  // no value
	BOOL,	  //
	CHAR,	  // character, unsigned
	INT8,	  //
	INT16,	  //
	INT,	  // integer number
	LONG,	  // long integer number
	UINT8,	  //
	UINT16,	  //
	UINT,	  // integer number
	ULONG,	  // long integer number
	FLOAT,	  // floating point number
	DOUBLE,	  // floating point number
	VARIADIC, // container for variable types
	STRING,	  // text string
	STRUCT,	  // structured variable
	PROC,	  // procedure, presumable with VREF
	VREF   = 0x20,
	ARRAY1 = 0x40, // 1-dim array
	ARRAY2 = 0x80, // 2-dim array
	ARRAY3 = 0xC0, // 3-dim array
};
inline constexpr Type operator|(Type a, int b) { return Type(b | a); }
inline constexpr Type operator&(Type a, int b) { return Type(b & a); }
inline constexpr Type operator+(Type a, int b) { return Type(b + a); }
inline constexpr Type operator-(Type a, int b) { return Type(uint8(a) - b); }

constexpr bool is_numeric(Type t) noexcept { return t >= INT && t <= VARIADIC; }
constexpr uint size_of(Type t)
{
	constexpr uint8 sz[] = {0, 1, 1, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8, 12};
	return t < STRING ? sz[t] : sizeof(ptr);
}
constexpr uint sizeshift_of(Type t)
{
	// not for VOID and VARIADIC
	constexpr uint8 sz[] = {0, 0, 0, 0, 1, 2, 3, 0, 1, 2, 3, 2, 3, 4 /*16*/};
	return t < STRING ? sz[t] : msbit(sizeof(ptr));
}

constexpr bool is_unsigned(Type t) { return t >= UINT8 && t <= ULONG; }

constexpr uint num_dimensions(Type t) { return (t >> 6) & 3; }


// ################################# WORD in dict ########################################

struct Symbol
{
	enum Flags : uint8 {
		PROC   = 0,
		INLINE = 1,
		OPCODE = 2,
		GVAR   = 3,
		LVAR   = 4,
		CONST  = 5,
	};

	IdfID name;
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
};
struct Callable : public Symbol
{
	uint8 argc;
	Type  argv[7];
};

struct OpcodeDef : public Callable
{
	uint16 opcode;
};
struct ProcDef : public Callable
{
	uint16 offset; // in rom[]
};
struct InlineDef : public Callable
{
	uint16	count;
	uint16* code; // allocated
};
struct ConstDef : public Symbol
{
	Var value;
};
struct GVarDef : public Symbol
{
	uint16 offset; // in ram[]
};
struct LVarDef : public Symbol
{
	uint16 offset; // to lp ?
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


// ####################### NUMERIC OPERATOR INFORMATION ########################################

struct NumericOperatorInfo
{
	IdfID  idf;
	uint16 opcode[7]; // INT,UINT,LONG,ULONG,FLOAT,DOUBLE,VARIADIC
	Type   args[7];
	Type   rtype[7];
	uint8  prio;
};

constexpr Type I = INT, U = UINT, L = LONG, UL = ULONG, F = FLOAT, D = DOUBLE, V = VARIADIC;

// clang-format off
static NumericOperatorInfo oper_info[]=
{
	{tEQ,{EQ,EQ,EQl,EQl,EQf,EQd,EQv},{},{},pCmp},			//		TODO
};
// clang-format on

// ################################# CODE with RTYPE ########################################

struct ObjCode
{
	Type	rtype = VOID;
	char	_padding[3];
	uint	cnt	 = 0;
	uint	max	 = 0;
	uint16* code = nullptr;

	~ObjCode() { free(code); }
	ObjCode() = default;
	ObjCode(ObjCode&& q) : rtype(q.rtype), cnt(q.cnt), code(q.code) { q.code = nullptr; }
	ObjCode& operator=(ObjCode&& q)
	{
		rtype = q.rtype;
		cnt	  = q.cnt;
		std::swap(code, q.code);
		return *this;
	}
	NO_COPY(ObjCode);

	void grow(uint d)
	{
		if (cnt + d <= max) return;
		uint  newmax;
		void* p = realloc(code, (newmax = cnt + d + 512) * sizeof(uint16));
		if (!p) p = realloc(code, (newmax = cnt + d) * sizeof(uint16));
		if (!p) panic(OUT_OF_MEMORY);
		code = uint16ptr(p);
		max	 = newmax;
	}
	void append(const uint16* q, uint sz)
	{
		grow(sz);
		memcpy(code + cnt, q, sz * sizeof(uint16));
		cnt += sz;
	}
	template<typename T>
	void append(T v)
	{
		constexpr uint sz = sizeof(T) / sizeof(uint16);
		static_assert(sz);
		append(&v, sz);
	}
	void append(uint16 v)
	{
		grow(1);
		code[cnt++] = v;
	}
	void append(int16 v)
	{
		append(int16(v)); //
	}
	void append(Opcode v)
	{
		append(int16(v)); //
	}
	void append_opcode(uint16 opcode, Type t)
	{
		if (opcode == 666) throw "TODO";
		append(opcode);
		rtype = t;
	}
	void append_ival(int value, Type t = INT)
	{
		if (int16(value) == value)
		{
			grow(2);
			code[cnt++] = IVALi16;
			code[cnt++] = uint16(value);
		}
		else
		{
			grow(3);
			code[cnt++] = IVAL;
			code[cnt++] = uint16(value);
			code[cnt++] = uint16(value >> 16);
		}
		rtype = t;
	}
	void append_ival(Var value, Type t) { append_ival(value.i32, t); }
	void append_ival(float value) { append_ival(Var(value), FLOAT); }
	void append_ival(cstr string) { append_ival(Var(string), STRING); }
	void append_ival(int64) { TODO(); }

	void append(const uint16* q, uint cnt, Type t)
	{
		append(q, cnt);
		rtype = t;
	}
	void append(const ObjCode& q) { append(q.code, q.cnt); }
	void append_label_def(uint16 label) { append(uint16(label | 0x8000u)); }
	void append_label_ref(uint16 label) { append(uint16(label)); }

	void prepend(const ObjCode& q)
	{
		grow(q.cnt);
		memmove(code + q.cnt, code, cnt * sizeof(*code));
		memcpy(code, q.code, q.cnt * sizeof(*code));
		cnt += q.cnt;
	}
};

bool is_numeric(const ObjCode& v) { return is_numeric(v.rtype); }


// ################################# SYS VARS ########################################

/*
	memory map:
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


// ################################# COMPILER STATE ########################################

static uint16*				  rom;
static Var*					  ram;
static uint					  ram_size;	  // allocated size (in int32)
static uint					  rom_size;	  // allocated size (in uint16)
static uint					  gvars_size; // end of used gvars, start of gap
static uint					  code_size;  // end of used code, where to store new code
static HashMap<uint, Symbol*> dict;		  // note: uint = idf_id

void setup(uint romsize, uint ramsize)
{
	ram		   = new Var[ramsize];
	rom		   = new uint16[romsize];
	ram_size   = ramsize;
	rom_size   = romsize;
	gvars_size = 0;
	code_size  = 0;
}

static uint	  num_nested_loops	  = 0;
static uint	  num_nested_switches = 0;
static bool	  in_proc_def		  = false;
static uint16 label				  = 0;


// ################################# COMPILE ########################################

void expect(cstr& source, cstr s)
{
	if (test_word(source, s)) return;
	else throw usingstr("expected '%s'", s);
}

void deref(ObjCode& z) { TODO(); }
void cast_to(ObjCode& z, Type type) { TODO(); }
void cast_to_same(ObjCode& z, ObjCode& z2) { TODO(); }
void cast_to_bool(ObjCode& z) { TODO(); }

ObjCode value(cstr& source, uint prio)
{
	Var		var;
	ObjCode z;
a:
	IdfID idf = next_word(source, &var, false);

	switch (idf)
	{
		//case tNL: goto a;

	case t_LONG: TODO();
	case t_INT: z.append_ival(var.i32, INT); break;
	case t_CHAR: z.append_ival(var.i32, CHAR); break;
	case t_FLOAT: z.append_ival(var.f32); break;
	case t_STRING: z.append_ival(cstr(var.cptr)); break;

	case tRKauf:
		z = value(source, pAny);
		expect(source, ")");
		break;

	case tNOT:
	{
		z = value(source, pUna);
		deref(z);
		if (z.rtype == VOID) throw "not numeric";
		else if (z.rtype <= VARIADIC)
		{
			constexpr Opcode o[] = {NOT, NOT, NOT, NOT, NOT, NOTl, NOT, NOT, NOT, NOTl, NOTf, NOTd, NOTv};
			z.append_opcode(o[z.rtype - BOOL], BOOL);
		}
		else
		{
			assert(size_of(z.rtype) == sizeof(int));
			z.append_opcode(NOT, BOOL);
		}
		break;
	}
	case tCPL:
	{
		z = value(source, pUna);
		deref(z);
		Type ztype = z.rtype;
		if (ztype >= BOOL && ztype <= VARIADIC)
		{
			constexpr Opcode x	 = Opcode(0);
			constexpr Opcode o[] = {SUB1, CPL, CPL, CPL, CPL, CPLl, CPL, CPL, CPL, CPLl, x, x, CPLv};
			constexpr Type	 t[] = {UINT, UINT, UINT, UINT, UINT, ULONG, UINT, UINT, UINT, ULONG, VOID, VOID, VARIADIC};
			if (o[ztype - BOOL] == x) throw "not implemented";
			z.append_opcode(o[ztype - BOOL], t[ztype - BOOL]);
		}
		else throw "not numeric";
		{
			assert(size_of(z.rtype) == sizeof(int));
			z.append_opcode(NOT, BOOL);
		}
		break;
	}
	case tSUB:
	{
		z = value(source, pUna);
		deref(z);
		Type ztype = z.rtype;
		if (ztype >= BOOL && ztype <= VARIADIC)
		{
			constexpr Opcode x	 = Opcode(0);
			constexpr Opcode o[] = {SUB1, NEG, NEG, NEG, NEG, NEGl, NEG, NEG, NEG, NEGl, NEGf, NEGd, NEGv};
			constexpr Type	 t[] = {INT, INT, INT, INT, INT, LONG, INT, INT, INT, LONG, FLOAT, DOUBLE, VARIADIC};
			if (o[ztype - BOOL] == x) throw "not implemented";
			z.append_opcode(o[ztype - BOOL], t[ztype - BOOL]);
		}
		else throw "not numeric";
		{
			assert(size_of(z.rtype) == sizeof(int));
			z.append_opcode(NOT, BOOL);
		}
		break;
	}
	case tADD:
	{
		z = value(source, pUna);
		deref(z);
		if (z.rtype < BOOL || z.rtype > VARIADIC) throw "not numeric";
		break;
	}
	default:
	{
		Symbol* w = dict[idf];
		if (!w) throw usingstr("%s not found", names[idf]);

		if (ConstDef* c = w->as_const_def())
		{
			if (!is_numeric(w->rtype)) throw "TODO const is not numeric";
			if (w->rtype == FLOAT) z.append_ival(c->value.f32);
			else if (w->rtype == INT) z.append_ival(c->value.i32, INT);
			else if (w->rtype == CHAR) z.append_ival(c->value.i32, CHAR);
			else TODO();
			break;
		}
		if (GVarDef* gvar = w->as_gvar_def())
		{
			z.append_opcode(GVAR, gvar->rtype | VREF);
			z.append(gvar->offset);
			break;
		}
		if (Callable* fu = w->as_callable())
		{
			uint  argc = fu->argc;
			Type* argv = fu->argv;
			for (uint i = 0; i < argc; i++)
			{
				if (i) expect(source, ",");
				z = value(source, pAny);
				cast_to(z, argv[i]);
			}
			if (OpcodeDef* pd = w->as_opcode_def())
			{
				z.append_opcode(pd->opcode, pd->rtype);
				break;
			}
			if (InlineDef* id = w->as_inline_def())
			{
				z.append(id->code, id->count, id->rtype);
				break;
			}
			if (ProcDef* pd = w->as_proc_def())
			{
				// TODO: CALL opcode?
				// TODO: else need minimum address to distinguish from opcodes!
				z.append_opcode(pd->offset, pd->rtype);
				break;
			}
		}
		IERR();
		break;
	}

		// expect operator

	o:
		IdfID oper = next_word(source, var, true);

		switch (oper)
		{
		case tNL: return z;
		case tINCR:
		case tDECR:
		{
			constexpr Opcode oo[2][12] = {
				{INCRb, INCRb, INCRs, INCR, INCRl, INCRb, INCRs, INCR, INCRl, INCRf, INCRd, INCRv},
				{DECRb, DECRb, DECRs, DECR, DECRl, DECRb, DECRs, DECR, DECRl, DECRf, DECRd, DECRv}};

			if ((z.rtype & VREF) == 0) throw "vref required";
			Type ztype = z.rtype & ~VREF;
			if (ztype < CHAR || ztype > VARIADIC) throw "numeric type required";

			Opcode o = oo[oper - tINCR][ztype - CHAR];
			z.append_opcode(o, VOID);
			return z;
		}
		case tSL:
		case tSR:
		{
			constexpr Opcode oo[2][12] = {
				{SL, SL, SL, SL, SLl, SL, SL, SL, SLl, SLf, SLd, SLv},
				{SRu, SR, SR, SR, SRl, SRu, SRu, SRu, SRul, SRf, SRd, SRv}};

			if (prio >= pShift) return z;
			deref(z);
			if (z.rtype < CHAR || z.rtype > VARIADIC) throw "numeric type required";
			ObjCode z2 = value(source, pShift);
			cast_to(z2, INT);
			z.append(z2);

			Opcode o = oo[oper - tSL][z.rtype - CHAR];
			z.append_opcode(o, z.rtype);
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

			// note: CHAR INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			constexpr Opcode x		   = Opcode(0);
			constexpr Opcode oo[8][12] = {
				{ADD, ADD, ADD, ADD, ADDl, ADD, ADD, ADD, ADDl, ADDf, ADDd, ADDv},
				{SUB, SUB, SUB, SUB, SUBl, SUB, SUB, SUB, SUBl, SUBf, SUBd, SUBv},
				{MUL, MUL, MUL, MUL, MULl, MUL, MUL, MUL, MULl, MULf, MULd, MULv},
				{DIVu, DIV, DIV, DIV, DIVl, DIVu, DIVu, DIVu, DIVl, DIVf, DIVd, DIVv},
				{MODu, MOD, MOD, MOD, MODl, MODu, MODu, MODu, MODul, x, x, MODv},
				{AND, AND, AND, AND, ANDl, AND, AND, AND, ANDl, x, x, ANDv},
				{OR, OR, OR, OR, ORl, OR, OR, OR, ORl, x, x, ORv},
				{XOR, XOR, XOR, XOR, XORl, XOR, XOR, XOR, XORl, x, x, XORv},
			};

			if (prio >= opri[oper - tADD]) return z;
			deref(z);
			Type rtype = z.rtype;
			if (rtype < CHAR || rtype > VARIADIC) throw "numeric type required";
			Opcode o = oo[oper - tADD][z.rtype - CHAR];
			if (o == x) throw "unsupport data type for operation";

			ObjCode z2 = value(source, pShift);
			cast_to_same(z, z2);
			assert(z.rtype == rtype);
			z.append(z2);
			z.append_opcode(o, rtype);
			goto o;
		}
		case tEQ:
		case tNE:
		case tLT:
		case tGT:
		case tLE:
		case tGE:
		{
			constexpr Opcode oo[6][12] = {
				{EQ, EQ, EQ, EQ, EQl, EQ, EQ, EQ, EQl, EQf, EQd, EQv},
				{NE, NE, NE, NE, NEl, NE, NE, NE, NEl, NEf, NEd, NEv},
				{GE, GE, GE, GE, GEl, GE, GE, GE, GEl, GEf, GEd, GEv},
				{LE, LE, LE, LE, LEl, LE, LE, LE, LEl, LEf, LEd, LEv},
				{GT, GT, GT, GT, GTl, GT, GT, GT, GTl, GTf, GTd, GTv},
				{LT, LT, LT, LT, LTl, LT, LT, LT, LTl, LTf, LTd, LTv},
			};

			if (prio >= pCmp) return z;
			deref(z);
			ObjCode z2 = value(source, pCmp);
			cast_to_same(z, z2);
			z.append(z2);

			if (z.rtype < CHAR || z.rtype > VARIADIC) throw "numeric type required";
			Opcode o = oo[oper - tEQ][z.rtype - CHAR];
			z.append_opcode(o, BOOL);
			goto o;
		}
		case tANDAND:
		{
			if (prio >= pBoolean) return z;
			cast_to_bool(z);
			ObjCode z2 = value(source, pBoolean);
			cast_to_bool(z2);
			z.append_opcode(JZ, VOID);
			z.append_label_ref(label);
			z.append(z2);
			z.append_label_def(label++);
			goto o;
		}
		case tOROR:
		{
			if (prio >= pBoolean) return z;
			cast_to_bool(z);
			ObjCode z2 = value(source, pBoolean);
			cast_to_bool(z2);
			z.append_opcode(JNZ, VOID);
			z.append_label_ref(label);
			z.append(z2);
			z.append_label_def(label++);
			goto o;
		}
		case tQMARK:
		{
			if (prio >= pTriadic + 1) return z;
			cast_to_bool(z);
			ObjCode z2 = value(source, pTriadic);
			expect(source, ":");
			ObjCode z3 = value(source, pTriadic);
			cast_to_same(z2, z3);
			z.append(JZ);
			z.append_label_ref(label);
			z.append(z2);
			z.append(JR);
			z.append_label_ref(label + 1);
			z.append_label_def(label);
			z.append(z3);
			z.append_label_def(label + 1);
			label += 2;
			goto o;
		}
		case tGL:
		{
			//if (prio >= pAssign) return z;
			if ((z.rtype & VREF) == 0) throw "vref required";
			Type	ztype = z.rtype & ~VREF;
			uint	sz	  = size_of(ztype);
			ObjCode z2	  = value(source, pAssign);
			cast_to(z2, ztype);
			z2.append(z);

			switch (sz)
			{
			case 1: z2.append_opcode(POKE8, VOID); return z2;
			case 2: z2.append_opcode(POKE16, VOID); return z2;
			case 4: z2.append_opcode(POKE, VOID); return z2;
			case 8: //z2.append_opcode(POKEl, VOID); return z2;
			default: TODO();
			}
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
			constexpr Opcode x		 = Opcode(0);
			constexpr Opcode oo[][9] = {
				{ADDGLb, SUBGLb, x, x, ANDGLb, ORGLb, XORGLb, x, x},					// CHAR
				{ADDGLb, SUBGLb, x, x, ANDGLb, ORGLb, XORGLb, x, x},					// INT8
				{ADDGLs, SUBGLs, x, x, ANDGLs, ORGLs, XORGLs, x, x},					// INT16
				{ADDGL, SUBGL, MULGL, DIVGL, ANDGL, ORGL, XORGL, SLGL, SRGL},			// INT
				{ADDGL, SUBGLl, MULGLl, DIVGLl, ANDGLl, ORGLl, XORGLl, SLGLl, SRGLl},	// LONG
				{ADDGLb, SUBGLb, x, x, ANDGLb, ORGLb, XORGLb, x, x},					// UINT8
				{ADDGLs, SUBGLs, x, x, ANDGLs, ORGLs, XORGLs, x, x},					// UINT16
				{ADDGL, SUBGL, MULGL, DIVGLu, ANDGL, ORGL, XORGL, SLGL, SRGLu},			// UINT
				{ADDGL, SUBGLl, MULGLl, DIVGLlu, ANDGLl, ORGLl, XORGLl, SLGLl, SRGLlu}, // ULONG
				{ADDGLf, SUBGLf, MULGLf, DIVGLf, x, x, x, x, x},						// FLOAT
				{ADDGLd, SUBGLd, MULGLd, DIVGLd, x, x, x, x, x},						// DOUBLE
				{ADDGLv, SUBGLv, MULGLv, DIVGLv, x, x, x, x, x},						// VARIADIC
			};

			static_assert(oo[0][8] == x);
			static_assert(oo[UINT - CHAR][tDIVGL - tADDGL] == DIVGLu);
			static_assert(oo[VARIADIC - CHAR][tDIVGL - tADDGL] == DIVGLv);

			//if (prio >= pAssign) return z;
			if ((z.rtype & VREF) == 0) throw "vref required";
			Type ztype = z.rtype & ~VREF;
			if (ztype < CHAR || ztype > VARIADIC) throw "numeric type required";
			Opcode o = oo[ztype - CHAR][oper - tADDGL];
			if (o == x) throw "opcode not supported for data type";

			ObjCode z2 = value(source, pAssign);
			cast_to(z2, ztype);
			z2.append(z);
			z2.append_opcode(o, VOID);
			return z2;
		}
		case tEKauf:
		{
			if (num_dimensions(z.rtype) == 0) throw "array required";

			do {
				if (num_dimensions(z.rtype) == 0) throw "too many subscripts";
				ObjCode z2 = value(source, pAny);
				cast_to(z, UINT);
				z.prepend(z2);
				deref(z);
				Type   itype = z.rtype - ARRAY1;
				uint   ss	 = sizeshift_of(itype);
				Opcode ati[] = {ATI8, ATI16, ATI, ATI64};
				z.append_opcode(ati[ss], itype + VREF);
			}
			while (test_word(source, ","));

			expect(source, "]");
			goto o;
		}
		case tRKauf: // proc ( arguments ... )
		{
			// TODO: we need a data type for callables with all argument types and rtype!
			if ((0))
			{
				Symbol* w = dict[idf];
				if (!w) throw usingstr("%s not found", names[idf]);

				Callable* fu = w->as_callable();
				if (!fu) throw usingstr("%s is not callable", names[idf]);

				uint  argc = fu->argc;
				Type* argv = fu->argv;
				for (uint i = 0; i < argc; i++)
				{
					if (i) expect(source, ",");
					z = value(source, pAny);
					cast_to(z, argv[i]);
				}
				if (OpcodeDef* pd = w->as_opcode_def())
				{
					z.append_opcode(pd->opcode, pd->rtype);
					goto o;
				}
				if (InlineDef* id = w->as_inline_def())
				{
					z.append(id->code, id->count, id->rtype);
					goto o;
				}
				if (ProcDef* pd = w->as_proc_def())
				{
					// TODO: use a CALL opcode?
					// TODO: else need minimum address to distinguish from opcodes!
					z.append_opcode(pd->offset, pd->rtype);
					goto o;
				}
				IERR();
			}
			TODO(); //
		}
		default: break;
		}

		putback_word();
		return z;
	}
}

void compile_value(Type expected_type, uint prio) { TODO(); }

void compile_word(Word* word)
{
	// proc name => call proc
	// var name => assign var
	// typename => define proc or var

	switch ((word->type() & 0xF8))
	{
	case Word::PROC: TODO();
	case Word::GVAR: TODO();
	case Word::CONST: TODO();
	default: IERR();
	}
}

uint compile_instruction(cstr& source)
{
	// instruction:
	// proc name
	// var name
	// type name
	// flow_control

	Token w	 = next_word(source, false);
	uint  id = w.idf_id;
	switch (id)
	{
	case tEOF: return id;
	case t_LINE: return tCOLON;
	case tCOLON: return tCOLON;
	case tCONST: TODO(); // define const value
	case tENUM: TODO();	 // define const values by enumeration
	case tTYPE: TODO();	 // define new type (struct)
	case tINT: TODO();	 // define int var or proc
	case tSTR: TODO();	 // define str var or proc
	case tFLOAT: TODO(); // define float var or proc
	case tCHAR: TODO();	 // define char var or proc
	case tIF:
	{
		compile_value(INT);
		expect(":");
		compile_instruction();
		while (test_word("elif")) TODO();
		if (test_word("else")) TODO();
		TODO(); //store code
	}
	case tSWITCH:
	{
		compile_value(INT);
		expect(":");
		while (test_word("case")) TODO();
		if (test_word("default")) TODO();
		TODO(); //store code
	}
	case tBREAK:
	{
		if (num_nested_switches == 0) throw "not in switch";
		TODO();
	}
	case tGKauf:
	{
		while (!test_word("}")) compile_instruction();
		return tCOLON;
	}
	case tFOR: TODO();
	case tDO:
	{
		TODO();
	}
	case tWHILE:
	{
		if (num_nested_loops == 0) throw "not in loop";
		compile_value(INT);
		TODO();
	}
	case tUNTIL:
	{
		if (num_nested_loops == 0) throw "not in loop";
		compile_value(INT);
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
		if (rtype != VOID) compile_value(rtype);
		return tCOLON;
	}
	case t_IDF:
	{
		// proc name => call proc
		// var name => assign var
		// typename => define proc or var
		Word* word = find_word(w.name);
		if (word) compile_word(word);
		else throw "not found";
	}
	}
	throw "unexpected";
}


} // namespace kio::Vcc


/*








































*/
