// Copyright (c) 2010 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


// clang-format off

#if defined M || !defined IDF_ID_H

#ifndef M
#define IDF_ID_H
#define M_wasnt_defined
enum IdfID : unsigned short 
{	
#define M(ID,STR)	ID
#endif


// predefined token ids:

	M(tEMPTYSTR =0,	"" ),		// empty string
	M(tNL,			"\n" ),
	M(tEOF,			"<eof>"),	// end of source

// special ids for immediate values in preprocessor
	//M(t_LINE,	"<line>"),		// LINE, line_number, indent
	M(t_CHAR,	"<char>" ),		// INT, char
	M(t_INT,	"<int>" ),		// INT, lo(int32), hi(int32)
	M(t_LONG,	"<long>" ),		// INT, lolo(int64) .. hihi(int64)
	M(t_FLOAT,	"<float>"),		// FLOAT, lo(float), hi(float)
	M(t_STRING,	"<str>"),		// STR, lo(&str), hi(&str)
	M(t_IDF,	"<idf>"),
//	M(tMACROARG,"<macroarg>" ),	// special id for macro arguments in preprocessor

// operators:
// must match Symbols.cpp: cstr opnames[];

	M(tGL,		"=" ),		// opri 0
//	M(tDPGL,	":=" ),
	M(tADDGL,	"+=" ),
	M(tSUBGL,	"-=" ),
	M(tMULGL,	"*=" ),
	M(tDIVGL,	"/=" ),
//	M(tMODGL,	"%=" ),
//	M(tPTRGL,	"->" ),
	M(tANDGL,	"&=" ),
	M(tORGL,	"|=" ),
	M(tXORGL,	"^=" ),
	M(tSLGL,	"<<=" ),
	M(tSRGL,	">>=" ),
//	M(tANDANDGL,"&&=" ),
//	M(tORORGL,	"||=" ),

	M(tQMARK,	"?" ),		// opri 1
	M(tCOLON,	":" ),

	M(tANDAND,	"&&" ),		// opri 2
	M(tOROR,	"||" ),

//	M(tIS,		"===" ),	// opri 3				
//	M(tISNOT,	"!==" ),	//		 				
	M(tEQ,		"==" ),
	M(tNE,		"!=" ),
	M(tGE,		">=" ),
	M(tLE,		"<=" ),
	M(tGT,		">" ),
	M(tLT,		"<" ),

	M(tADD,		"+" ),		// opri 4
	M(tSUB,		"-" ),

	M(tMUL,		"*" ),		// opri 5
	M(tDIV,		"/" ),
	M(tMOD,		"%" ),
//	M(tDIVMOD,	"/%"),

	M(tAND,		"&" ),		// opri 6
	M(tOR,		"|" ),
	M(tXOR,		"^" ),

	M(tSR,		">>" ),		// opri 7
	M(tSL,		"<<" ),

	M(tINCR,	"++" ),		// opri 9
	M(tDECR,	"--" ),
	M(tNOT,		"!" ),
//	M(tNOTNOT,	"!!" ),
	M(tCPL,		"~" ),
//	M(tNEG,		" -" ),

// special characters:

	M(tRKauf,	"(" ),
	M(tGKauf,	"{" ),
	M(tEKauf,	"[" ),
	M(tRKzu,	")" ),
	M(tGKzu,	"}" ),
	M(tEKzu,	"]" ),
	M(tCOMMA,	"," ),
	M(tDOT,		"." ),
	M(tSEMIK,	";" ),
	M(tHASH,	"#" ),
	M(tAT,		"@" ),
	M(tCENT,	"¢" ),
	M(tDOLLAR,	"$" ),

// start of 'names':
// reserved names:

// block or definition starter:	
	M(tIF,		"if" 	),
	M(tELSE,	"else" 	),
	M(tELIF,	"elif" 	),					// <-- also preprocessor
	M(tFOR,		"for" 	),
	M(tDO,		"do" 	),
	M(tSWITCH,	"switch" ),
	M(tCASE,	"case" 	),
	M(tDEFAULT,	"default" ),
//	M(tTYPE,	"type" 	),
	M(tCONST,	"const" ),
	M(tENUM,	"enum"	),

// other reserved names:	
//	M(tTO,		"to" 	),	
	M(tWHILE,	"while" ),
	M(tUNTIL,	"until" ),
	M(tEXIT,	"exit" 	),
	M(tBREAK,	"break" ),
//	M(tEXTERN,	"extern" ),
//	M(tSCOPE,	"scope" ),
//	M(tEND,		"end" 	),
	M(tASSERT,	"assert" ),
//	M(tSIZEOF,	"sizeof" ),
//	M(tSIZE,	"size" 	),
//	M(tCOUNT,	"count" ),
	M(tRETURN,	"return" ),
	M(tNEXT,	"next" 	),

// preprocessor instructions:

//	M(tENDIF,	"endif" ),
//	M(tINCLUDE,	"include" ),
//	M(tREQUIRE,	"require" ),
//	M(tIFDEF,	"ifdef" ),
//	M(tIFNDEF,	"ifndef" ),
//	M(tDEFINE,	"define" ),
//	M(tUNDEF,	"undef" ),
//	M(tERROR,	"error" ),
//	M(t__FILE__,	"__FILE__" ),		
//	M(t__LINE__,	"__LINE__" ),		
//	M(tDEFINED,		"defined" ), 	
	
// more:

//	M(tOPERATOR,	"operator" 	),
//	M(tOPCODE,		"opcode" 	),
//	M(tASM,			"asm" 		),
//	M(tINLINE,		"inline" 	),
//	M(tREVERTED,	"reverted" 	),
//	M(tGLOBAL,		"global" 	),
//	M(tIMPLICIT,	"implicit"	),
//	M(tSLOT,		"slot" 		),
//	M(tNULL,		"null" 		),

// types:

	M(tVOID,		"void"		),
	M(tINT8,		"int8"		),	// int8 ... void:	same sequence as type_id´s!
	M(tINT16,		"int16"		),
	M(tINT32,		"int32"		),
	M(tINT64,		"int64"		),
	M(tUINT8,		"uint8"		),
	M(tUINT16,		"uint16"	),
	M(tUINT32,		"uint32"	),
	M(tUINT64,		"uint64"	),
	M(tFLOAT,		"float"		),
	M(tDOUBLE,		"double"	),
	M(tVARIADIC,	"var"   	),
	M(tSTR,			"str"		),
//	M(tFLOAT32,		"float32"	),
//	M(tFLOAT64,		"float64"	),
//	M(tFLOAT128,	"float128"	),
//	M(tBYTE,		"byte"		),
//	M(tUBYTE,		"ubyte"		),
	M(tCHAR,		"char"		),
//	M(tUTF8CHAR,	"utf8char"	),
//	M(tUCS1CHAR,	"ucs1char"	),
//	M(tUCS2CHAR,	"ucs2char"	),
//	M(tUCS4CHAR,	"ucs4char"	),
//	M(tUTF8STR,		"utf8str"	),
//	M(tUCS1STR,		"ucs1str"	),
//	M(tUCS2STR,		"ucs2str"	),
//	M(tUCS4STR,		"ucs4str"	),
//	M(tSHORT,		"short"		),
//	M(tUSHORT,		"ushort"	),
	M(tINT,			"int"		), // >= sizeof register, >= sizeof pointer
	M(tUINT,		"uint"		),
	M(tLONG,		"long"		), // >= 32 bit, >= sizeof int
	M(tULONG,		"ulong"		),
//	M(tMP_INT,		"mp_int"	),
//	M(tMP_FLOAT,	"mp_float"	),
//	M(tPTR,			"ptr"		),
//	M(tPROC,		"proc"		),
//	M(tSIGNAL,		"signal"	),
//	M(tOBJECT_t,	"Object"	),
//	M(tMUTEX_t,		"Mutex"		),
//	M(tSEMA_t,		"Sema"		),
//	M(tIRPT_t,		"Irpt"		),
//	M(tTHREAD_t,	"Thread"	),

// instructions:

//	M(tDEALLOC,		"dealloc"	),
//	M(tALLOC,		"alloc"		),
//	M(tREALLOC,		"realloc"	),
//	M(tRETAIN,		"retain"	),
//	M(tNEW,			"new"		),
//	M(tCOPYOF,		"copyof"	),
//	M(tDELETE,		"delete"	),
//	M(tDISPOSE,		"dispose"	),
//	M(tREFCNT,		"refcnt"	),
//	M(tIS_SHARED,	"is_shared"	),
//	M(tINIT,		"init"		),
//	M(tCOPY,		"copy"		),
//	M(tCOPYQIZIN,	"copyqizin"	),
//	M(tCOPYRANGE,	"copyrange"	),
//	M(tKILL,		"kill"		),
//	M(tCONNECT,		"connect"	),		unten bei Sockets definiert
//	M(tDISCONNECT,	"disconnect"),
//	M(tSEND,		"send"		),
//	M(tSHRINK,		"shrink"	),
//	M(tSHRINKSTRARRAY,"shrinkstrarray"	),
//	M(tGROW,		"grow"		),
//	M(tRESIZE,		"resize"	),

// opcodes:

	M(tPEEK,		"peek"		),
//	M(tPEEK_NZ,		"peek_nz"	),
//	M(tPEEK_CLEAR,	"peek_clear"),
//	M(tPEEKSIGNAL,	"peeksignal"),
	M(tPOKE,		"poke"		),
	M(tSWAPWITHVAR,	"swapwithvar" ),
	M(tSWAP,		"swap"		),
	M(tNOP,			"nop"		),
	M(tIN,			"in"		),
	M(tOUT,			"out"		),
	M(tCAST,		"cast"		),
	M(tTOR,			"tor"		),
	M(tDUP2R,		"dup2r"		),
	M(tFROMR,		"fromr"		),
	M(tPEEKR,		"peekr"		),
	M(tPUSHR,		"pushr"		),
	M(tDROPR,		"dropr"		),
	M(tBOOL,		"bool"		),
	M(tTRUE,		"true"		),
	M(tFALSE,		"false"		),
	M(tJP,			"jp"		),
	M(tJP_Z,		"jp_z"		),
	M(tJP_NZ,		"jp_nz"		),
	M(tAND0,		"and0"		),
	M(tOR1,			"or1"		),
//	M(tFORALLITEMS,	"forallitems"),
//	M(tFORRANGE,	"forrange"	),
	M(tPPPEEK,		"pppeek"	),
	M(tMMPEEK,		"mmpeek"	),
	M(tPEEKPP,		"peekpp"	),
	M(tPEEKMM,		"peekmm"	),
	M(tATINDEX,		"atindex"	),
	M(tDROP,		"drop"		),
	M(tNIP,			"nip"		),
	M(tDUP,			"dup"		),
	M(tDUP2,		"dup2"		),
//	M(tDUP2_MIDSTR,	"dup2_midstr"),
//	M(tDUP_IF_NZ,	"dup_if_nz"	),
//	M(tSUPER,		"super"		),
//	M(tNULLPTR,		"nullptr"	),

//	M(tPUSH_CONTEXT0,	"push_context0" ),
//	M(tPUSH_CONTEXT1,	"push_context1" ),
//	M(tPUSH_CONTEXT2,	"push_context2" ),
//	M(tPUSH_CONTEXT3,	"push_context3" ),
//	M(tPOP_CONTEXT0,	"pop_context0"  ),
//	M(tPOP_CONTEXT1,	"pop_context1"  ),
//	M(tPOP_CONTEXT2,	"pop_context2"  ),
//	M(tPOP_CONTEXT3,	"pop_context3"  ),

//	M(tCATSTR,		"catstr"	),
//	M(tCATCHAR,		"catchar"	),
//	M(tADDARRAY,	"addarray"	),
//	M(tADDITEM,		"additem"	),
//	M(tAPPENDARRAY,	"appendarray"),
//	M(tAPPENDITEM,	"appenditem"),
//	M(tADDSTRARRAY,		"addstrarray"	),
//	M(tADDSTRITEM,		"addstritem"	),
//	M(tAPPENDSTRARRAY,	"appendstrarray"),
//	M(tAPPENDSTRITEM,	"appendstritem" ),
	M(tEQSTR,		"eqstr"		),
	M(tNESTR,		"nestr"		),
	M(tGESTR,		"gestr"		),
	M(tLESTR,		"lestr"		),
	M(tGTSTR,		"gtstr"		),
	M(tLTSTR,		"ltstr"		),
//	M(tFIRST,		"first"		),
//	M(tLAST,		"last"		),

	M(tGETCHAR,		"getchar"	),	// stdin
	M(tPUTCHAR,		"putchar"	),	// stdout
	M(tPUTBYTE,		"putbyte"	),
	M(tPUTSTR,		"putstr"	),
	M(tPUTNUM,		"putnum"	),
//	M(tLOGCHAR,		"logchar"	),	// stderr
//	M(tLOGSTR,		"logstr"	),
//	M(tLOGNUM,		"lognum"	),

	M(tSPACESTR,	"spacestr"	),
	M(tSUBSTR,		"substr"	),
	M(tMIDSTR,		"midstr"	),
	M(tLEFTSTR,		"leftstr"	),
	M(tRIGHTSTR,	"rightstr"	),
	M(tNUMSTR,		"numstr"	),
//	M(tERRORSTR,	"errorstr"	),
//	M(tMIN,			"min"		),
//	M(tMAX,			"max"		),
//	M(tNOW,			"now"		),
//	M(tTIME,		"time"		),
//	M(tTIMESTR,		"timestr"	),
//	M(tTIMEVAL,		"timeval"	),
//	M(tSELF,		"self"		),
//	M(tFREEZE,		"freeze"	),
//	M(tVFREE,		"vfree"		),
//	M(tRFREE,		"rfree"		),
//	M(tTIMER,		"timer"		),
//	M(tWAIT,		"wait"		),
//	M(tBUSYWAIT,	"busywait"	),
//	M(tSHEDULE,		"shedule"	),
//	M(tRUN,			"run"		),
//	M(tSETPRIO,		"setprio"	),
//	M(tSUSPEND,		"suspend"	),
//	M(tRESUME,		"resume"	),
//	M(tTERMI,		"termi"		),
//	M(tCLEAR,		"clear"		),
//	M(tREQUEST,		"request"	),
//	M(tRELEASE,		"release"	),
//	M(tTRYREQUEST,	"tryrequest"),
//	M(tLOCK,		"lock"		),
//	M(tUNLOCK,		"unlock"	),
//	M(tTRYLOCK,		"trylock"	),
//	M(tTRIGGER,		"trigger"	),
//	M(tSTDIN,		"stdin"		),
//	M(tSTDOUT,		"stdout"	),
//	M(tSTDERR,		"stderr"	),
//	M(tEI,			"ei"		),
//	M(tDI,			"di"		),
//	M(tA,			"a"			),
//	M(tB,			"b"			),
//	M(tC,			"c"			),
//	M(tI,			"i"			),

	M(tRANDOM,		"random"	),
//	M(tSORT,		"sort"		),
//	M(tRSORT,		"rsort"		),
//	M(tSHUFFLE,		"shuffle"	),
//	M(tREVERT,		"revert"	),
//	M(tFLIP,		"flip"		),	renamed to 'revert'
//	M(tROL,			"rol"		),
//	M(tROR,			"ror"		),
//	M(tFIND,		"find"		),
//	M(tRFIND,		"rfind"		),
//	M(tJOIN,		"join"		),
//	M(tSPLIT,		"split"		),
	M(tMSBIT,		"msbit"		),
	M(tMSBIT0,		"msbit0"	),
//	M(tNAN,			"NaN"		),
//	M(tINF_M,		"inf_m"		),
//	M(tZERO,		"zero"		),

//	M(tESCAPE,		"escape"	),
//	M(tUNESCAPE,	"unescape"	),
//	M(tENTAB,		"entab"		),
//	M(tDETAB,		"detab"		),
//	M(tTOUTF8,		"toutf8"	),
//	M(tTOUCS1,		"toucs1"	),
//	M(tTOUCS2,		"toucs2"	),
//	M(tTOUCS4,		"toucs4"	),
//	M(tTOUPPER,		"toupper"	),
// 	M(tTOLOWER,		"tolower"	),
//	M(tURLENCODE,	"urlencode"	),
// 	M(tURLDECODE,	"urldecode"	),

	M(tABS,			"abs"		),
	M(tSIGN,		"sign"		),
	M(tROUND,		"round"		),
	M(tFLOOR,		"floor"		),
	M(tCEIL,		"ceil"		),
	M(tINTEG,		"integ"		),
	M(tFRACT,		"fract"		),
	M(tSIN,			"sin"		),
	M(tCOS,			"cos"		),
	M(tTAN,			"tan"		),
	M(tASIN,		"asin"		),
	M(tACOS,		"acos"		),
	M(tATAN,		"atan"		),
	M(tSINH,		"sinh"		),
	M(tCOSH,		"cosh"		),
	M(tEXP10,		"exp10"		),
	M(tLOG10,		"log10"		),
	M(tEXP2,		"exp2"		),
	M(tLOG2,		"log2"		),
	M(tEXPE,		"expe"		),
	M(tLOGE,		"loge"		),
	M(tEXP,			"exp"		),
	M(tLOG,			"log"		),
	M(tSQRT,		"sqrt"		),
	M(tTANH,		"tanh"		),
	M(tPOW,			"pow"		),
	M(tMANT,		"mant"		),
	M(tFREXP,		"frexp"		),
	M(tLO,			"lo"		),
	M(tHI,			"hi"		),
//	M(tLOHI,		"lohi"		),
//	M(tHILO,		"hilo"		),
//	M(tIS_LETTER,	"is_letter" ),
//	M(tIS_BIN_DIGIT,"is_bin_digit" ),
//	M(tIS_HEX_DIGIT,"is_hex_digit" ),
//	M(tIS_DEC_DIGIT,"is_dec_digit" ),
//	M(tDIGIT_VALUE,	"digit_value" ),
	M(tMEMCPY,		"memcpy"	),
	M(tRMEMCPY,		"rmemcpy"	),
	M(tMEMMOVE,		"memmove"	),
	M(tMEMCLR,		"memclr"	),
	M(tMEMSET,		"memset"	),
	M(tCHARSTR,		"charstr"	),
	M(tHEXSTR,		"hexstr"	),
	M(tBINSTR,		"binstr"	),
	M(tNUMVAL,		"numval"	),
	M(tINF,			"inf"		),
	M(tNAN,			"nan"		),
//	M(tNUM_CORES,	"num_cores" ),
//	M(t_RANGETOEND,	"helper_range_to_end"),
//	M(tINFO_START_OF_ARRAY_ITEMS,"info: start_of_array_items"),
//	M(tINFO_END_OF_ARRAY_ITEMS,  "info: end_of_array_items"),
//	M(tSELECT,		"select"	),
	M(tSYSTEMTIME,	"systemtime"),
//	M(tDESELECT,	"deselect"	),
//	M(tABORT,		"abort"		),
	M(tTODO,		"todo"		),
//	M(tOOMEM,		"oomem"		),
	M(tPANIC,		"panic"		),
//	M(tTRON,		"tron"		),
//	M(tTROFF,		"troff"		),

//	M(tFD,			"FD"		),
//	M(tOPEN,		"open"		),
//	M(tCLOSE,		"close"		),
//	M(tFLEN,		"flen"		),
//	M(tFPOS,		"fpos"		),
//	M(tSEEK,		"seek"		),
//	M(tTRUNCATE,	"truncate"	),
//	M(tREAD,		"read"		),
//	M(tWRITE,		"write"		),
//	M(tPRINT,		"print"		),
//	M(tSOCKET,		"socket"	),
//	M(tCONNECT,		"connect"	),
//	M(tACCEPT,		"accept"	),
//	M(tSTREAM_t,	"Stream"	),
//	M(tSTREAMS,		"streams"	),
//	M(tENV,			"env"		),
//	M(tARGS,		"args"		),

//	M(tMATCH,		"match"		),
//	M(tFULLPATH,	"fullpath"	),
//	M(tNODETYPE,	"nodetype"	),
//	M(tFILEINFO,	"fileinfo"	),
//	M(tREADFILE,	"readfile"	),
//	M(tREADLINK,	"readlink"	),
//	M(tREADDIR,		"readdir"	),
//	M(tCREATEFILE,	"createfile"),
//	M(tCREATELINK,	"createlink"),
//	M(tCREATEDIR,	"createdir"	),
//	M(tCREATEPIPE,	"createpipe"),
//	M(tRMVFILE,		"rmvfile"	),
//	M(tRMVLINK,		"rmvlink"	),
//	M(tRMVDIR,		"rmvdir"	),
//	M(tRENAMEFILE,	"renamefile"),
//	M(tSWAPFILES,	"swapfiles"	),
//	M(tTEMPFILE,	"tempfile"	),

//	M(tLOCAL,		"local"		),
//	M(tINET,		"inet"		),
//	M(tTCP,			"tcp"		),
//	M(tUDP,			"udp"		),
//	M(t_STREAM,		"stream"	),
//	M(tPACKET,		"packet"	),
//	M(tDATAGRAM,	"datagram"	),

//	M(tCSTRARRAY,	"4int8AEcAE¢"),		// cstr[]
//	M(tFILEINFO_t,	"FileInfo"	),		// class FileInfo

//	M(tOPERATOR,    "operator"),
//	M(tIDENTIFIER,  "identifier"),

#ifdef M_wasnt_defined
};
#endif 
#undef M_wasnt_defined
#undef M

#endif


// clang-format on
