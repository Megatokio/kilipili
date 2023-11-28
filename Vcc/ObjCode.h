// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Opcodes.h"
#include "Type.h"
#include "Var.h"
#include "VxRunner.h"
#include "string.h"

namespace kio::Vcc
{


struct ObjCode
{
	Type	  rtype = VOID;
	uint	  cnt	= 0;
	uint	  max	= 0;
	VxOpcode* code	= nullptr;

	~ObjCode() { free(code); }
	ObjCode() = default;
	ObjCode(ObjCode&& q) : rtype(q.rtype), cnt(q.cnt), code(q.code) { q.code = nullptr; }
	ObjCode& operator=(ObjCode&& q)
	{
		rtype = q.rtype;
		cnt	  = q.cnt;
		std::swap(max, q.max);
		std::swap(code, q.code);
		return *this;
	}
	ObjCode(const ObjCode& q)			 = delete;
	ObjCode& operator=(const ObjCode& q) = delete;

	void grow(uint d)
	{
		if (cnt + d <= max) return;
		uint  newmax;
		void* p = realloc(code, (newmax = cnt + d + 512) * sizeof(*code));
		if (!p) p = realloc(code, (newmax = cnt + d) * sizeof(*code));
		if (!p) panic(OUT_OF_MEMORY);
		code = VxOpcodePtr(p);
		max	 = newmax;
	}
	void append(const int* objcode, uint count)
	{
		assert(sizeof(*objcode) == sizeof(*code));
		grow(count);
		memcpy(code + cnt, objcode, count * sizeof(*code));
		cnt += count;
	}
	template<typename T>
	void append(T value)
	{
		static_assert(sizeof(T) % sizeof(*code) == 0);
		constexpr uint sz = sizeof(T) / sizeof(*code);
		append(&value, sz);
	}
	void append(VxOpcode opcode) // actual opcode
	{
		grow(1);
		code[cnt++] = opcode;
	}
	void append(Opcode opcode) // opcode enumeration
	{
		append(vx_opcodes[opcode]);
	}
	void append(int value)
	{
		assert(sizeof(value) == sizeof(*code));
		append(VxOpcode(value));
	}
	void append(uint value)
	{
		assert(sizeof(value) == sizeof(*code));
		append(VxOpcode(value));
	}
	void append_opcode(Opcode opcode, Type t)
	{
		append(opcode);
		rtype = t;
	}
	void append_ival(int value, Type t = INT)
	{
		assert(sizeof(value) == sizeof(*code));
		grow(2);
		code[cnt++] = vx_opcodes[PUSH_IVAL];
		code[cnt++] = VxOpcode(value);
		rtype		= t;
	}
	void append_ival(Var value, Type t) { append_ival(value.i32, t); }
	void append_ival(float value) { append_ival(Var(value), FLOAT); }
	void append_ival(cstr string) { append_ival(Var(string), STRING); }
	void append_ival(int64) { throw "TODO: append_ival64"; }

	void append(const int* q, uint cnt, Type t)
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

	bool is_ival()
	{
		if (rtype == VOID) return false;
		if (cnt == 2 && code[0] == vx_opcodes[IVAL]) return true;
		if (cnt == 2 && code[0] == vx_opcodes[PUSH_IVAL]) return true;
		return false;
	}

	int value()
	{
		assert(is_ival());
		switch (code[0])
		{
		case PUSH_IVALi16:
		case IVALi16: return code[1];
		case PUSH_IVAL:
		case IVAL: return code[1] + code[2] * 0x10000;
		default: return false;
		}
	}

	void deref()
	{
		if (rtype.is_vref)
		{
			// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

			constexpr Opcode oo[] = {NOP,	  PEEKi8, PEEKi16, PEEK, PEEKl, PEEKu8,
									 PEEKu16, PEEK,	  PEEKl,   PEEK, PEEKl, PEEKv};

			uint   sz	= rtype.size_of();
			Opcode peek = sz == sizeof(int) ? PEEK : oo[rtype];
			if (peek == NOP) throw "todo: deref for data type";
			append_opcode(peek, rtype.strip_vref());
		}
	}

	void cast_to_bool()
	{
		// for arithmetic types this is obvious,
		// for larger size in TOP this TODO
		// string, struct, proc (or other ptr types) this tests the pointer for null

		// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

		constexpr Opcode oo[] = {NOP,	  ITObool, ITObool, ITObool, LTObool, ITObool,
								 ITObool, ITObool, LTObool, FTObool, DTObool, VTOB};

		uint   sz	   = rtype.size_of();
		Opcode to_bool = sz == sizeof(int) ? ITObool : oo[rtype];
		if (to_bool == NOP) throw "todo: to_bool for data type";
		append_opcode(to_bool, Type::make_enum(tBOOL, UINT8));
	}

	bool can_cast_without_conversion(Type ztype)
	{
		// can cast without loss of data
		// e.g. &T1 -> &T2 not allowed

		ztype.strip_enum();
		Type qtype = rtype.strip_enum();

		if (qtype == rtype) return true;

		if (qtype != qtype.basetype) return false; // is_array, is_vref
		if (ztype != ztype.basetype) return false; // is_array, is_vref

		if (!ztype.is_integer()) return false; // always needs conversion
		if (!qtype.is_integer()) return false; // always needs conversion

		uint zsz = ztype.size_of();
		uint qsz = qtype.size_of();
		if (ztype < qtype) return false;					   // needs limiting
		if (zsz >= 4 || qsz >= 4) return zsz == qsz;		   // allow 4->4 and 8->8 mixed signedness
		return ztype.is_signed_int() == qtype.is_signed_int(); // i8->i16->i32 or u8->u16->u32
	}

	void cast_to(Type ztype, bool explicit_cast = false)
	{
		// most casts are either nop or only explicit.

		if (ztype.is_vref)
		{
			if (ztype == rtype) return;
			if (!rtype.is_vref) throw "variable required";
			throw "wrong data type"; // never cast destination of an assignment!
		}
		else if (rtype.is_vref) deref();

		if (ztype.is_enum)
		{
			if (ztype != rtype)
			{
				// source is no enum or different enum or any of both is not scalar
				if (!explicit_cast) throw "wrong data type";
				if (!can_cast_without_conversion(ztype.basetype)) throw "incompatible base types";
				rtype = Type {ztype.basetype, 1, ztype.is_vref, ztype.dims, ztype.info};
			}
		}
		else rtype.strip_enum();

		if (ztype == rtype) return;

		else if (ztype.is_array() || rtype.is_array())
		{
			if (ztype.dims != rtype.dims) throw "wrong number of dimensions";
			/*if (ztype != rtype)*/ throw "wrong data type";
		}

		else if (rtype == VARIADIC)
		{
			if constexpr (VTOX == NOP) throw "todo: cast <-> variadic!";
			append(VTOX);
		}
		else if (ztype == VARIADIC)
		{
			if constexpr (VTOX == NOP) throw "todo: cast <-> variadic!";
			append(XTOV);
		}

		else if (!rtype.is_numeric() || !ztype.is_numeric())
		{
			// TODO: struct: implicit cast struct to base type
			// TODO: string: explicit
			// proc:   never cast to anything
			throw "wrong data type";
		}

		else if (ztype.basetype >= FLOAT && rtype.is_integer())
		{
			Opcode oo[2][2][2] = {{{UTOF, ULTOF}, {ITOF, LTOF}}, {{UTOD, ULTOD}, {ITOD, LTOD}}};
			Opcode o		   = oo[ztype == DOUBLE][rtype.is_signed_int()][rtype.size_of() == 8];

			if (o == NOP) throw "todo: cast long or double";
			append(ITOL);
		}

		else if (rtype.basetype >= FLOAT && ztype.is_integer())
		{
			if (!explicit_cast) throw "wrong data type";

			Opcode oo[2][2][2] = {{{FTOU, FTOUL}, {FTOI, FTOL}}, {{DTOU, DTOUL}, {DTOI, DTOL}}};
			Opcode o		   = oo[ztype == DOUBLE][rtype.is_signed_int()][rtype.size_of() == 8];

			if (o == NOP) throw "todo: cast long or double";
			append(ITOL);
		}

		// numeric <-> numeric

		else if (rtype.is_signed_int())
		{
			// int -> uint:        not allowed
			// int -> smaller int: not allowed
			// int -> larger int:  allowed.		note: i8->i16->i32 is a nop

			if (ztype.is_unsigned_int()) // signed -> unsigned
			{
				if (!explicit_cast) throw "wrong data type";
				if (ztype != UINT) append(ztype == UINT8 ? ITOu8 : ztype == UINT16 ? ITOu16 : ITOL);
			}
			else if (ztype < rtype) // signed -> smaller
			{
				if (!explicit_cast) throw "wrong data type";
				if (rtype.basetype <= INT) append(ztype == INT8 ? ITOu8 : ITOu16);
				else append(ztype == INT8 ? LTOu8 : ztype == INT16 ? LTOi16 : LTOI);
			}
			else if (ztype.basetype == LONG)
			{
				if constexpr (ITOL == NOP) throw "todo: cast to long";
				append(ITOL);
			}
		}
		else // rtype is unsigned
		{
			// uint -> smaller uint:  not allowed
			// uint -> same size int: normally disallowed, but allow uint->int and ulong->long
			// uint -> larger uint:   allowed.		note: u8->u16->u32 is a nop
			// uint -> larger int:    allowed.		note: u8->i16 and u16->i32 is a nop

			uint zss = Type::ss[ztype]; // log2(size) => 0=int8, 1=in16, 2=int, 3=long
			uint qss = Type::ss[rtype];

			static constexpr Opcode oo[2][2]  = {{ITOi8, ITOi16}, {ITOu8, ITOu16}};
			static constexpr Opcode ooo[2][3] = {{LTOi8, LTOi16, LTOI}, {LTOu8, LTOu16, LTOI}};

			if (zss < qss) // unsigned -> smaller int or uint
			{
				if (!explicit_cast) throw "wrong data type";
				if (qss <= 2) append(oo[ztype.is_signed_int()][zss]);
				else append(ooo[ztype.is_signed_int()][zss]);
			}
			else if (zss == qss) // unsigned -> signed
			{
				// uint->int and ulong->long is allowed and is a nop
				if (zss < 2)
				{
					if (!explicit_cast) throw "wrong data type";
					append(oo[ztype.is_signed_int()][zss]);
				}
			}
			else if (zss == 3)
			{
				if constexpr (UTOL == NOP) throw "todo: cast to long";
				append(UTOL);
			}
		}

		rtype = ztype;
	}

	void cast_to_same(ObjCode& other) { TODO(); }

	uint16 operator[](uint i) const { return code[i]; }
};


} // namespace kio::Vcc

/*








































*/
