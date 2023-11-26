// Copyright (c) 2020 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#if defined M || !defined OPCODES_H

  #ifndef VCC_LONG
	#define VCC_LONG false
  #endif
  #ifndef VCC_VARIADIC
	#define VCC_VARIADIC false
  #endif

  #ifndef M
	#define OPCODES_H
	#define M_wasnt_defined
	#define M(MNEMONIC, NAME, ARGS) MNEMONIC
	#include "standard_types.h"

namespace kio::Vcc
{

constexpr bool vcc_long		= VCC_LONG;
constexpr bool vcc_variadic = VCC_VARIADIC;

enum Opcode : uint16 {
  #endif

	// clang-format off
	
	M(NOP,			"NOP",			NOARG),
	M(PUSH,			"PUSH",			NOARG),
	M(POP,			"POP",			NOARG),

	M(PUSH0,		"PUSH_0",		NOARG),
	M(PUSH0x2,		"PUSH_2x0", 	NOARG),
	M(PUSH0x3,		"PUSH_3x0", 	NOARG),
	M(PUSH0x4,		"PUSH_4x0", 	NOARG),

	M(IVAL,			"IVAL", 		ARGi16),	// TOP = IVAL
	M(IVALi16,		"IVALs",		ARGi16),
	M(PUSH_IVAL,	"PUSH IVAL",	ARGi16),
	M(PUSH_IVALi16,	"PUSH IVALs",	ARGi16),
	
	M(GVAR,			"GVAR",			ARGu16),	// TOP = GVAR
	M(GGET,			"GGET",			ARGu16),
	M(GSET,			"GSET",			ARGu16),
	M(PUSH_GVAR,	"PUSH GVAR",	ARGu16),
	M(PUSH_GGET,	"PUSH GGET",	ARGu16),
	
	M(LVAR,			"LVAR",			ARGu16),	// TOP = LVAR
	M(LGET,			"LGET",			ARGu16),
	M(LSET,			"LSET",			ARGu16),
	M(PUSH_LVAR,	"PUSH LVAR",	ARGu16),
	M(PUSH_LGET,	"PUSH LGET",	ARGu16),

	M(IVAR,			"IVAR",			ARGu16),
	M(IVAR8,		"IVAR8",		ARGu16),
	M(IVAR16,		"IVAR16",		ARGu16),
	M(IGET,			"IGET",			ARGu16),
	M(IGETi8,		"IGET8",		ARGu16),
	M(IGETi16,		"IGET16",		ARGu16),
	M(IGETu8,		"IGETu8",		ARGu16),
	M(IGETu16,		"IGETu16",		ARGu16),
	M(ISET,			"ISET",			ARGu16),
	M(ISET8,		"ISET8",		ARGu16),
	M(ISET16,		"ISET16",		ARGu16),

	M(ATI,			"ATI",			NOARG),
	M(ATI8,			"ATI8",			NOARG),
	M(ATI16,		"ATI16",		NOARG),
	M(ATIGET,		"ATIGET",		NOARG),
	M(ATIGETu8,		"ATIGETu8",		NOARG),
	M(ATIGETu16,	"ATIGETu16",	NOARG),
	M(ATIGETi8,		"ATIGET8",		NOARG),
	M(ATIGETi16,	"ATIGET16",		NOARG),
	M(ATISET,		"ATISET",		NOARG),
	M(ATISET8,		"ATISET8",		NOARG),
	M(ATISET16,		"ATISET16",		NOARG),

	M(PEEK,			"PEEK",			NOARG),
	M(PEEKi8,		"PEEK8",		NOARG),
	M(PEEKi16,		"PEEK16",		NOARG),
	M(PEEKu8,		"PEEKU8",		NOARG),
	M(PEEKu16,		"PEEKU16",		NOARG),
	M(POKE,			"POKE",			NOARG),
	M(POKE8,		"POKE8",		NOARG),
	M(POKE16,		"POKE16",		NOARG),

	M(ADD,			"+",			NOARG),
	M(SUB,			"-",			NOARG),
	M(MUL,			"*",			NOARG),
	M(DIV,			"/",			NOARG),
	M(DIVu,			"/u",			NOARG),
	M(MOD,			"%",			NOARG),
	M(MODu,			"%u",			NOARG),
	M(AND,			"&",			NOARG),
	M(OR,			"|",			NOARG),
	M(XOR,			"^",			NOARG),
	M(SL,			"<<",			NOARG),
	M(SR,			">>",			NOARG),
	M(SRu,			"u>>",			NOARG),

	M(ADDI,			"addi",			ARGi16),
	//M(SUBI,		"",				ARGi16), --> ADDI
	M(MULI,			"muli",			ARGi16),
	M(DIVI,			"divi",			ARGi16),
	M(DIVIu,		"diviu",		ARGi16),
	M(ANDI,			"andi",			ARGi16),
	M(ORI,			"ori",			ARGi16),
	M(XORI,			"xori",			ARGi16),
	M(SLI,			"sli",			ARGi16),
	M(SRI,			"sri",			ARGi16),
	M(SRIu,			"sriu",			ARGi16),

	M(ADD1,			"1 -",			NOARG),
	M(ADD2,			"2 -",			NOARG),
	M(SUB1,			"1 -",			NOARG),
	M(SUB2,			"2 -",			NOARG),

	M(SL1,			"1 <<",			NOARG),
	M(SL2,			"2 <<",			NOARG),
	M(SR1,			"1 >>",			NOARG),
	M(SR2,			"2 >>",			NOARG),
	M(SR1u,			"1 u>>",		NOARG),
	M(SR2u,			"2 u>>",		NOARG),

	M(NOT,			"!",			NOARG),
	M(CPL,			"~",			NOARG),
	M(NEG,			"NEG",			NOARG),
	M(ABS,			"ABS",			NOARG),
	M(SIGN,			"SIGN",			NOARG),

	M(MIN,			"MIN",			NOARG),
	M(MINu,			"MINu",			NOARG),
	M(MAX,			"MAX",			NOARG),
	M(MAXu,			"MAXu",			NOARG),
	M(RANDOMu,		"RANDOMu",		NOARG),	// ( void -- uint )
	//M(NOW,		"NOW",			NOARG),

	M(EQ,			"==",			ARGi16),
	M(NE,			"!=",			ARGi16),
	M(LT,			"<",			ARGi16),
	M(LE,			"<=",			ARGi16),
	M(GT,			">",			ARGi16),
	M(GE,			">=",			ARGi16),
	M(LTu,			"LTu",			ARGi16),
	M(LEu,			"LEu",			ARGi16),
	M(GTu,			"GTu",			ARGi16),
	M(GEu,			"GEu",			ARGi16),

	M(ADDGL,		"+0",			NOARG),
	M(SUBGL,		"-=",			NOARG),
	M(MULGL,		"*=",			NOARG),
	M(DIVGL,		"/=",			NOARG),
	M(DIVGLu,		"/=u",			NOARG),
	M(ANDGL,		"&=",			NOARG),
	M(ORGL,			"|=",			NOARG),
	M(XORGL,		"^=",			NOARG),
	M(SLGL,			"<<=",			NOARG),
	M(SRGL,			">>=",			NOARG),
	M(SRGLu,		">>=u",			NOARG),
	M(INCR,			"++",			NOARG),
	M(DECR,			"--",			NOARG),

	M(ADDGLs,		"+=s",			NOARG),
	M(SUBGLs,		"-=s",			NOARG),
	M(ANDGLs,		"&=s",			NOARG),
	M(ORGLs,		"|=s",			NOARG),
	M(XORGLs,		"^=s",			NOARG),
	M(INCRs,		"++s",			NOARG),
	M(DECRs,		"--s",			NOARG),

	M(ADDGLb,		"+=b",			NOARG),
	M(SUBGLb,		"-=b",			NOARG),
	M(ANDGLb,		"&=b",			NOARG),
	M(ORGLb,		"|=b",			NOARG),
	M(XORGLb,		"^=b",			NOARG),
	M(INCRb,		"++b",			NOARG),
	M(DECRb,		"--b",			NOARG),
	
	M(ADDf,			"ADDf",			NOARG),
	M(SUBf,			"SUBf",			NOARG),
	M(MULf,			"MULf",			NOARG),
	M(DIVf,			"DIVf",			NOARG),
	M(SLf,			"SLf",			NOARG),
	M(SRf,			"SRf",			NOARG),
	M(ADD1f,		"ADD1f",		NOARG),
	M(SUB1f,		"SUB1f",		NOARG),
	M(NOTf,			"NOTf",			NOARG),
	M(NEGf,			"NEGf",			NOARG),
	M(ABSf,			"ABSf",			NOARG),
	M(SIGNf,		"SIGNf",		NOARG),

	M(SIN,			"sin",			NOARG),
	M(COS,			"cos",			NOARG),
	M(TAN,			"tan",			NOARG),
	M(ASIN,			"asin",			NOARG),
	M(ACOS,			"acos",			NOARG),
	M(ATAN,			"atan",			NOARG),
	M(SINH,			"sinh",			NOARG),
	M(COSH,			"cosh",			NOARG),
	M(TANH,			"tanh",			NOARG),
	M(ASINH,		"asinh",		NOARG),
	M(ACOSH,		"acosh",		NOARG),
	M(ATANH,		"atanh",		NOARG),
	M(LOG2,			"log2",			NOARG),
	M(LOGE,			"loge",			NOARG),
	M(LOG10,		"log10",		NOARG),
	M(LOG,			"log",			NOARG),
	M(EXP2,			"exp2",			NOARG),
	M(EXPE,			"expe",			NOARG),
	M(EXP10,		"exp10",		NOARG),
	M(EXP,			"exp",			NOARG),
	M(SQRT,			"sqrt",			NOARG),
	M(FLOOR,		"floor",		NOARG),
	M(CEIL,			"ceil",			NOARG),
	M(ROUND,		"round",		NOARG),
	M(INTEG,		"integ",		NOARG),
	M(FRACT,		"fract",		NOARG),
	M(EXPONENT,		"exponent",		NOARG),
	M(MANTISSA,		"mantissa",		NOARG),
	M(COMPOSE,		"compose",		NOARG),
	M(MINf,			"minf",			NOARG),
	M(MAXf,			"maxf",			NOARG),
	M(RANDOMf1,		"randomf1",		NOARG),
	M(RANDOMf,		"randomf",		NOARG),
	//M(NOWf,		"nowf",			NOARG),

	M(EQf,			"EQf",			NOARG),
	M(NEf,			"NEf",			NOARG),
	M(LTf,			"LTf",			NOARG),
	M(LEf,			"LEf",			NOARG),
	M(GTf,			"GTf",			NOARG),
	M(GEf,			"GEf",			NOARG),

	M(ADDGLf,		"+=f",			NOARG),
	M(SUBGLf,		"-=f",			NOARG),
	M(MULGLf,		"*=f",			NOARG),
	M(DIVGLf,		"/=f",			NOARG),
	M(INCRf,		"++f",			NOARG),
	M(DECRf,		"--f",			NOARG),

	M(JZ,			"JZ",			DISTi16),
	M(JNZ,			"JNZ",			DISTi16),
	M(JEQ,			"JEQ",			DISTi16),
	M(JNE,			"JNE",			DISTi16),
	M(JLT,			"JLT",			DISTi16),
	M(JLE,			"JLE",			DISTi16),
	M(JGT,			"JGT",			DISTi16),
	M(JGE,			"JGE",			DISTi16),
	M(JLTu,			"JLTu",			DISTi16),
	M(JLEu,			"JLEu",			DISTi16),
	M(JGTu,			"JGTu",			DISTi16),
	M(JGEu,			"JGEu",			DISTi16),

	M(JEQI,			"JEQI",			ARGi16_DISTi16), // 2 arguments: N and ±dis
	M(JNEI,			"JNEI",			ARGi16_DISTi16),
	M(JLTI,			"JLTI",			ARGi16_DISTi16),
	M(JLTIu,		"JLTIu",		ARGi16_DISTi16),
	M(JLEI,			"JLEI",			ARGi16_DISTi16),
	M(JLEIu,		"JLEIu",		ARGi16_DISTi16),
	M(JGEI,			"JGEI",			ARGi16_DISTi16),
	M(JGEIu,		"JGEIu",		ARGi16_DISTi16),
	M(JGTI,			"JGTI",			ARGi16_DISTi16),
	M(JGTIu,		"JGTIu",		ARGi16_DISTi16),

	M(JZf,			"JZf",			DISTi16),
	M(JNZf,			"JNZf",			DISTi16),
	M(JEQf,			"JEQf",			DISTi16),
	M(JNEf,			"JNEf",			DISTi16),
	M(JLTf,			"JLTf",			DISTi16),
	M(JLEf,			"JLEf",			DISTi16),
	M(JGEf,			"JGEf",			DISTi16),
	M(JGTf,			"JGTf",			DISTi16),

	M(JR,			"JR",			DISTi16),
	M(JP,			"JP",			DESTu32),
	M(JSR,			"JSR",			DESTu32),
	M(CALL,			"CALL",			NOARG),
	M(RET,			"RET",			NOARG),
	M(SWITCH,		"SWITCH",		NOARG),
	
	M(TRY,			"try",			ARGi16),
	M(THROW,		"throw",		NOARG),
	M(TRYEND,		"tryend",		NOARG),
	M(CATCH,		"catch",		NOARG),

	M(DROP,			"DROP",			NOARG),
	M(DROP2,		"DROP2",		NOARG),
	M(DROP3,		"DROP3",		NOARG),
	M(DROPN,		"DROPN",		NOARG),

	M(DROP_RET,		"DROP RET", 	NOARG),
	M(DROP2_RET,	"DROP2 RET",	NOARG),
	M(DROP3_RET,	"DROP3 RET",	NOARG),
	M(DROPN_RET,	"DROPN RET",	NOARG),
	
	M(ITOi8,		"ITOi8",		NOARG),
	M(ITOi16,		"ITOi16",		NOARG),
	M(ITOu8,		"UTOu8",		NOARG),
	M(ITOu16,		"UTOu16",		NOARG),
	M(ITOF,			"ITOF",			NOARG),
	M(UTOF,			"UTOF",			NOARG),
	M(FTOI,			"FTOI",			NOARG),
	M(FTOU,			"FTOU",			NOARG),
	M(ITObool,		"ITObool",		NOARG),
	M(FTObool,		"FTObool",		NOARG),

	M(_filler1,"",0),
	M(_filler2,"",0),
	M(_filler3,"",0),
	M(_filler4,"",0),
	M(_filler5,"",0),
	M(_filler6,"",0),
	M(_filler7,"",0),
	M(_filler8,"",0),
	M(_filler9,"",0),
	
	M(EXIT,			"EXIT",			NOARG),


// ===========================================================================
//			memory access if 8 byte data types are supported	
// ===========================================================================
	
#if VCC_LONG || VCC_VARIADIC 
	
	M(ATIl,			"ATIl",			NOARG),
	M(ATIGETl,		"ATIGETl",		NOARG),
	M(ATISETl,		"ATISETl",		NOARG),
	M(ISETl,		"ISETl",		NOARG),
	M(IGETl,		"IGETl",		NOARG),
	M(PEEKl,		"PEEKl",		NOARG),
	M(POKEl,		"POKEl",		NOARG),

	M(GGETl,		"GGETl",		ARGu16),
	M(LGETl,		"LGETl",		ARGu16),
	M(PUSH_GGETl,	"PUSH GGETl",	ARGu16),
	M(PUSH_GSETl,	"PUSH GSETl",	ARGu16),
	M(PUSH_LGETl,	"PUSH LGET",	ARGu16),
	M(PUSH_LSETl,	"PUSH LSET",	ARGu16),
	M(PUSHl_IVAL,	"PUSHl IVAL",	ARGi16),
	M(PUSHl_IVALs,	"PUSHl IVALs",	ARGi16),
	M(PUSHl_GVAR,	"PUSHl GVAR",	ARGu16),
	M(PUSHl_GGET,	"PUSHl GGET",	ARGu16),
	M(PUSHl_LVAR,	"PUSHl LVAR",	ARGu16),
	M(PUSHl_LGET,	"PUSHl LGET",	ARGu16),	
	M(PUSHl_GGETl,	"PUSHl GGETl",	ARGu16),
	M(PUSHl_GSETl,	"PUSHl GSETl",	ARGu16),
	M(PUSHl_LGETl,	"PUSHl LGET",	ARGu16),
	M(PUSHl_LSETl,	"PUSHl LSET",	ARGu16),
	
#endif 
		

// ===========================================================================
//			LONG, ULONG and DOUBLE opcodes
// ===========================================================================

#if VCC_LONG 

	M(EQl,			"EQl",			NOARG),
	M(NEl,			"NEl",			NOARG),
	M(GTl,			"GTl",			NOARG),
	M(LTl,			"LTl",			NOARG),
	M(GEl,			"GEl",			NOARG),
	M(LEl,			"LEl",			NOARG),
	M(LEul,			"LEul",			NOARG),
	M(GEul,			"GEul",			NOARG),
	M(LTul,			"LTul",			NOARG),	
	M(GTul,			"GTul",			NOARG),
	
	M(SLl,			"SLl",			NOARG),
	M(SRl,			"SRl",			NOARG),
	M(SRul,			"SRul",			NOARG),
	M(ADDl,			"ADDl",			NOARG),
	M(SUBl,			"SUBl",			NOARG),
	M(MULl,			"MULl",			NOARG),
	M(DIVl,			"DIVl",			NOARG),
	M(DIVul,		"DIVul", 		NOARG),
	M(MODl,			"MODl",			NOARG),
	M(MODul,		"MODul", 		NOARG),
	M(ANDl,			"ANDl",			NOARG),
	M(ORl,			"ORl",			NOARG),
	M(XORl,			"XORl",			NOARG),

	M(CPLl,			"CPLl",			NOARG),
	M(NEGl,			"NEGl",			NOARG),
	M(NOTl,			"NOTl",			NOARG),
	M(ABSl,			"ABSl",			NOARG),
	M(SIGNl,		"SIGNl",		NOARG),
	M(MINl,			"MINl",			NOARG),
	M(MAXl,			"MAXl",			NOARG),
	M(MINul,		"MINul",		NOARG),
	M(MAXul,		"MAXul",		NOARG),

	M(ADDGLl,		"ADDGLl",		NOARG),
	M(SUBGLl,		"SUBGLl",		NOARG),
	M(MULGLl,		"MULGLl",		NOARG),
	M(DIVGLl,		"DIVGLl",		NOARG),
	M(DIVGLlu,		"DIVGLlu",		NOARG),
	M(ANDGLl,		"ANDGLl",		NOARG),
	M(ORGLl,		"ORGLl",		NOARG),
	M(XORGLl,		"XORGLl",		NOARG),
	M(SLGLl,		"SLGLl",		NOARG),
	M(SRGLl,		"SRGLl",		NOARG),
	M(SRGLlu,		"SRGLlu",		NOARG),
	M(INCRl,		"INCRl",		NOARG),
	M(DECRl,		"DECRl",		NOARG),
	
	M(EQd,			"EQd",			NOARG),
	M(NEd,			"NEd",			NOARG),
	M(GTd,			"GTd",			NOARG),
	M(LTd,			"LTd",			NOARG),
	M(GEd,			"GEd",			NOARG),
	M(LEd,			"LEd",			NOARG),
	
	M(SLd,			"SLd",			NOARG),
	M(SRd,			"SRd",			NOARG),	
	M(SUBd,			"SUBd",			NOARG),
	M(ADDd,			"ADDd",			NOARG),
	M(MULd,			"MULd",			NOARG),
	M(DIVd,			"DIVd",			NOARG),
	M(NOTd,			"NOTd",			NOARG),
	M(NEGd,			"NEGd",			NOARG),
	M(ADDGLd,		"ADDGLd",		NOARG),
	M(SUBGLd,		"SUBGLd",		NOARG),
	M(MULGLd,		"MULGLd",		NOARG),
	M(DIVGLd,		"DIVGLd",		NOARG),
	M(INCRd,		"INCRd",		NOARG),
	M(DECRd,		"DECRd",		NOARG),
	
	M(LTObool,		"LTObool",		NOARG),
	M(ITOL,			"ITOL",			NOARG),
	M(UTOL, 		"UTOL", 		NOARG),
	M(LTOI,			"LTOI",			NOARG),
	M(LTOi8,		"LTOi8",		NOARG),
	M(LTOi16,		"LTOi16",		NOARG),
	M(LTOu8,		"LTOu8",		NOARG),
	M(LTOu16,		"LTOu16",		NOARG),
	
	M(LTOF, 		"LTOF",			NOARG),
	M(ULTOF, 		"ULTOF", 		NOARG),
	M(FTOL,			"FTOL", 		NOARG),
	M(FTOUL,		"FTOUL",		NOARG),
	
	M(DTObool,		"DTObool",		NOARG),
	M(DTOI,			"DTOI",			NOARG),
	M(DTOU,			"DTOU",			NOARG),
	M(DTOL,			"DTOL",			NOARG),
	M(DTOUL,		"DTOUL",		NOARG),
	M(DTOF,			"DTOF",			NOARG),
	M(ITOD,			"ITOD",			NOARG),
	M(UTOD,			"UTOD",			NOARG),
	M(LTOD,			"LTOD",			NOARG),
	M(ULTOD,		"ULTOD",		NOARG),
	M(FTOD,			"FTOD",			NOARG),
	
#endif 

	
// ===========================================================================
//			VARIADIC opcodes
// ===========================================================================
	
#if VCC_VARIADIC 

	M(NOTv,			"NOTv",			NOARG),
	M(CPLv,			"CPLv",			NOARG),
	M(NEGv,			"NEGv",			NOARG),
	M(EQv,			"EQv",			NOARG),
	M(NEv,			"NEv",			NOARG),
	M(GTv,			"GTv",			NOARG),
	M(LTv,			"LTv",			NOARG),
	M(GEv,			"GEv",			NOARG),
	M(LEv,			"LEv",			NOARG),
	M(SLv,			"SLv",			NOARG),
	M(SRv,			"SRv",			NOARG),
	M(SUBv,			"SUBv",			NOARG),
	M(MULv,			"MULv",			NOARG),
	M(DIVv,			"DIVv",			NOARG),
	M(MODv,			"MODv",			NOARG),
	M(ANDv,			"ANDv",			NOARG),
	M(ORv,			"ORv",			NOARG),
	M(XORv,			"XORv",			NOARG),
	M(PEEKv,		"PEEKv",		NOARG),
	M(ADDv,			"ADDv",			NOARG),
	M(SLGLv,		"SLGLv",		NOARG),
	M(SRGLv,		"SRGLv",		NOARG),
	M(ANDGLv,		"ANDGLv",		NOARG),
	M(ORGLv,		"ORGLv",		NOARG),
	M(XORGLv,		"XORGLv",		NOARG),
	M(ADDGLv,		"ADDGLv",		NOARG),
	M(SUBGLv,		"SUBGLv",		NOARG),
	M(MULGLv,		"MULGLv",		NOARG),
	M(DIVGLv,		"DIVGLv",		NOARG),
	M(INCRv,		"INCRv",		NOARG),
	M(DECRv,		"DECRv",		NOARG),
	
	M(VTOB,"",0),
	M(VTOX,			"VTOX",			ARGi16),
	M(XTOV,			"XTOV",			ARGi16),
	
#endif 
	
	
//
// ============= more to come ================
//	
	

//	M(NEW_OBJECT, "", ARGu16),
//	M(NEW_ARRAY, "", ARGu16),
//	M(SETCONST, "", NOARG),
//
//	M(COUNT, "", NOARG),
//	M(CLONE, "", NOARG),
//	M(CONCAT, "", NOARG),
//	M(CONCATu, "", NOARG),
//	M(CONCATs, "", NOARG),
//	M(LEFTRANGE, "", NOARG),
//	M(MIDRANGE, "", NOARG),
//	M(RIGHTRANGE, "", NOARG),
//
//	M(CALL_VFUNC, "", NOARG),
//
//	M(ENVIRON, "", NOARG),
//	M(ARGV, "", NOARG),
//	M(WAIT, "", NOARG),
//	M(WAIT_UNTIL, "", NOARG),
//	M(WAIT_DELAY, "", NOARG),
//	M(RUN_GC, "", NOARG),

// clang-format on

  #ifdef M_wasnt_defined
}; // enum

	#if !VCC_LONG && !VCC_VARIADIC
	  #define ATIl		  NOP
	  #define ATIGETl	  NOP
	  #define ATISETl	  NOP
	  #define ISETl		  NOP
	  #define IGETl		  NOP
	  #define PEEKl		  NOP
	  #define POKEl		  NOP
	  #define GGETl		  NOP
	  #define LGETl		  NOP
	  #define PUSH_GGETl  NOP
	  #define PUSH_GSETl  NOP
	  #define PUSH_LGETl  NOP
	  #define PUSH_LSETl  NOP
	  #define PUSHl_IVAL  NOP
	  #define PUSHl_IVALs NOP
	  #define PUSHl_GVAR  NOP
	  #define PUSHl_GGET  NOP
	  #define PUSHl_LVAR  NOP
	  #define PUSHl_LGET  NOP
	  #define PUSHl_GGETl NOP
	  #define PUSHl_GSETl NOP
	  #define PUSHl_LGETl NOP
	  #define PUSHl_LSETl NOP
	#endif

	#if !VCC_LONG
	  #define EQl	  NOP
	  #define NEl	  NOP
	  #define GTl	  NOP
	  #define LTl	  NOP
	  #define GEl	  NOP
	  #define LEl	  NOP
	  #define LEul	  NOP
	  #define GEul	  NOP
	  #define LTul	  NOP
	  #define GTul	  NOP
	  #define SLl	  NOP
	  #define SRl	  NOP
	  #define SRul	  NOP
	  #define ADDl	  NOP
	  #define SUBl	  NOP
	  #define MULl	  NOP
	  #define DIVl	  NOP
	  #define DIVul	  NOP
	  #define MODl	  NOP
	  #define MODul	  NOP
	  #define ANDl	  NOP
	  #define ORl	  NOP
	  #define XORl	  NOP
	  #define CPLl	  NOP
	  #define NEGl	  NOP
	  #define NOTl	  NOP
	  #define ABSl	  NOP
	  #define SIGNl	  NOP
	  #define MINl	  NOP
	  #define MAXl	  NOP
	  #define MINul	  NOP
	  #define MAXul	  NOP
	  #define ADDGLl  NOP
	  #define SUBGLl  NOP
	  #define MULGLl  NOP
	  #define DIVGLl  NOP
	  #define DIVGLlu NOP
	  #define ANDGLl  NOP
	  #define ORGLl	  NOP
	  #define XORGLl  NOP
	  #define SLGLl	  NOP
	  #define SRGLl	  NOP
	  #define SRGLlu  NOP
	  #define INCRl	  NOP
	  #define DECRl	  NOP
	  #define EQd	  NOP
	  #define NEd	  NOP
	  #define GTd	  NOP
	  #define LTd	  NOP
	  #define GEd	  NOP
	  #define LEd	  NOP
	  #define SLd	  NOP
	  #define SRd	  NOP
	  #define SUBd	  NOP
	  #define ADDd	  NOP
	  #define MULd	  NOP
	  #define DIVd	  NOP
	  #define NOTd	  NOP
	  #define NEGd	  NOP
	  #define ADDGLd  NOP
	  #define SUBGLd  NOP
	  #define MULGLd  NOP
	  #define DIVGLd  NOP
	  #define INCRd	  NOP
	  #define DECRd	  NOP
	  #define LTObool NOP
	  #define ITOL	  NOP
	  #define UTOL	  NOP
	  #define LTOI	  NOP
	  #define LTOi8	  NOP
	  #define LTOi16  NOP
	  #define LTOu8	  NOP
	  #define LTOu16  NOP
	  #define LTOF	  NOP
	  #define ULTOF	  NOP
	  #define FTOL	  NOP
	  #define FTOUL	  NOP
	  #define DTObool NOP
	  #define DTOI	  NOP
	  #define DTOU	  NOP
	  #define DTOL	  NOP
	  #define DTOUL	  NOP
	  #define DTOF	  NOP
	  #define ITOD	  NOP
	  #define UTOD	  NOP
	  #define LTOD	  NOP
	  #define ULTOD	  NOP
	  #define FTOD	  NOP
	#endif

	#if !VCC_VARIADIC
	  #define NOTv	 NOP
	  #define CPLv	 NOP
	  #define NEGv	 NOP
	  #define EQv	 NOP
	  #define NEv	 NOP
	  #define GTv	 NOP
	  #define LTv	 NOP
	  #define GEv	 NOP
	  #define LEv	 NOP
	  #define SLv	 NOP
	  #define SRv	 NOP
	  #define SUBv	 NOP
	  #define MULv	 NOP
	  #define DIVv	 NOP
	  #define MODv	 NOP
	  #define ANDv	 NOP
	  #define ORv	 NOP
	  #define XORv	 NOP
	  #define PEEKv	 NOP
	  #define ADDv	 NOP
	  #define SLGLv	 NOP
	  #define SRGLv	 NOP
	  #define ANDGLv NOP
	  #define ORGLv	 NOP
	  #define XORGLv NOP
	  #define ADDGLv NOP
	  #define SUBGLv NOP
	  #define MULGLv NOP
	  #define DIVGLv NOP
	  #define INCRv	 NOP
	  #define DECRv	 NOP
	  #define VTOB	 NOP
	  #define VTOX	 NOP
	  #define XTOV	 NOP
	#endif

} // namespace
  #endif
  #undef M_wasnt_defined
  #undef M


#endif
