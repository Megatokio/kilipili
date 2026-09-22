// Copyright (c) 2008 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
#include "standard_types.h"


/*	Temporary Memory Pool
	=====================
	
	tempmem provides fast memory for temporary c-style strings and return values
	which can be used like static c-strings and text literals.
	
	In addition to c-strings any small data with no destructor can be allocated in tempmem.
	
	For an example see `cstrings.h|.cpp`, the primary use case.

	• tempmem should not be used for long living values,
	  only for stack (automatic) strings and return values.

	• tempmem is thread local and thus thread safe.
	• tempmem is created and destroyed automatically.
	• TempMemSave can be used to set save positions and easily bulk erase locally created temp strings.
	
	• Short running programs (e.g. a scripts or a cgi handlers) can simply forget 
	  about purging the accumulating waste memory. 
	• Long running programs should purge the tempmem regularly at a low function level.
	• Functions which use a lot of tempmem should create a local TempMemSave to save the current tempmem position.
	  To return a string from within a TempMemSave use xdupstr() to create a copy below the save position.
	  Be careful if you have nested save positions. xdupstr() only skips one level.
*/


namespace kilipili
{

extern char emptystr[]; // non-const version of ""

extern str	newstr(uint n);			  // allocate memory with new[]
extern str	newcopy(cstr);			  // allocate memory with new[] and copy string
extern str	tempstr(uint len);		  // allocate tempmem: 0-terminated, not cleared
extern str	dupstr(cstr);			  // allocate tempmem and copy string
extern ptr	tempmem(uint size);		  // allocate tempmem: aligned, not cleared
extern void purge_tempmem() noexcept; // purge tempmem up to the last save position
extern str	xdupstr(cstr);			  // purge_tempmem (!) and copy string before current save position


/*	save and restore the current allocation position of the tempmem pool.
	Use this inside or before calling a function which may allocate some temp strings
	and would, if called repeatedly, overflow the heap.
*/
struct TempMemSave
{
	TempMemSave() noexcept;
	~TempMemSave() noexcept;

	// purge tempmem up to this TempMemSave's save position
	static void purge() noexcept; // same as purge_tempmem()

	ptr old_save;
};

} // namespace kilipili


/*






































*/
