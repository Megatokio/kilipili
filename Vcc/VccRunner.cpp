// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "DReg.h"
#include "Opcode.h"
#include "Var.h"
#include "Vcc.h"
#include "Xoshiro128.h"
#include <math.h>
#include <pico/stdlib.h>

namespace kio::Vcc
{

static Xoshiro128 rng {time_us_32()};


// clang-format off
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
	
Var execute(Var* ram, const uint16* ip, const uint16** rp, Var* sp)
{
	DReg		  top = 1;	
	
#define push(X)  *--sp = (X)
#define pop		 (*sp++)
#define N		 int(*int16ptr(ip++))
#define U		 uint(*uint16ptr(ip++))
#define jr()	 ip += *int16ptr(ip)
#define pushr(X) *rp++ = (X)
#define popr     *--rp
	
	constexpr uint16 opcode_exit = EXIT;
	pushr(&opcode_exit);
	
//	// handle exception:
//	if (0)
//	{
//_throw:	assert(tp != nullptr);
//		rp = tp;
//		ip = *rp++ +1;	// -> behind CATCH
//		sp = reinterpret_cast<Var*>(*rp++);
//		tp = reinterpret_cast<uint16**>(*rp++);
//	}

//	try
//	{

#define loop goto _loop
_loop:

	switch (uint8(*ip++))
	{
	case NOP: loop;
	case PUSH: push(top); loop;		// technically this is DUP
	case POP:  top = pop.u32; loop;	// technically this is DROP
		
	// Numeric Literals:

	// technically these are not PUSH because they ignore the TOP value:
	// used between instructions to create new variables.
	case PUSH0x4: push(0);		  //  ( void -- 0 0 0 0 void )	
	case PUSH0x3: push(0);		  //  ( void -- 0 0 0 void )
	case PUSH0x2: push(0);		  //  ( void -- 0 0 void )
	case PUSH0:   push(0); loop;  //  ( void -- 0 void )

	// immediate value: 
	// note: larger constants are stored in globals[]
	case PUSH_IVALi16:	push(top);						//  ( -- i16 )
	case IVALi16:		top = N; loop;					//  ( void -- i16 )
	case PUSH_IVAL:		push(top);						//  ( -- i16 )
	case IVAL:			top = N; top += N << 16; loop;	//  ( void -- i16 )

	// Global Variables in gvar[]:

	case PUSH_GVAR:	push(top);			//  ( -- addr )
	case GVAR:		top = &ram[U]; loop;//  ( void -- addr )
	case PUSH_GGET:	push(top);			//  ( -- value )
	case GGET:		top = ram[U]; loop;	//  ( void -- value )
	case GSET:		ram[U] = top; loop;	//  ( value -- void )
	
	// Local Variables on Stack:

	case PUSH_LVAR: push(top);			//  ( -- addr )
	case LVAR:		top = &sp[U]; loop;	//  ( void -- addr )
	case PUSH_LGET:	push(top);			//  ( -- value )
	case LGET:	    top = sp[U]; loop;	//  ( void -- value )
	case LSET:		sp[U] = top; loop;	//  ( value -- void )

	// Items in Objects:
	// offset and data alignment depend on data size!
	// => also usable for ati[U] with fixed index U

	case IVAR:	  top.i32ptr += U;	   loop; //  ( addr -- addr )
	case IVAR8:   top.i8ptr  += U;	   loop;
	case IVAR16:  top.i16ptr += U;	   loop;
	case IGET:	  top = top.i32ptr[U]; loop; //  ( addr -- value )
	case IGETi8:  top = top.i8ptr[U];  loop;
	case IGETi16: top = top.i16ptr[U]; loop;
	case IGETu8:  top = top.u8ptr[U];  loop;
	case IGETu16: top = top.u16ptr[U]; loop;
	case ISET:    top.i32ptr[U] = pop; loop; //  ( value addr U -- void )
	case ISET8:   top.i8ptr [U] = pop; loop;
	case ISET16:  top.i16ptr[U] = pop; loop;

	// Items in Arrays:

	case ATI:	    top.i32ptr += pop; loop;	 //  ( idx addr -- addr )
	case ATI8:	    top.i8ptr  += pop; loop;
	case ATI16:	    top.i16ptr += pop; loop;
	case ATIGET:	top = top.i32ptr[pop]; loop; //  ( idx addr -- value )
	case ATIGETi8:  top = top.i8ptr [pop]; loop;
	case ATIGETi16: top = top.i16ptr[pop]; loop;
	case ATIGETu8:  top = top.u8ptr [pop]; loop;
	case ATIGETu16: top = top.u16ptr[pop]; loop;
	case ATISET: 	{ uint idx = pop; top.i32ptr[idx] = pop; } loop;
	case ATISET8:	{ uint idx = pop; top.i8ptr [idx] = pop; } loop; //  ( value idx addr -- )
	case ATISET16:	{ uint idx = pop; top.i16ptr[idx] = pop; } loop;
		
	// get / set arbitrary address:

	case PEEK:		top = *top.i32ptr; loop; 	// peek ( addr -- value )
	case PEEKi8:	top = *top.i8ptr;  loop;
	case PEEKu8:	top = *top.u8ptr;  loop;
	case PEEKi16:	top = *top.i16ptr; loop;
	case PEEKu16:	top = *top.u16ptr; loop;
	case POKE:  	*top.i32ptr = pop; loop;	// poke ( value addr -- void )
	case POKE8: 	*top.u8ptr  = pop; loop;
	case POKE16:	*top.u16ptr = pop; loop;

	// Integer Arithmetics:

	case ADD:	top.i32  +=  pop.i32; loop;	//  ( A B -- B+A )
	case SUB:	top.i32  -=  pop.i32; loop;	//  ( A B -- B-A )
	case MUL:	top.i32  *=  pop.i32; loop;	//  ( A B -- B*A )
	case DIV:	top.i32  /=  pop.i32; loop;	//  ( A B -- B/A )
	case DIVu: 	top.u32  /=  pop.u32; loop;	//  ( A B -- B/A )
	case MOD:	top.i32  %=  pop.i32; loop;	//  ( A B -- B%A )
	case MODu: 	top.u32  %=  pop.u32; loop;	//  ( A B -- B%A )
	case AND:	top.u32  &=  pop.u32; loop;	//  ( A B -- B&A )
	case OR: 	top.u32  |=  pop.u32; loop;	//  ( A B -- B|A )
	case XOR:	top.u32  ^=  pop.u32; loop;	//  ( A B -- B^A )
	case SL:	top.u32 <<=  pop.u32; loop; //  ( A B -- B<<A )
	case SR: 	top.i32 >>=  pop.u32; loop; //  ( A B -- B>>A )
	case SRu:	top.u32 >>=  pop.u32; loop; //  ( A B -- B>>A )

	case ADDI:	top += N; loop;  		//  ( N -- N )
	case MULI:	top *= N; loop;
	case DIVI:	top /= N; loop;
	case DIVIu:	top /= U; loop;
	case ANDI:	top &= U; loop;
	case ORI:	top |= U; loop;
	case XORI:	top ^= U; loop;
	case SLI:	top.u32 <<= U; loop;
	case SRI:	top.u32 >>= U; loop;
	case SRIu:	top.i32 >>= U; loop;

	case ADD1:	top += 1; loop;			//  ( N -- N )
	case ADD2:	top += 2; loop; 
	case SUB1:	top -= 1; loop; 
	case SUB2:	top -= 2; loop; 
		
	case SL1:	top.u32 <<= 1; loop;	//  ( N -- N )
	case SL2:	top.u32 <<= 2; loop;
	case SR1:	top.i32 >>= 1; loop;
	case SR2:	top.i32 >>= 2; loop;
	case SR1u:	top.u32 >>= 1; loop;
	case SR2u:	top.u32 >>= 2; loop;

	case CPL:	top = ~top; loop;			//  ( N -- N )
	case NEG:	top = -top; loop;			//  ( N -- N )
	case NOT:	top = !top.i32; loop;		//  ( N -- N )
	case ABS:	top = abs(top.i32); loop;	//  ( N -- N )
	case SIGN:	top = sign(top.i32); loop;	//  ( N -- N )

	case MIN:	top = min(top.i32,pop.i32); loop;
	case MINu:	top = min(top.u32,pop.u32); loop;
	case MAX:	top = max(top.i32,pop.i32); loop;
	case MAXu:	top = max(top.u32,pop.u32); loop;
	case RANDOMu: top = rng.random(top.u32); loop;	//  ( void -- N )	[0 .. [max
	//case NOW:	top = now<time_t>(); loop;			// seconds since 00:00 hours, Jan 1, 1970 UTC

	case EQ:  	top = top.i32 == pop.i32; loop; //  ( A B -- B==A )
	case NE:  	top = top.i32 != pop.i32; loop; //  ( A B -- B!=A )
	case LT:  	top = top.i32 <  pop.i32; loop; //  ( A B -- B< A )
	case LE:  	top = top.i32 <= pop.i32; loop; //  ( A B -- B<=A )
	case GT:  	top = top.i32 >  pop.i32; loop; //  ( A B -- B> A )
	case GE:  	top = top.i32 >= pop.i32; loop; //  ( A B -- B>=A )
	case LTu:  	top = top.u32 <  pop.u32; loop; //  ( A B -- B< A )
	case LEu:  	top = top.u32 <= pop.u32; loop; //  ( A B -- B<=A )
	case GTu:  	top = top.u32 >  pop.u32; loop; //  ( A B -- B> A )
	case GEu:  	top = top.u32 >= pop.u32; loop; //  ( A B -- B>=A )
		
	case ADDGL:	*top.i32ptr += pop.i32; loop;
	case SUBGL: *top.i32ptr -= pop.i32; loop; 
	case MULGL: *top.i32ptr *= pop.i32; loop; 
	case DIVGL: *top.i32ptr /= pop.i32; loop; 
	case DIVGLu:*top.u32ptr /= pop.u32; loop;
	case ANDGL: *top.i32ptr &= pop.i32; loop; 
	case ORGL:  *top.i32ptr |= pop.i32; loop; 
	case XORGL: *top.i32ptr ^= pop.i32; loop;
	case SLGL:  *top.i32ptr <<= pop.i32; loop;
	case SRGL:  *top.i32ptr >>= pop.i32; loop;
	case SRGLu: *top.u32ptr >>= pop.i32; loop;
	case INCR:  (*top.i32ptr)++; loop;
	case DECR:  (*top.i32ptr)--; loop;
				
	case ADDGLs: *top.i16ptr += pop.i32; loop;
	case SUBGLs: *top.i16ptr -= pop.i32; loop;
	case ANDGLs: *top.i16ptr &= pop.i32; loop;
	case ORGLs:	 *top.i16ptr |= pop.i32; loop;
	case XORGLs: *top.i16ptr ^= pop.i32; loop;
	case INCRs:	 (*top.i16ptr)++; loop;
	case DECRs:	 (*top.i16ptr)--; loop;
		
	case ADDGLb: *top.i8ptr += pop.i32; loop;
	case SUBGLb: *top.i8ptr -= pop.i32; loop;
	case ANDGLb: *top.i8ptr &= pop.i32; loop;
	case ORGLb:	 *top.i8ptr |= pop.i32; loop;
	case XORGLb: *top.i8ptr ^= pop.i32; loop;
	case INCRb:	 (*top.i8ptr)++; loop;
	case DECRb:	 (*top.i8ptr)--; loop;
		
	// Floating Point Arithmetics:

	case ADDf:	top = top.f32 + pop.f32; loop;		//  ( A B -- B+A )
	case SUBf:	top = top.f32 - pop.f32; loop;		//  ( A B -- B-A )
	case MULf:	top = top.f32 * pop.f32; loop;		//  ( A B -- B*A )
	case DIVf:	top = top.f32 / pop.f32; loop;		//  ( A B -- B/A )
	case SLf:	top = ldexp(top.f32,+pop.i32); loop;//  ( A B -- B<<A )
	case SRf:	top = ldexp(top.f32,-pop.i32); loop;//  ( A B -- B>>A )
	case ADD1f:	top.f32++; loop;					//  ( value -- value )
	case SUB1f:	top.f32--; loop;
	case NOTf:	top = top.f32 == 0.0f; loop;		//  ( f32 -- i32 )
	case NEGf:	top = -top.f32; loop;
	case ABSf:	top = fabs(top.f32); loop;
	case SIGNf:	top = sign(top.f32); loop;			//  ( float -- int )

	case SIN:	top = sin(top.f32); loop;
	case COS:	top = cos(top.f32); loop;
	case TAN:	top = tan(top.f32); loop;
	case ASIN:	top = asin(top.f32); loop;
	case ACOS:	top = acos(top.f32); loop;
	case ATAN:	top = atan(top.f32); loop;

	case SINH:  top = sinh(top.f32); loop;
	case COSH:  top = cosh(top.f32); loop;
	case TANH:  top = tanh(top.f32); loop;
	case ASINH: top = asinh(top.f32); loop;
	case ACOSH: top = acosh(top.f32); loop;
	case ATANH: top = atanh(top.f32); loop;

	case LOGE:	top = log(top.f32); loop;				 //  ( f32 -- f32 )
	case LOG10:	top = log10(top.f32); loop;				 //	 ( f32 -- f32 )		ln(top) / ln(10)
	case LOG2:	top = log2(top.f32); loop;				 //  ( f32 -- f32 )		ln(top) / ln(2)
	case LOG:	top = log(pop.f32) / log(top.f32); loop; //  ( N E -- logE(N) )	log(e,n) = ln(n) / ln(e)

	case EXPE:	top = exp(top.f32); loop;				 //  ( f32 -- f32 )
	case EXP2:	top = exp2(top.f32); loop;				 //  ( f32 -- f32 )		exp(top) * ln(2)
	case EXP10:	top = exp(top.f32) * log(10.0f); loop;	 //  ( f32 -- f32 )		exp10(N) is only defined as double
	case EXP:	top = exp(pop.f32) * log(top.f32); loop; //  ( N E -- expE(N) )	exp(e,n) = exp(n) * ln(e)

	case SQRT:	top = sqrt(top.f32); loop;				 //  ( f32 -- f32 )
	case FLOOR:	top = floor(top.f32); loop;				 //  ( f32 -- f32 )
	case ROUND:	top = round(top.f32); loop;				 //  ( f32 -- f32 )
	case CEIL:	top = ceil(top.f32); loop;				 //  ( f32 -- f32 )
		
	case FRACT:		{ float z; top = modf(top.f32, &z); loop; }	//  ( N -- N%1 )
	case INTEG:		{ modf(top.f32, &top.f32); loop; }			//  ( N -- N/1 )
	case MANTISSA:	{ int i; top = frexp(top.f32, &i); loop; }	//  ( f32 -- i32 ) top!=0 --> 0.5 <= top < 1.0
	case EXPONENT:	{ frexp(top.f32, &top.i32); loop; }			//  ( f32 -- i32 ) 
	case COMPOSE:   { top = ldexp(top.f32, pop.i32); loop; }	//  ( E:i32 f32 -- f32 )  compose(mant,exp)

	case MINf:		top = min(top.f32,pop.f32); loop;
	case MAXf:		top = max(top.f32,pop.f32); loop;
	case RANDOMf1:	top = rng.random(); loop;			//  ( void -- f32 )		[0 .. [1
	case RANDOMf:	top = rng.random(top.f32); loop;	//  ( f32 -- f32 )		[0 .. [max
	//case NOWf:  	top = now(); loop;					// seconds since 00:00 hours, Jan 1, 1970 UTC

	case EQf:  	top = top.f32 == pop.f32; loop;		//  ( A B -- B==A )
	case NEf:  	top = top.f32 != pop.f32; loop;		//  ( A B -- B!=A )
	case LTf:  	top = top.f32 <  pop.f32; loop;		//  ( A B -- B< A )
	case LEf:  	top = top.f32 <= pop.f32; loop;		//  ( A B -- B<=A )
	case GTf:  	top = top.f32 >  pop.f32; loop;		//  ( A B -- B> A )
	case GEf:  	top = top.f32 >= pop.f32; loop;		//  ( A B -- B>=A )

	case ADDGLf: *top.f32ptr += pop.f32; loop;
	case SUBGLf: *top.f32ptr -= pop.f32; loop;
	case MULGLf: *top.f32ptr *= pop.f32; loop;
	case DIVGLf: *top.f32ptr /= pop.f32; loop;
	case INCRf:  (*top.f32ptr)++; loop;
	case DECRf:  (*top.f32ptr)--; loop;
		
	// Flow Control:

	case JZ:	if (top.i32 == 0)	jr(); else ip++; loop;  // ( N -- void )
	case JNZ:	if (top.i32 != 0)	jr(); else ip++; loop;  // ( N -- void )
	case JEQ:	if (top == pop.u32) jr(); else ip++; loop;  // ( A B -- void )
	case JNE:	if (top != pop.u32) jr(); else ip++; loop;  // ( A B -- void )
	case JLT:	if (top <  pop.i32) jr(); else ip++; loop;  // ( A B -- void )
	case JLTu:	if (top <  pop.u32) jr(); else ip++; loop;  // ( A B -- void )
	case JLE:	if (top <= pop.i32) jr(); else ip++; loop;  // ( A B -- void )
	case JLEu:	if (top <= pop.u32) jr(); else ip++; loop;  // ( A B -- void )
	case JGE:	if (top >= pop.i32) jr(); else ip++; loop;  // ( A B -- void )
	case JGEu:	if (top >= pop.u32) jr(); else ip++; loop;  // ( A B -- void )
	case JGT:	if (top >  pop.i32) jr(); else ip++; loop;  // ( A B -- void )
	case JGTu:	if (top >  pop.u32) jr(); else ip++; loop;  // ( A B -- void )

	case JEQI:	if (top == N) jr(); else ip++; loop; 		// ( N -- void )
	case JNEI:	if (top != N) jr(); else ip++; loop; 		// ( N -- void )
	case JLTI:	if (top <  N) jr(); else ip++; loop; 		// ( N -- void )
	case JLTIu:	if (top <  U) jr(); else ip++; loop; 		// ( N -- void )
	case JLEI:	if (top <= N) jr(); else ip++; loop; 		// ( N -- void )
	case JLEIu:	if (top <= U) jr(); else ip++; loop; 		// ( N -- void )
	case JGEI:	if (top >= N) jr(); else ip++; loop; 		// ( N -- void )
	case JGEIu:	if (top >= U) jr(); else ip++; loop; 		// ( N -- void )
	case JGTI:	if (top >  N) jr(); else ip++; loop; 		// ( N -- void )
	case JGTIu:	if (top >  U) jr(); else ip++; loop; 		// ( N -- void )
		
	case JZf:	if (top == 0.0f)	jr(); else ip++; loop;	// ( N -- void )
	case JNZf:	if (top != 0.0f)	jr(); else ip++; loop;	// ( N -- void )
	case JEQf:	if (top == pop.f32)	jr(); else ip++; loop;	// ( A B -- void )
	case JNEf:	if (top != pop.f32)	jr(); else ip++; loop;	// ( A B -- void )
	case JLTf:	if (top <  pop.f32)	jr(); else ip++; loop;	// ( A B -- void )
	case JLEf:	if (top <= pop.f32)	jr(); else ip++; loop;	// ( A B -- void )
	case JGEf:	if (top >= pop.f32)	jr(); else ip++; loop;	// ( A B -- void )
	case JGTf:	if (top >  pop.f32)	jr(); else ip++; loop;	// ( A B -- void )

	case JR:	jr(); loop;

	case CALL:	pushr(ip); ip = top; top = pop; loop;
	case JSR:	pushr(ip + 2);
	case JP:	ip += *uint16ptr(ip) + (*int16ptr(ip+1)<<16); loop;	// relative!	
	case RET:	ip = popr; loop;

	case SWITCH: //	 ( value -- void )		 SWITCH, max, dist[max], dflt, code…
		top.u32 = min(top.u32, U);
		ip += top.u32; jr(); loop;

	case TRY: TODO();
	//	*--rp = uint16ptr(tp);
	//	*--rp = uint16ptr(sp);
	//	*--rp = ip + *int16ptr(ip); ip++;
	//	tp = rp;
	//	loop;

	case THROW: TODO();
	//	if (r.memptr->isa(ExceptionClassID))
	//	{
	//		reinterpret_cast<Exception*>(r.memptr)->ip = ip;
	//	}

	//	if (tp) // instead of _throw: this way we don't leave the c++ try-catch block
	//	{
	//		rp = tp;
	//		ip = *rp++ +1;	// -> behind CATCH
	//		sp = reinterpret_cast<Var*>(*rp++);
	//		tp = reinterpret_cast<uint16**>(*rp++);
	//		loop;
	//	}
	//	else goto _throw_unhandled;	// throw unhandled exception

	case TRYEND: TODO();
	//	rp += 1;
	//	sp = reinterpret_cast<Var*>(*rp++);
	//	tp = reinterpret_cast<uint16**>(*rp++);
	//	loop;

	case CATCH: TODO();
	//	// executed if program executes RET inside a TRY-CATCH block:
	//	// -> restore sp and tp and RET again
	//	sp = reinterpret_cast<Var*>(*rp++);
	//	tp = reinterpret_cast<uint16**>(*rp++);
	//	ip = *rp++;
	//	loop;
		
	// technically these are not DROP, because they ignore the TOP value
	// used between instructions to destroy variables and before return.
	case DROP_RET:	ip = popr;		//  ( X rval -- rval )		note: rval may be void
	case DROP:		sp += 1; loop;	//  ( X void -- void )
	case DROP2_RET: ip = popr;		//  ( A B rval -- rval )	note: rval may be void
	case DROP2:		sp += 2; loop;	//  ( A B void -- void )
	case DROP3_RET: ip = popr;
	case DROP3:		sp += 3; loop;
	case DROPN_RET: ip = popr;
	case DROPN:		sp += N; loop;
	
	// Conversion:

	case ITOi8:		top = int8(top); loop;			// int32 to smaller int
	case ITOi16:	top = int16(top); loop;
	case ITOu8:		top = uint8(top); loop;			// uint32 to smaller int
	case ITOu16:	top = uint16(top); loop;
	case ITOF:		top = float(top.i32); loop;		// float <-> int
	case FTOI:		top = int32(top.f32); loop;
	case UTOF:		top = float(top.u32); loop;
	case FTOU:		top = uint32(top.f32); loop;
	case ITObool:	top = top != 0; loop;			//  ( int -- bool )
	case FTObool:	top = top != 0.f; loop;			//  ( float -- bool )
	
	case _filler1:
	case _filler2:
	case _filler3:
	case _filler4:
	case _filler5:
	case _filler6:
	case _filler7:
	case _filler8:
	case _filler9: IERR();
		
	case EXIT:
		static_assert(EXIT == 255);
		delete[] rp;
		return top;


#if VCC_LONG || VCC_VARIADIC
		
	// size = 8:	
	// TODO:
	case PUSH_GGETl:		  push(top);			//  ( -- value )
	case GGETl: top = ram[U]; loop;	//  ( void -- value )
	case PUSH_GSETl:		  ram[U] = top; loop;	//  ( value -- void )
	case PUSH_LGETl:		  push(top);			//  ( -- value )
	case LGETl: top = sp[U]; loop;	//  ( void -- value )
	case PUSH_LSETl:		  sp[U] = top; loop;	//  ( value -- void )
	case IGETl:	  top = top.i32ptr[U]; loop; //  ( addr -- value )
	case ISETl:    top.i32ptr[U] = pop; loop; //  ( value addr U -- void )
	case ATIl:	    top.i32ptr += pop; loop;	 //  ( idx addr -- addr )
	case ATIGETl:    top = top.i32ptr[pop]; loop;
	case ATISETl: 	{ uint idx = pop; top.i32ptr[idx] = pop; } loop;
	case PEEKl:		top = *top.i32ptr; loop; 	// peek ( addr -- value )
	case POKEl:  	*top.i32ptr = pop; loop;	// poke ( value addr -- void )

	default:
//		r = new Exception(illegal_opcode,ip);
//		if (tp) goto _throw;
//		else 
			break;

#endif 		

#if VCC_LONG 
		
	// INT64 and UINT64
	// TODO
	case ADDl:	top = pop.i32 + top.i32; loop;	//  ( A B -- A+B )
	case SUBl:	top = pop.i32 - top.i32; loop;	//  ( A B -- A-B )
	case MULl:	top = pop.i32 * top.i32; loop;
	case DIVl:	top = pop.i32 / top.i32; loop;	//  ( A B -- A/B )
	case DIVul:	top = pop.u32 / top.u32; loop;
	case MODl:	top = pop.i32 % top.i32; loop;
	case MODul:	top = pop.u32 % top.u32; loop;
	case ANDl:	top = pop.u32 & top.u32; loop;	//  ( A B -- A&B )
	case ORl: 	top = pop.u32 | top.u32; loop;	//  ( A B -- A|B )
	case XORl:	top = pop.u32 ^ top.u32; loop;	//  ( A B -- A^B )
	case SLl:	top = pop.u32 << top.i32; loop; //  ( A B -- A<<B )
	case SRl: 	top = pop.i32 >> top.i32; loop; //  ( A B -- A>>B )
	case SRul:	top = pop.u32 >> top.i32; loop; //  ( A B -- A>>B )
		
	//case ADDIl:	top += N; loop;			//  ( N -- N )
	//case MULIl:	top *= N; loop;
	//case DIVIl:	top /= N; loop;
	//case DIVIul:	top /= U; loop;
	//case ANDIl:	top &= U; loop;
	//case ORIl:	top |= U; loop;
	//case XORIl:	top ^= U; loop;
	//case SLIl:	top.u32 <<= U; loop;
	//case SRIl:	top.u32 >>= U; loop;
	//case SRIul:	top.i32 >>= U; loop;

	//case ADD1l:	top += 1; loop;			//  ( N -- N )
	//case ADD2l:	top += 2; loop; 
	//case ADD4l:	top += 4; loop; 
	//case ADD8l:	top += 8; loop; 
	//case SUB1l:	top -= 1; loop; 
	//case SUB2l:	top -= 2; loop; 
	//case SUB4l:	top -= 4; loop; 
	//case SUB8l:	top -= 8; loop; 
		
	//case SL1l:	top.u32 <<= 1; loop;	//  ( N -- N )
	//case SL2l:	top.u32 <<= 2; loop;
	//case SL3l:	top.u32 <<= 3; loop;
	//case SL4l:	top.u32 <<= 4; loop;
	//case SR1l:	top.i32 >>= 1; loop;
	//case SR2l:	top.i32 >>= 2; loop;
	//case SR3l:	top.i32 >>= 3; loop;
	//case SR4l:	top.i32 >>= 4; loop;		
	//case SR1ul:	top.u32 >>= 1; loop;
	//case SR2ul:	top.u32 >>= 2; loop;
	//case SR3ul:	top.u32 >>= 3; loop;
	//case SR4ul:	top.u32 >>= 4; loop;
		
	case CPLl:	top = ~top; loop;			//  ( N -- N )
	case NEGl:	top = -top; loop;			//  ( N -- N )
	case NOTl:	top = !top.i32; loop;		//  ( N -- N )
	case ABSl:	top = abs(top.i32); loop;	//  ( N -- N )
	case SIGNl:	top = sign(top.i32); loop;	//  ( N -- N )

	case MINl:	top = min(top.i32,pop.i32); loop;
	case MINul:	top = min(top.u32,pop.u32); loop;
	case MAXl:	top = max(top.i32,pop.i32); loop;
	case MAXul:	top = max(top.u32,pop.u32); loop;
	//case RANDOMul: push(top); top = rng.random(top.u32); loop;	//  ( void -- N )	[0 .. [max
	//case NOW:	push(top); top = now<time_t>(); loop;			// seconds since 00:00 hours, Jan 1, 1970 UTC

	case EQl:  	top = pop.i32 == top.i32; loop; //  ( A B -- A==B )
	case NEl:  	top = pop.i32 != top.i32; loop; //  ( A B -- A!=B )
	case LTl:  	top = pop.i32 <  top.i32; loop; //  ( A B -- A<B )
	case LEl:  	top = pop.i32 <= top.i32; loop; //  ( A B -- A<=B )
	case GTl:  	top = pop.i32 >  top.i32; loop; //  ( A B -- A>B )
	case GEl:  	top = pop.i32 >= top.i32; loop; //  ( A B -- A>=B )
	case LTul: 	top = pop.u32 <  top.u32; loop; //  ( A B -- A<B )
	case LEul: 	top = pop.u32 <= top.u32; loop; //  ( A B -- A<=B )
	case GTul: 	top = pop.u32 >  top.u32; loop; //  ( A B -- A>B )
	case GEul: 	top = pop.u32 >= top.u32; loop; //  ( A B -- A>=B )

	// DOUBLE
	// TODO
		
	case ADDd:	top = pop.f32 + top.f32; loop;	 //  ( A B -- A+B )
	case SUBd:	top = pop.f32 - top.f32; loop;	 //  ( A B -- A-B )
	case MULd:	top = pop.f32 * top.f32; loop;	 //  ( A B -- A*B )
	case DIVd:	top = pop.f32 / top.f32; loop;	 //  ( A B -- A/B )
	case SLd:	top = ldexp(pop.f32,+top.i32); loop; //  ( A B -- A<<B )
	case SRd:	top = ldexp(pop.f32,-top.i32); loop; //  ( A B -- A>>B )
	//case INCd:	top.f32++; loop;					 //  ( value -- value )
	//case DECd:	top.f32--; loop;
	//case NOTd:	top = !top.f32; loop;			 //  ( f32 -- i32 )
	//case NEGd:	top = -top.f32; loop;
	//case ABSd:	top = fabs(top.f32); loop;
	//case SIGNd:	top = sign(top.f32); loop;		 //  ( float -- int )

	//case SINd:	top = sin(top.f32); loop;
	//case COSd:	top = cos(top.f32); loop;
	//case TANd:	top = tan(top.f32); loop;
	//case ASINd:	top = asin(top.f32); loop;
	//case ACOSd:	top = acos(top.f32); loop;
	//case ATANd:	top = atan(top.f32); loop;

	//case SINHd:  top = sinh(top.f32); loop;
	//case COSHd:  top = cosh(top.f32); loop;
	//case TANHd:  top = tanh(top.f32); loop;
	//case ASINHd: top = asinh(top.f32); loop;
	//case ACOSHd: top = acosh(top.f32); loop;
	//case ATANHd: top = atanh(top.f32); loop;

	//case LOGEd:	top = log(top.f32); loop;				 //  ( f32 -- f32 )
	//case LOG10d:	top = log10(top.f32); loop;				 //	 ( f32 -- f32 )		ln(top) / ln(10)
	//case LOG2d:	top = log2(top.f32); loop;				 //  ( f32 -- f32 )		ln(top) / ln(2)
	//case LOGd:	top = log(top.f32) / log(pop.f32); loop; //  ( E N -- logE(N) )	log(n,e) = ln(n) / ln(e)

	//case EXPEd:	top = exp(top.f32); loop;				 //  ( f32 -- f32 )
	//case EXP2d:	top = exp2(top.f32); loop;				 //  ( f32 -- f32 )		exp(top) * ln(2)
	//case EXP10d:	top = exp(top.f32) * log(10.0f); loop;	 //  ( f32 -- f32 )		exp10(N) is only defined as double
	//case EXPd:	top = exp(pop.f32) * log(top.f32); loop; //  ( E N -- expE(N) )	exp(n,e) = exp(n) * ln(e)

	//case SQRTd:	top = sqrt(top.f32); loop;				 //  ( f32 -- f32 )
	//case FLOORd:	top = floor(top.f32); loop;				 //  ( f32 -- f32 )
	//case ROUNDd:	top = round(top.f32); loop;				 //  ( f32 -- f32 )
	//case CEILd:	top = ceil(top.f32); loop;				 //  ( f32 -- f32 )
		
	//case FRACTd:		{ float z; top = modf(top.f32, &z); loop; }	//  ( N -- N%1 )
	//case INTEGd:		{ modf(top.f32, &top.f32); loop; }			//  ( N -- N/1 )
	//case MANTISSAd:	{ int i; top = frexp(top.f32, &i); loop; }	//  ( f32 -- i32 ) top!=0 --> 0.5 <= top < 1.0
	//case EXPONENTd:	{ frexp(top.f32, &top.i32); loop; }			//  ( f32 -- i32 ) 
	//case COMPOSEd:   { top = ldexp(pop.f32, top.i32); loop; }	//  ( f32 E:i32 -- f32 )

	//case MINd:		top = min(top.f32,pop.f32); loop;
	//case MAXd:		top = max(top.f32,pop.f32); loop;
	//case RANDOMd1:	push(top); top = rng.random(); loop;	//  ( void -- f32 )		[0 .. [1
	//case RANDOMd:	top = rng.random(top.f32); loop;		//  ( f32 -- f32 )		[0 .. [max
	//case NOWd:  	push(top); top = now(); loop;			// seconds since 00:00 hours, Jan 1, 1970 UTC

	case EQd:  	top = pop.f32 == top.f32; loop;		//  ( A B -- A==B )
	case NEd:  	top = pop.f32 != top.f32; loop;		//  ( A B -- A!=B )
	case LTd:  	top = pop.f32 <  top.f32; loop;		//  ( A B -- A<B )
	case LEd:  	top = pop.f32 <= top.f32; loop;		//  ( A B -- A<=B )
	case GTd:  	top = pop.f32 >  top.f32; loop;		//  ( A B -- A>B )
	case GEd:  	top = pop.f32 >= top.f32; loop;		//  ( A B -- A>=B )

#endif 		

#if VCC_VARIADIC 
		
	// VARIADIC
	// TODO
	//case ATIGETuxx:
	//	switch (r.memptr->type.itemSizeShift())
	//	{
	//	case 0:  goto ATIGETu8;
	//	case 1:  goto ATIGETu16;
	//	case 2:  goto ATIGETu32;
	//	default: IERR();
	//	}

	//case ATIGETixx:
	//	switch (r.memptr->type.itemSizeShift())
	//	{
	//	case 0:  goto ATIGETi8;
	//	case 1:  goto ATIGETi16;
	//	case 2:  goto ATIGETi32;
	//	default: IERR();
	//	}

	//case ATISETixx:
	//	switch (r.memptr->type.itemSizeShift())
	//	{
	//	case 0: goto ATISET8;
	//	case 1: goto ATISET16;
	//	case 2: goto ATISET32;
	//	default: IERR();
	//	}

#endif 
	
	// Objects, Strings and Arrays:

	//case NEW_OBJECT:	r = newObject(ClassID(U)); loop;	// TODO: newObject(ClassID,size)
	//case NEW_ARRAY:		r = newArray(ClassID(U),r); loop;	// TODO: newArray.8,16,32,64,128(TypeID,count)
	//case SETCONST:			MemoryPtr(r)->setConst(); loop;

	//case COUNT:			r = count(r); loop;					// TODO: count.8,16,32,64,128()

	//case CLONE:			r = clone(r); loop;
	//case CONCAT:		r = concat(r,POP); loop;				// TODO: cat.8,16,32,64,128()
	//case CONCATu:		r = concat_u(r,POP); loop;
	//case CONCATs:		r = concat_s(r,POP); loop;
	//case MIDRANGE: 		r = range(r,*sp,*(sp+1)); sp+=2; loop;	// signed indexes
	//case LEFTRANGE:		r = leftrange(r,POP); loop;				// ""
	//case RIGHTRANGE:	r = rightrange(r,POP); loop;			// ""

	//case ARGV:			r = arguments; loop;				// TODO in globals?
	//case ENVIRON:		r = getEnvironStringList(); loop;	// TODO in globals?

	//case WAIT:			TODO(); // wait for signal
	//case WAIT_UNTIL:	TODO(); // wait until time in top
	//case WAIT_DELAY:	TODO(); // wait delay in top

	//case RUN_GC:		TODO();

	//case CALL_VFUNC:	TODO(); // call virtual member function

	}

//	}
//	catch (Exception* m)
//	{
//		m->ip = ip;
//		r = m;
//		if (tp) goto _throw;
//	}
//	catch (Memory* m)
//	{
//		r = m;
//		if (tp) goto _throw;
//	}
//	catch (std::exception& e)
//	{
//		r = new Exception(newString(e.what()),ip);
//		if (tp) goto _throw;
//	}
//	catch (...)
//	{
//		r = new Exception(unknown_exception,ip);
//		if (tp) goto _throw;
//	}
//
//
//// unhandled exception --> throw
//_throw_unhandled:
//
//	assert (tp == nullptr);
//
//	if (r.memptr->isArrayOfChar())
//	{
//		Array* a = reinterpret_cast<Array*>(r.memptr);
//		throw AnyError("unhandled exception: %s", tostr(a));
//	}
//	else if (r.memptr->isa(ExceptionClassID))
//	{
//		Exception* e = reinterpret_cast<Exception*>(r.memptr);
//		throw AnyError("unhandled exception: %s", tostr(e->msg));
//	}
//	else
//	{
//		throw AnyError("unhandled exception");
//	}
	
	return 666;
}

#pragma GCC diagnostic pop


}

