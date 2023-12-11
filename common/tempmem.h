// Copyright (c) 2008 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
#include "no_copy_move.h"
#include "standard_types.h"


/*	Temporary Memory Pool
	=====================
	
	TempMem provides fast memory for temporary c-style strings and return values
	which can be used like static c-strings and text literals.
	
	In addition to c-strings any small data with no destructor can be allocated in TempMem.
	
	For an example see `cstrings.h|.cpp`, the primary use case.

	• TempMem should not be used for long living values,
	  only for stack (automatic) strings and return values.

	• Pools are thread local and thus thread safe.
	• Pools are created and destroyed automatically.
	• Local pools can be used to bulk-erase locally created temp strings.
	
	• Short running programs (e.g. a scripts or a cgi handlers) can simply forget 
	  about purging the accumulating waste memory. 
	• Long running programs should purge the tempmem regularly at a low function level.		 
	  This may also be accomplished by using short running worker threads.
	• Functions which use a lot of tempmem should create a local TempMem pool.
	  In this case use xdupstr() to copy return values into the surrounding pool.
*/


namespace kio
{

extern str emptystr; // non-const version of ""

extern str	newstr(uint n);			  // allocate memory with new[]
extern str	newcopy(cstr);			  // allocate memory with new[] and copy string
extern str	tempstr(uint len);		  // allocate in current pool: 0-terminated, not cleared
extern str	dupstr(cstr);			  // allocate in current pool and copy string
extern ptr	tempmem(uint size);		  // allocate in current pool: aligned, not cleared
extern void purge_tempmem() noexcept; // purge the current pool

// note: only use the outer pool to return results from a function with a local pool.
extern str	xtempstr(uint len);	 // allocate in outer pool: 0-terminated, not cleared
extern cstr xdupstr(cstr);		 // allocate in outer pool and copy string
extern ptr	xtempmem(uint size); // allocate in outer pool: aligned, not cleared


/*
	Class to create and auto-destruct a local TempMem pool:
*/
class TempMem
{
	NO_COPY_MOVE(TempMem);

public:
	TempMem(uint size = 0);
	~TempMem() noexcept;
	static void purge() noexcept { purge_tempmem(); }
};


} // namespace kio


/*






































*/
