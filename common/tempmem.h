// Copyright (c) 2008 - 2026 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "cdefs.h"
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
	• TempMemSave can be used to bulk-erase locally created temp strings.
	
	• Short running programs (e.g. a scripts or a cgi handlers) can simply forget 
	  about purging the accumulating waste memory. 
	• Long running programs should purge the tempmem regularly at a low function level.
	• Functions which use a lot of tempmem should create a local TempMemSave.
	  In this case use xdupstr() to copy return values below the TempMemSave position.
*/


namespace kilipili
{

extern char emptystr[]; // non-const version of ""

extern str	newstr(uint n);			  // allocate memory with new[]
extern str	newcopy(cstr);			  // allocate memory with new[] and copy string
extern str	tempstr(uint len);		  // allocate in current pool: 0-terminated, not cleared
extern str	dupstr(cstr);			  // allocate in current pool and copy string
extern ptr	tempmem(uint size);		  // allocate in current pool: aligned, not cleared
extern void purge_tempmem() noexcept; // purge the current pool


/*	get and restore the current allocation position of the TempMem pool.
	Use this inside or before calling a function which may allocate some temp strings
	and would, if called repeatedly, overflow the heap.
*/
struct TempMemSave
{
	TempMemSave() noexcept;
	~TempMemSave() noexcept;

	// purge tempmem up to this TempMemSave's save position
	void purge() noexcept;

	// purge tempmem and copy string before this TempMemSave's save position
	// so that the string still exists after this TempMemSave is destroyed.
	str xdupstr(cstr);

	ptr old_save;
};

} // namespace kilipili


/*






































*/
