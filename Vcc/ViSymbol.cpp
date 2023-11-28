// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "ViSymbol.h"


namespace kio::Vcc
{

ViSymbol* ViSymbol::deref()
{
	if (!rtype.is_vref) return this;

	// note: VOID INT8 INT16 INT LONG UINT8 UINT16 UINT ULONG FLOAT DBL VAR

	static constexpr Opcode oo[] = {NOP,	 PEEKi8, PEEKi16, PEEK, PEEKl, PEEKu8,
									PEEKu16, PEEK,	 PEEKl,	  PEEK, PEEKl, PEEKv};

	Type   rtype = this->rtype.strip_vref();
	uint   sz	 = rtype.size_of();
	Opcode o	 = sz == sizeof(int) ? PEEK : oo[rtype];
	if (o == NOP) throw "todo: deref for data type";

	return newViSymbolOpcode(o, rtype.strip_vref(), this);
}


} // namespace kio::Vcc


/*





















*/
