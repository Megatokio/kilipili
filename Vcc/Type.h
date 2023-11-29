// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "basic_math.h"
#include "idf_ids.h"
#include "kilipili_cdefs.h"
#include "standard_types.h"

namespace kio::Vcc
{

enum SigID : uint16;


enum BaseType : uint {
	//							where:	top		lvar	gvar	array	struct
	VOID,	  // no value				x		-		-		-		-
	INT8,	  //						int		int		int		x		x
	INT16,	  //						int		int		int		x		x
	INT,	  // integer number			x		x		x		x		x
	LONG,	  // long integer number	x?		x		x		x		x		TODO
	UINT8,	  //						uint	uint	uint	x		x
	UINT16,	  //						uint	uint	uint	x		x
	UINT,	  // integer number			x		x		x		x		x
	ULONG,	  // long integer number	x?		x		x		x		x		TODO
	FLOAT,	  // floating point number	x		x		x		x		x
	DOUBLE,	  // floating point number	x?		x		x		x		x		TODO
	VARIADIC, // variable with type		x?		x		x		x		x		TODO
	STRING,	  // text string			x		x		x		x		x		ptr
	STRUCT,	  // structured variable	x		x		x		x		x		ptr, info in msb
	PROC,	  // procedure				x		x		x		x		x		ptr, info in msb

	// VREF	  //						x		-		-		-		-
	// ARRAY  //						x		x		x		x		x		ptr
	//
	// BOOL = UINT8 + ENUM + 0, //		uint	uint	uint	x		x
	// CHAR = UINT8 + ENUM + 1, //		uint	uint	uint	x		x		latin-1
};

union Type
{
	uint32 all;
	struct
	{
		BaseType basetype : 8;
		bool	 is_enum  : 1  = false;
		bool	 is_vref  : 1  = false;
		uint	 dims	  : 6  = 0;
		uint	 info	  : 16 = 0; // for enum, struct and proc
	};

	constexpr Type() : all(0) {}
	constexpr explicit Type(uint32 all) : all(all) {}
	constexpr Type(BaseType t) : basetype(t) {}
	constexpr Type(BaseType t, bool e, bool v, uint dims, uint info) :
		basetype(t),
		is_enum(e),
		is_vref(v),
		dims(dims),
		info(info)
	{}

	constexpr operator uint() { return uint(basetype); }

	static constexpr uint8 _size_of[]	  = {0, 1, 2, 4, 8, 1, 2, 4, 8, 4, 8, 8};
	static constexpr uint8 _size_on_top[] = {4, 4, 4, 4, 8, 4, 4, 4, 8, 4, 8, 8};
	static constexpr uint8 ss[]			  = {0, 0, 1, 2, 3, 0, 1, 2, 3, 2, 3, 3}; // not for void

	constexpr Type arithmetic_result_type() const noexcept;

	constexpr bool operator==(Type t) const noexcept { return all == t.all; }
	constexpr bool operator==(BaseType t) const noexcept { return all == t; }

	constexpr bool is_numeric() const noexcept
	{
		return basetype != VOID && basetype <= VARIADIC && dims == 0 && !is_vref;
	}
	constexpr uint size_of() const noexcept
	{
		return dims || is_vref || (basetype >= STRING) ? sizeof(ptr) : _size_of[basetype];
	}
	constexpr uint ss_of() const noexcept
	{
		return dims || is_vref || (basetype >= STRING) ? msbit(sizeof(ptr)) : ss[basetype];
	}
	constexpr uint size_on_top() const noexcept { return max(size_of(), 4u); }
	constexpr bool is_unsigned_int() { return basetype >= UINT8 && basetype <= ULONG; }
	constexpr bool is_signed_int() { return basetype >= INT8 && basetype <= LONG; }
	constexpr bool is_array() { return dims; }
	constexpr bool is_callable() const noexcept { return basetype == PROC && !is_vref && dims == 0; }
	constexpr bool is_integer() const noexcept
	{
		return (basetype >= INT8 && basetype <= ULONG) || basetype == VARIADIC;
	}

	static constexpr Type make_proc(SigID sid) noexcept { return Type(PROC, 0, 0, 0, sid); }
	static constexpr Type make_enum(IdfID enum_name, BaseType bt) { return Type(bt, 1, 0, 0, enum_name); }

	constexpr Type add_vref() { return Type(all | 0x200); }
	constexpr Type strip_enum() { return is_enum ? Type(all & 0xfeff) : *this; }
	constexpr Type strip_vref() { return Type(all & ~0x200u); }
	constexpr Type strip_dim()
	{
		assert(dims);
		return Type(all - 0x400);
	}
};


inline constexpr Type Type::arithmetic_result_type() const noexcept
{
	constexpr Type _ari_result[] = {VOID, INT, INT, INT, LONG, UINT, UINT, UINT, ULONG, FLOAT, DOUBLE, VARIADIC};
	assert(basetype < NELEM(_ari_result));
	return _ari_result[basetype];
}

} // namespace kio::Vcc
