// Copyright (c) 2010 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#pragma once
#include "idf_id.h"


inline bool isaIdf(IdfID t) { return t >= tGL; }  // tokens; not "", IVAL or MACROARG
inline bool isaName(IdfID t) { return t >= tIF; } // idfs conforming to general naming conventions
inline bool isaOperatorName(IdfID t)
{
	return t >= tGL && t <= tCPL;
} // idfs not conforming to general naming conventions
inline bool isaReservedName(IdfID t) { return t >= tIF && t <= tNEXT; }
inline bool isaStarter(IdfID t) { return t >= tIF && t <= tENUM; }

inline bool isaOperator(IdfID t) { return t >= tGL && t <= tDECR; } // operators with left-side argument
inline bool isaBinaryOperator(IdfID t) { return t >= tGL && t <= tSL && t != tQMARK && t != tCOLON; }
//inline bool isaNonvoidOperator(IdfID t)	{ return t>=tQMARK && t<=tDECR; }
//inline bool isaAssignmentOperator(IdfID t){ return t>=tGL && t<=tORORGL; }
//inline bool isaPruningOperator(IdfID t)	{ return t>=tQMARK && t<=tOROR; }
inline bool isaOpener(IdfID t) { return t >= tRKauf && t <= tEKauf; }
