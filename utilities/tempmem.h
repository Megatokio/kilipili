// Copyright (c) 2008 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "no_copy_move.h"
#include "standard_types.h"


/*	Temporary Memory Pool
	=====================
	
	TempMem provides 
		• fast memory for temporary c-strings 
		• which can be used like static c-strings and text literals.
	
	In addition to c-strings any small data with no destructor can be allocated in TempMem.	
	
	• TempMem should only be used for stack (automatic) strings and return values.
	• See cstrings.cpp for the main use case.

	• pools are thread local and thus thread safe.
	• they are created and destroyed automatically.
	• local pools can be used to bulk-erase locally created temp strings.
	
	There are implementation variants depending on which "tempmem-*.cpp" is included.
	Desktop implementations automatically allocate data buffers as needed.
	Microcomputers with limited ram use fixed-size rolling buffers.

	Desktop: 
		For short running programs (e.g. a scripts or a cgi handlers) can simply forget 
		about purging the accumulating waste memory. 
		Long running programs should purge the tempmem regularly at a low function level.		 
		This may also be done by using short running worker threads.
		Functions which use a lot of tempmem should create a local TempMem pool.
		In this case use xdupstr() to copy return values or exceptions into the surrounding pool.
		
	Rolling tempmem buffers:
		Microcontroller projects are often short of memory and an ever-growing tempmem may be fatal.
		Therefore they use a statically allocated buffer (for each core).
		This buffer never grows but repeatedly wraps around instead, thus overwriting older strings.
		Functions which create a lot of temp strings should create a local pool to protect the caller's strings.
		In this case use xdupstr() to copy return values or exceptions into the surrounding pool.
		Optimize by choosing the right buffer size, when to use a local pool and how long a string may be.
*/


extern str emptystr; // non-const version of ""

extern str	 newstr(uint n) noexcept;	  // allocate memory with new[]
extern str	 newcopy(cstr) noexcept;	  // allocate memory with new[] and copy string
extern char* tempmem(uint size) noexcept; // allocate in current pool: aligned, not cleared
extern str	 tempstr(uint size) noexcept; // allocate in current pool: 0-terminated, not cleared
extern str	 dupstr(cstr) noexcept;		  // allocate  and copy string in the current pool
extern cstr	 xdupstr(cstr) noexcept;	  // allocate  and copy string in the surrounding pool.
extern void	 purge_tempmem() noexcept;	  // purge the current pool


/* convenience:
   allocate in current pool: aligned, not initialized, not cleared.
   type T must not have a destructor.
*/
template<typename T>
T* tempmem(uint count) noexcept
{
	return reinterpret_cast<T*>(tempmem(count * sizeof(T)));
}


/* 
   Class to create and auto-destruct a local TempMem pool.   
*/
class TempMem
{
	NO_COPY_MOVE(TempMem);

public:
	TempMem(uint size = 0); // size is only used for rolling TempMem buffers
	~TempMem();
	static void purge() noexcept { purge_tempmem(); }
};


#if 0

// Examples:

// No local tempmem pool:
// Function foo() calls function do_work_using_temp_mem() which returns a static or temp c-string
// and may throw a plain c-string as well.
// Here the return value can be returned directly, and the thrown c-string can also be passed as-is.
// If this function uses a lot of temp strings then these hang around until they are purged.
// In a rolling tempmem implementation when a temp string is pending in the caller,
// e.g. in function bummer(), it may be overwritten before it is used.
// To avoid this define a local pool in function foo(). (or in do_work_using_temp_mem()).

cstr foo()
{
	cstr rval;
	rval = do_work_using_temp_mem();
	return rval;
}

cstr bummer() { return catstr(numstr(42), foo()); }


// Local tempmem pool:
// Function good() defines a local pool so it can bulk-erase it on return and doesn't flood the caller's pool.
// Function do_work_using_temp_mem() does exactly this and may throw a plain c-string as above.
// On exit of good() the current TempMem pool is destroyed,
// therefore any return string must be copied into the surrounding pool.
// This is also true for a thrown c-string!
// Hint: function xdupstr() duplicates only strings which are in the current pool and returns all others as-is!
// In function bad() the local pool is destroyed before the string is copied, therefore the result is void!

cstr good()
{
	TempMem _t;
	cstr	rval;
	try
	{
		rval = do_work_using_temp_mem();
	}
	catch (cstr err)
	{
		throw xdupstr(err);
	}
	return xdupstr(rval);
}

cstr bad()
{
	cstr rval;
	try
	{
		TempMem _t;
		rval = do_work_using_temp_mem();
	}
	catch (cstr err)
	{
		throw xdupstr(err);
	}
	return xdupstr(rval);
}

#endif

/*






































*/
