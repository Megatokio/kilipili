// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#pragma once
#include "Array.h"
#include "standard_types.h"
#include <string.h>


namespace kio
{

/*
	Tagged data store for the internal program flash on the Raspberry Pico / RP2040
	probably usable for any RP2040 board as long as the pico-sdk is used.

	The data store is always located at the end of the flash.
	The size must be a multiple of the erase sector size which is 1<<12 = 4096 bytes.
	The size is set with FLASH_PREFERENCES. If it is not set then there are no preferences.

	The store can hold up to 255 items with tag numbers from 0 to 254. 
	The items must be flat data and can have a size of 1 to 255 bytes.
	For c-strings this includes the final null-byte, so the string length can be 0 to 254.

	New items are initially stored in an array which allocates memory in the heap.
	write pending data to flash and dispose of the array with `sync()`.

	`get(tag,cstr)` returns a pointer directly to the found string.
	If this has been updated with `set()` and not yet written to the flash,
	then the returned cstr points into the array and may become invalid with the next call to set()
	or when sync() is called or the instance is destroyed.
	
	ATTN: Writing to the program flash requires to stop whatever runs in or reads from flash!
	  When data is written to flash `__weak suspend_core1()` and `resume_core1()` are called.
	  The default weak implementations are in Devices/VideoController, 
	  They may be reimplemented by the application if core1 is not used for video.
*/
class Preferences
{
public:
	// create instance to access preferences in flash.
	// both core0 and core1 should create their own instance unless you only read from the prefs.
	Preferences() noexcept;

	// destroy instance and write updated preferences back into the flash data store.
	// writing to flash is only allowed for core0 and data set on core1 are lost.
	// if preferences were updated then the system is locked out during flash write.
	// core1 may see an empty data store for a short moment.
	// logs an error if sync() threw.
	~Preferences() noexcept;

	// write updated preferences to flash data store.
	// has no effect and throws an Error if called on core1.
	// if preferences were updated then the system is locked out during flash write.
	// core1 may see an empty data store for a short moment.
	// Throws and nothing is written if the flash data store would overflow.
	void sync() throws;

	// read arbitrary flat data from the preferences.
	// throws if the data size does not match.
	const void* read(uint8 tag, const void* default_value, uint size) throws;

	// read arbitrary flat data from the preferences.
	// throws if the data size does not match.
	template<typename T>
	T read(uint8 tag, const T& default_value) throws
	{
		T v;
		memcpy(&v, read(tag, &default_value, sizeof(T)), sizeof(T));
		return v;
	}

	// read a cstring from the preferences.
	// if the tag was updated then a pointer into the dending_data array is returned.
	// any way, the returned cstring should be duplicated or used immediately.
	// throws if the tag contains no string.
	cstr read(uint8 tag, cstr default_value) throws;
	cstr read(uint8 tag, str default_value) throws { return read(tag, cstr(default_value)); }

	// store arbitrary flat data into the preferences.
	// the data is stored into the pending_data array and only written in sync() or the destructor.
	// until then the updated value is visible to this core only.
	// core1 cannot write to flash and the update will be lost.
	void write(uint8 tag, const void* data, uint size) throws;

	// store arbitrary flat data into the preferences.
	template<typename T>
	void write(uint8 tag, const T& data) throws
	{
		write(tag, &data, sizeof(data));
	}

	// store a cstring into the preferences.
	void write(uint8 tag, cstr string) throws;
	void write(uint8 tag, str string) throws { write(tag, cstr(string)); }

	// remove a tag from the preferences.
	// the removal is stored in the pending_data array and only written in sync() or the destructor.
	// until then removed values are still visible for the other core.
	// core1 cannot write to flash and the changes are lost.
	void remove(uint8 tag) throws;

	// calculate free size in flash data store
	// this adds the remaining free size, the gaps in the store and subtracts the pending_data size.
	// if the returned value is negative then the store will overflow when sync() is called or
	// the instance is destroyed. You may try to remove or shorten some tags and increase the
	// store size with a program update. The added sectors must first be erased with 0xFF!
	int free() noexcept;

	void dump_store();

private:
	Array<uint8> pending_data;
};

} // namespace kio

/*































*/
