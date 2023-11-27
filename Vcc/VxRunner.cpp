// Copyright (c) 2023 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "VxRunner.h"
#include "DReg.h"
#include "Var.h"
#include "Xoshiro128.h"
#include "kilipili_cdefs.h"
#include "standard_types.h"
#include "string.h"
#include <math.h>
#include <memory>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <pico/time.h>


namespace kio::Vcc
{

static Xoshiro128 rng {time_us_32()};

using VxOpcode	  = const void*;
using VxOpcodePtr = VxOpcode*;

VxOpcode vx_opcodes[num_VxOpcodes];

bool vx_exit[2];


Var* execute(Var* ram, const VxOpcode* ip, uint stack_size, uint32 timeout_ms)
{
	std::unique_ptr<Var[]> stack {new Var[stack_size]};

	const VxOpcode** rp = reinterpret_cast<const VxOpcode**>(stack.get());
	Var*			 sp = &stack[stack_size];

	const VxOpcode opcode_exit = vx_opcodes[EXIT];
	*rp++					   = &opcode_exit;

	if (timeout_ms)
		add_alarm_in_ms(
			timeout_ms,
			[](alarm_id_t, void* flag) {
				*ptr(flag) = true;
				return 0ll;
			},
			&vx_exit[get_core_num()], false);

	return execute(ram, ip, rp, sp);
}


// clang-format off
	
Var* execute(Var* ram, const VxOpcode* ip, const VxOpcode** rp, Var* sp)
{
	// ram: base address for global variables
	// ip:  start address
	// rp:  return stack pointer    (grows up)
	// sp:  variables stack pointer	(grows down)
	//
	// if ip==NULL then initialize vx_opcodes[] table 
	//
	// runs until EXIT is executed or global bool vx_exit is set.
	
	if (sp == nullptr)
	{
		VxOpcode opcodes[] =
		{
		#define M(A,B,C) &&A
		#include "Opcodes.h"
		};
		
		static_assert(NELEM(opcodes)==NELEM(vx_opcodes));		
		memcpy(vx_opcodes,opcodes,sizeof(opcodes));
		return nullptr;
	}

#define push(X)  *--sp = (X)
#define pop		 (*sp++)
#define N		 *intptr(ip++)
#define U		 *uintptr(ip++)
#define gvar	 VarPtr(size_t(ram)+(U))
#define jp()	 ip = VxOpcodePtr(*intptr(ip))
#define jr()	 ip = VxOpcodePtr(ssize_t(ip) + *intptr(ip))
#define pushr(X) *rp++ = (X)
#define popr     *--rp	
#define loop	 goto *ip++
#define vx_loop	 do { if likely(!vx_exit) loop; else goto yield; } while(0)

	DReg top;	
	
	if (ip == nullptr)
	{
		top = pop;
		rp = pop;
		ip = pop;
	}	
	
	bool& vx_exit = Vcc::vx_exit[get_core_num()];
	vx_exit = false;	

		
	NOP:		loop;
	EXIT:		ip--;
	yield:		pushr(ip); push(rp); 	// ( var -- void ) 
				push(top); return sp;
		
	PUSH:		push(top); loop;		// technically this is DUP
	POP:		top = pop.u32; loop;	// technically this is DROP

	// Numeric Literals:

	// technically these are not PUSH because they ignore the TOP value:
	// used between instructions to create new variables.
	PUSH4x0:	push(0);				//  ( void -- 0 0 0 0 void )	
	PUSH3x0:	push(0);				//  ( void -- 0 0 0 void )
	PUSH2x0:	push(0);				//  ( void -- 0 0 void )
	PUSH0:		push(0); loop;			//  ( void -- 0 void )

	// immediate value: 
	// note: larger constants are stored in globals[]
	PUSH_IVAL:	push(top);				//  ( -- int )
	IVAL:		top = N; loop;			//  ( void -- int )

	// Global Variables in gvar[]:

	PUSH_GVAR:	push(top);				//  ( -- addr )
	GVAR:		top = gvar; loop;		//  ( void -- addr )
	PUSH_GGET:	push(top);				//  ( -- value )
	GGET:		top = *gvar; loop;		//  ( void -- value )
	GSET:		ram[U] = top; loop;		//  ( value -- void )
	
	// Local Variables on Stack:

	PUSH_LVAR:	push(top);				//  ( -- addr )
	LVAR:		top = &sp[N]; loop;		//  ( void -- addr )
	PUSH_LGET:	push(top);				//  ( -- value )
	LGET:	    top = sp[N]; loop;		//  ( void -- value )
	LSET:		sp[N] = top; loop;		//  ( value -- void )

	// Items in Objects:
	// offset and data alignment depend on data size!
	// => also usable for ati[N] with fixed index N

	IVAR:		top.i32ptr += U;	 loop;	// ( addr -- addr )
	IVAR8:		top.i8ptr  += U;	 loop;
	IVAR16:		top.i16ptr += U;	 loop;
	IGET:		top = top.i32ptr[U]; loop;	// ( addr -- value )
	IGETi8:		top = top.i8ptr[U];  loop;
	IGETi16:	top = top.i16ptr[U]; loop;
	IGETu8:		top = top.u8ptr[U];  loop;
	IGETu16:	top = top.u16ptr[U]; loop;
	ISET:		top.i32ptr[U] = pop; loop;	// ( value addr U -- void )
	ISET8:		top.i8ptr [U] = pop; loop;
	ISET16:		top.i16ptr[U] = pop; loop;

	// Items in Arrays:

	ATI:		top.i32ptr += pop;	   loop;	// ( idx addr -- addr )
	ATI8:		top.i8ptr  += pop;	   loop;
	ATI16:		top.i16ptr += pop;	   loop;
	ATIGET:		top = top.i32ptr[pop]; loop;	// ( idx addr -- value )
	ATIGETi8:	top = top.i8ptr [pop]; loop;
	ATIGETi16:	top = top.i16ptr[pop]; loop;
	ATIGETu8:	top = top.u8ptr [pop]; loop;
	ATIGETu16:	top = top.u16ptr[pop]; loop;
	ATISET: 	{ uint idx = pop; top.i32ptr[idx] = pop; } loop;
	ATISET8:	{ uint idx = pop; top.i8ptr [idx] = pop; } loop; //  ( value idx addr -- )
	ATISET16:	{ uint idx = pop; top.i16ptr[idx] = pop; } loop;
		
	// get / set arbitrary address:

	PEEK:		top = *top.i32ptr; loop; 	// peek ( addr -- value )
	PEEKi8:		top = *top.i8ptr;  loop;
	PEEKu8:		top = *top.u8ptr;  loop;
	PEEKi16:	top = *top.i16ptr; loop;
	PEEKu16:	top = *top.u16ptr; loop;
	POKE:		*top.i32ptr = pop; loop;	// poke ( value addr -- void )
	POKE8:		*top.u8ptr  = pop; loop;
	POKE16:		*top.u16ptr = pop; loop;

	// Integer Arithmetics:

	ADD:	top.i32  +=  pop.i32; loop;	//  ( A B -- B+A )
	SUB:	top.i32  -=  pop.i32; loop;	//  ( A B -- B-A )
	MUL:	top.i32  *=  pop.i32; loop;	//  ( A B -- B*A )
	DIV:	top.i32  /=  pop.i32; loop;	//  ( A B -- B/A )
	DIVu: 	top.u32  /=  pop.u32; loop;	//  ( A B -- B/A )
	MOD:	top.i32  %=  pop.i32; loop;	//  ( A B -- B%A )
	MODu: 	top.u32  %=  pop.u32; loop;	//  ( A B -- B%A )
	AND:	top.u32  &=  pop.u32; loop;	//  ( A B -- B&A )
	OR: 	top.u32  |=  pop.u32; loop;	//  ( A B -- B|A )
	XOR:	top.u32  ^=  pop.u32; loop;	//  ( A B -- B^A )
	SL:		top.u32 <<=  pop.u32; loop; //  ( A B -- B<<A )
	SR: 	top.i32 >>=  pop.u32; loop; //  ( A B -- B>>A )
	SRu:	top.u32 >>=  pop.u32; loop; //  ( A B -- B>>A )

	ADDI:	top += N; loop;				//  ( N -- N )
	MULI:	top *= N; loop;
	DIVI:	top /= N; loop;
	DIVIu:	top /= U; loop;
	ANDI:	top &= U; loop;
	ORI:	top |= U; loop;
	XORI:	top ^= U; loop;
	SLI:	top.u32 <<= U; loop;
	SRI:	top.u32 >>= U; loop;
	SRIu:	top.i32 >>= U; loop;

	ADD1:	top += 1; loop;				//  ( N -- N )
	ADD2:	top += 2; loop; 
	SUB1:	top -= 1; loop; 
	SUB2:	top -= 2; loop; 
		
	SL1:	top.u32 <<= 1; loop;		//  ( N -- N )
	SL2:	top.u32 <<= 2; loop;
	SR1:	top.i32 >>= 1; loop;
	SR2:	top.i32 >>= 2; loop;
	SR1u:	top.u32 >>= 1; loop;
	SR2u:	top.u32 >>= 2; loop;

	CPL:	top = ~top; loop;			//  ( N -- N )
	NEG:	top = -top; loop;			//  ( N -- N )
	NOT:	top = !top.i32; loop;		//  ( N -- N )
	ABS:	top = abs(top.i32); loop;	//  ( N -- N )
	SIGN:	top = sign(top.i32); loop;	//  ( N -- N )

	MIN:	top = min(top.i32,pop.i32); loop;
	MINu:	top = min(top.u32,pop.u32); loop;
	MAX:	top = max(top.i32,pop.i32); loop;
	MAXu:	top = max(top.u32,pop.u32); loop;
	RANDOMu: top = rng.random(top.u32); loop;	//  ( void -- N )	[0 .. [max
	//NOW:	top = now<time_t>(); loop;			// seconds since 00:00 hours, Jan 1, 1970 UTC

	EQ:  	top = top.i32 == pop.i32; loop; //  ( A B -- B==A )
	NE:  	top = top.i32 != pop.i32; loop; //  ( A B -- B!=A )
	LT:  	top = top.i32 <  pop.i32; loop; //  ( A B -- B< A )
	LE:  	top = top.i32 <= pop.i32; loop; //  ( A B -- B<=A )
	GT:  	top = top.i32 >  pop.i32; loop; //  ( A B -- B> A )
	GE:  	top = top.i32 >= pop.i32; loop; //  ( A B -- B>=A )
	LTu:  	top = top.u32 <  pop.u32; loop; //  ( A B -- B< A )
	LEu:  	top = top.u32 <= pop.u32; loop; //  ( A B -- B<=A )
	GTu:  	top = top.u32 >  pop.u32; loop; //  ( A B -- B> A )
	GEu:  	top = top.u32 >= pop.u32; loop; //  ( A B -- B>=A )
		
	ADDGL:	*top.i32ptr += pop.i32; loop;
	SUBGL:	*top.i32ptr -= pop.i32; loop; 
	MULGL:	*top.i32ptr *= pop.i32; loop; 
	DIVGL:	*top.i32ptr /= pop.i32; loop; 
	DIVGLu:	*top.u32ptr /= pop.u32; loop;
	ANDGL:	*top.i32ptr &= pop.i32; loop; 
	ORGL:	*top.i32ptr |= pop.i32; loop; 
	XORGL:	*top.i32ptr ^= pop.i32; loop;
	SLGL:	*top.i32ptr <<= pop.i32; loop;
	SRGL:	*top.i32ptr >>= pop.i32; loop;
	SRGLu:	*top.u32ptr >>= pop.i32; loop;
	INCR:	(*top.i32ptr)++; loop;
	DECR:	(*top.i32ptr)--; loop;
				
	ADDGLs: *top.i16ptr += pop.i32; loop;
	SUBGLs: *top.i16ptr -= pop.i32; loop;
	ANDGLs: *top.i16ptr &= pop.i32; loop;
	ORGLs:	*top.i16ptr |= pop.i32; loop;
	XORGLs: *top.i16ptr ^= pop.i32; loop;
	INCRs:	(*top.i16ptr)++; loop;
	DECRs:	(*top.i16ptr)--; loop;
		
	ADDGLb: *top.i8ptr += pop.i32; loop;
	SUBGLb: *top.i8ptr -= pop.i32; loop;
	ANDGLb: *top.i8ptr &= pop.i32; loop;
	ORGLb:	*top.i8ptr |= pop.i32; loop;
	XORGLb: *top.i8ptr ^= pop.i32; loop;
	INCRb:	(*top.i8ptr)++; loop;
	DECRb:	(*top.i8ptr)--; loop;
		
	// Floating Point Arithmetics:

	ADDf:	top = top.f32 + pop.f32; loop;		//  ( A B -- B+A )
	SUBf:	top = top.f32 - pop.f32; loop;		//  ( A B -- B-A )
	MULf:	top = top.f32 * pop.f32; loop;		//  ( A B -- B*A )
	DIVf:	top = top.f32 / pop.f32; loop;		//  ( A B -- B/A )
	SLf:	top = ldexp(top.f32,+pop.i32); loop;//  ( A B -- B<<A )
	SRf:	top = ldexp(top.f32,-pop.i32); loop;//  ( A B -- B>>A )
	ADD1f:	top.f32++; loop;					//  ( value -- value )
	SUB1f:	top.f32--; loop;
	NOTf:	top = top.f32 == 0.0f; loop;		//  ( f32 -- i32 )
	NEGf:	top = -top.f32; loop;
	ABSf:	top = fabs(top.f32); loop;
	SIGNf:	top = sign(top.f32); loop;			//  ( float -- int )

	SIN:	top = sin(top.f32); loop;
	COS:	top = cos(top.f32); loop;
	TAN:	top = tan(top.f32); loop;
	ASIN:	top = asin(top.f32); loop;
	ACOS:	top = acos(top.f32); loop;
	ATAN:	top = atan(top.f32); loop;

	SINH:	top = sinh(top.f32); loop;
	COSH:	top = cosh(top.f32); loop;
	TANH:	top = tanh(top.f32); loop;
	ASINH:	top = asinh(top.f32); loop;
	ACOSH:	top = acosh(top.f32); loop;
	ATANH:	top = atanh(top.f32); loop;

	LOGE:	top = log(top.f32); loop;				 //  ( f32 -- f32 )
	LOG10:	top = log10(top.f32); loop;				 //	 ( f32 -- f32 )		ln(top) / ln(10)
	LOG2:	top = log2(top.f32); loop;				 //  ( f32 -- f32 )		ln(top) / ln(2)
	LOG:	top = log(pop.f32) / log(top.f32); loop; //  ( N E -- logE(N) )	log(e,n) = ln(n) / ln(e)

	EXPE:	top = exp(top.f32); loop;				 //  ( f32 -- f32 )
	EXP2:	top = exp2(top.f32); loop;				 //  ( f32 -- f32 )		exp(top) * ln(2)
	EXP10:	top = exp(top.f32) * log(10.0f); loop;	 //  ( f32 -- f32 )		exp10(N) is only defined as double
	EXP:	top = exp(pop.f32) * log(top.f32); loop; //  ( N E -- expE(N) )	exp(e,n) = exp(n) * ln(e)

	SQRT:	top = sqrt(top.f32); loop;				 //  ( f32 -- f32 )
	FLOOR:	top = floor(top.f32); loop;				 //  ( f32 -- f32 )
	ROUND:	top = round(top.f32); loop;				 //  ( f32 -- f32 )
	CEIL:	top = ceil(top.f32); loop;				 //  ( f32 -- f32 )
		
	FRACT:		{ float z; top = modf(top.f32, &z); loop; }	//  ( N -- N%1 )
	INTEG:		{ modf(top.f32, &top.f32); loop; }			//  ( N -- N/1 )
	MANTISSA:	{ int i; top = frexp(top.f32, &i); loop; }	//  ( f32 -- i32 ) top!=0 --> 0.5 <= top < 1.0
	EXPONENT:	{ frexp(top.f32, &top.i32); loop; }			//  ( f32 -- i32 ) 
	COMPOSE:	{ top = ldexp(top.f32, pop.i32); loop; }	//  ( E:i32 f32 -- f32 )  compose(mant,exp)

	MINf:		top = min(top.f32,pop.f32); loop;
	MAXf:		top = max(top.f32,pop.f32); loop;
	RANDOMf1:	top = rng.random(); loop;			//  ( void -- f32 )		[0 .. [1
	RANDOMf:	top = rng.random(top.f32); loop;	//  ( f32 -- f32 )		[0 .. [max
	//NOWf:  	top = now(); loop;					// seconds since 00:00 hours, Jan 1, 1970 UTC

	EQf:  	top = top.f32 == pop.f32; loop;		//  ( A B -- B==A )
	NEf:  	top = top.f32 != pop.f32; loop;		//  ( A B -- B!=A )
	LTf:  	top = top.f32 <  pop.f32; loop;		//  ( A B -- B< A )
	LEf:  	top = top.f32 <= pop.f32; loop;		//  ( A B -- B<=A )
	GTf:  	top = top.f32 >  pop.f32; loop;		//  ( A B -- B> A )
	GEf:  	top = top.f32 >= pop.f32; loop;		//  ( A B -- B>=A )

	ADDGLf: *top.f32ptr += pop.f32; loop;
	SUBGLf: *top.f32ptr -= pop.f32; loop;
	MULGLf: *top.f32ptr *= pop.f32; loop;
	DIVGLf: *top.f32ptr /= pop.f32; loop;
	INCRf:  (*top.f32ptr)++; loop;
	DECRf:  (*top.f32ptr)--; loop;
		
	// Flow Control:

	JZ: 	if (top.i32 == 0)	jr(); else ip++; vx_loop;  // ( N -- void )
	JNZ:	if (top.i32 != 0)	jr(); else ip++; vx_loop;  // ( N -- void )
	JEQ:	if (top == pop.u32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JNE:	if (top != pop.u32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JLT:	if (top <  pop.i32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JLTu:	if (top <  pop.u32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JLE:	if (top <= pop.i32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JLEu:	if (top <= pop.u32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JGE:	if (top >= pop.i32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JGEu:	if (top >= pop.u32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JGT:	if (top >  pop.i32) jr(); else ip++; vx_loop;  // ( A B -- void )
	JGTu:	if (top >  pop.u32) jr(); else ip++; vx_loop;  // ( A B -- void )

	JEQI:	if (top == N) jr(); else ip++; vx_loop; 		// ( N -- void )
	JNEI:	if (top != N) jr(); else ip++; vx_loop; 		// ( N -- void )
	JLTI:	if (top <  N) jr(); else ip++; vx_loop; 		// ( N -- void )
	JLTIu:	if (top <  U) jr(); else ip++; vx_loop; 		// ( N -- void )
	JLEI:	if (top <= N) jr(); else ip++; vx_loop; 		// ( N -- void )
	JLEIu:	if (top <= U) jr(); else ip++; vx_loop; 		// ( N -- void )
	JGEI:	if (top >= N) jr(); else ip++; vx_loop; 		// ( N -- void )
	JGEIu:	if (top >= U) jr(); else ip++; vx_loop; 		// ( N -- void )
	JGTI:	if (top >  N) jr(); else ip++; vx_loop; 		// ( N -- void )
	JGTIu:	if (top >  U) jr(); else ip++; vx_loop; 		// ( N -- void )
		
	JZf:	if (top == 0.0f)	jr(); else ip++; vx_loop;	// ( N -- void )
	JNZf:	if (top != 0.0f)	jr(); else ip++; vx_loop;	// ( N -- void )
	JEQf:	if (top == pop.f32)	jr(); else ip++; vx_loop;	// ( A B -- void )
	JNEf:	if (top != pop.f32)	jr(); else ip++; vx_loop;	// ( A B -- void )
	JLTf:	if (top <  pop.f32)	jr(); else ip++; vx_loop;	// ( A B -- void )
	JLEf:	if (top <= pop.f32)	jr(); else ip++; vx_loop;	// ( A B -- void )
	JGEf:	if (top >= pop.f32)	jr(); else ip++; vx_loop;	// ( A B -- void )
	JGTf:	if (top >  pop.f32)	jr(); else ip++; vx_loop;	// ( A B -- void )
 
	BSR:	pushr(ip + 1);
	JR:		jr(); loop;
	
	CALL:	pushr(ip); ip = top; top = pop; loop;	
	JSR:	pushr(ip + 1);	
	JP:		jp(); loop;		
	RET:	ip = popr; loop;

	SWITCH: top.u32 = min(top.u32, U);	// ( value -- void )	 SWITCH, max, dist[max], dflt, code…
			ip += top.u32; jr(); loop;

	TRY:	TODO();
	THROW:	TODO();
	TRYEND: TODO();
	CATCH:	TODO();
		
	// technically these are not DROP, because they ignore the TOP value
	// used between instructions to destroy variables and before return.
	DROP_RET:	ip = popr;		//  ( X rval -- rval )		note: rval may be void
	DROP:		sp += 1; loop;	//  ( X void -- void )
	DROP2_RET:	ip = popr;		//  ( A B rval -- rval )	note: rval may be void
	DROP2:		sp += 2; loop;	//  ( A B void -- void )
	DROP3_RET:	ip = popr;
	DROP3:		sp += 3; loop;
	DROPN_RET:	ip = popr;
	DROPN:		sp += N; loop;
	
	// Conversion:

	ITOi8:		top = int8(top); loop;			// int32 to smaller int
	ITOi16:		top = int16(top); loop;
	ITOu8:		top = uint8(top); loop;			// uint32 to smaller int
	ITOu16:		top = uint16(top); loop;
	ITOF:		top = float(top.i32); loop;		// float <-> int
	FTOI:		top = int32(top.f32); loop;
	UTOF:		top = float(top.u32); loop;
	FTOU:		top = uint32(top.f32); loop;
	ITObool:	top = top != 0; loop;			//  ( int -- bool )
	FTObool:	top = top != 0.f; loop;			//  ( float -- bool )
}

// clang-format on

} // namespace kio::Vcc

/*



































*/
