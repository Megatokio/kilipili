// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#if defined FLASH_PREFERENCES && FLASH_PREFERENCES != 0

  #include "Preferences.h"
  #include "Array.h"
  #include "Devices/Flash.h"
  #include <cstring>

  #if defined UNIT_TEST && UNIT_TEST
	#include "glue.h"
	#define logline(...) (void)0
  #else
	#include "Logger.h"
	#include <pico.h>
  #endif


/*
	Layout of the data store:

	The last byte in the store is 0xff.
	The store grows to lower addresses. New items are stored before the currently first_tag item.

	A new item can have the same tag as an existing item, logically overwriting the old one.
	If an item has zero length, it logically erases this tag.
	To prevent frequent flash_erase cycles the old tag is not physically overwritten.

	New items are initially stored in an Array.
	The array is written to flash with `sync()`.

	Layout of a tagged item:

		uchar[size] text
		uchar       size
		uchar       tag
*/

namespace kio
{

  #if defined UNIT_TEST && UNIT_TEST
	#define xip_flash_base Flash::flash_base()
	#define xip_flash_size Flash::flash_size()
  #else
	#define xip_flash_base cuptr(XIP_BASE)
	#define xip_flash_size PICO_FLASH_SIZE_BYTES
  #endif

  #define xip_flash_end (xip_flash_base + xip_flash_size)
  #define prefs_base	(xip_flash_end - prefs_size)
static constexpr uint prefs_size = FLASH_PREFERENCES;
static_assert(prefs_size % Flash::esize == 0 && prefs_size <= 32 kB);

static cuptr first_tag = nullptr;


static void _init() noexcept
{
	// search the store data from the start to find the first non-0xFF byte which is the first tag.
	// check the linking and set to an empty if it is invalid.

	cuptr p	  = prefs_base;
	cuptr end = xip_flash_end;

	while (p < end && *p == 0xff) { p++; }
	first_tag = p;

	while (p < end && *p != 0xff) { p += 2 + p[1]; }

	if (p != end)
	{
		first_tag = end;
		logline("preferences invalid");
	}
}

void Preferences::sync() throws
{
	const uint& count = pending_data.count();
	if (count == 0) return;
	if (get_core_num() != 0) throw "preferences set by core1 are lost";

	if (first_tag - count < prefs_base) // need to compact
	{
		bool seen[256];
		memset(seen, 0, 256);

		for (uint i = 0; i < count;)
		{
			seen[pending_data[i]] = true;
			if (uint sz = pending_data[i + 1]) i += 2 + sz;
			else pending_data.removerange(i, i + 2);
		}

		for (cuptr p = first_tag; p < xip_flash_end; p += 2 + p[1])
		{
			if (!seen[*p] && p[1] != 0) pending_data.append(p, 2 + p[1]); // throws
			seen[*p] = true;
		}

		if (xip_flash_end - count < prefs_base) throw "preferences overflowed";

		first_tag = xip_flash_end; // core1 sees an empty store
	}

	// if store was compacted or is invalid (or was just empty):
	if (first_tag == xip_flash_end) Flash::flash_erase(xip_flash_size - prefs_size, prefs_size);

	Flash::writeData(uint32(first_tag - count - xip_flash_base), pending_data.getData(), count);
	first_tag = first_tag - count; // core1 sees the new values
	pending_data.purge();
}

Preferences::Preferences() noexcept
{
	if (first_tag == nullptr) _init();
}

Preferences::~Preferences() noexcept
{
	try
	{
		sync();
	}
	catch (Error e)
	{
		logline(e);
	}
	catch (...)
	{}
}

static uint _find(cuptr data, uint size, uint8 tag) noexcept
{
	for (uint i = 0; i < size; i += 2 + data[i + 1])
		if (data[i] == tag) return i + 2;

	return 0; // not found
}

const void* Preferences::read(uint8 tag, const void* default_value, uint size) throws
{
	cuptr q = pending_data.getData();
	uint  i = _find(q, pending_data.count(), tag);
	if (i)
	{
		q += i - 1;						   // q -> tag.size
		if (*q == 0) return default_value; // erased
	}
	else
	{
		i = _find(first_tag, uint(xip_flash_end - first_tag), tag);
		q = first_tag + i - 1;						 // q -> tag.size
		if (i == 0 || *q == 0) return default_value; // not found or erased
	}

	if (size == 0) // want a string
	{
		if (memchr(q + 1, 0, *q) != q + *q) throw "preference value is not a string";
	}
	else // want flat data
	{
		if (*q != size) throw "preference value has wrong size";
	}

	return q + 1;
}

void Preferences::write(uint8 tag, const void* data, uint size) throws
{
	assert(size <= 255);
	assert(data != nullptr || size == 0);
	assert(size != 0 || data == nullptr);

	uint i = _find(pending_data.getData(), pending_data.count(), tag);
	if (i) pending_data.removerange(uint(i - 2), uint(i) + pending_data[i - 1]);

	pending_data.append(tag);
	pending_data.append(uint8(size));
	pending_data.append(cuptr(data), size);
}

int Preferences::free() noexcept
{
	int gaps = 0;

	bool seen[256];
	memset(seen, 0, 256);

	for (uint i = 0; i < pending_data.count(); i += 2 + pending_data[i + 1]) //
	{
		seen[pending_data[i]] = true;
		if (pending_data[i + 1] == 0) gaps += 2;
	}

	for (cuptr p = first_tag; p < xip_flash_end; p += 2 + p[1])
	{
		if (seen[*p] || p[1] == 0) gaps += 2 + p[1];
		seen[*p] = true;
	}

	return int(first_tag - prefs_base) + gaps - int(pending_data.count()); // may be negative!
}

void Preferences::write(uint8 tag, cstr text) throws { write(tag, text ? text : "", strlen(text ? text : "") + 1); }
cstr Preferences::read(uint8 tag, cstr default_value) throws { return cstr(read(tag, default_value, 0)); }
void Preferences::remove(uint8 tag) throws { write(tag, nullptr, 0); }

static cstr tostr(cuptr p)
{
	uint size = p[1];
	p += 2;
	if (size == 0) return "removed";
	if (size == 1 && *p == 0) return "int8 = 0 or empty string";
	if (size == 1) return usingstr("int8 = %i", *p);
	if (memchr(p, 0, size) == p + size - 1) { return quotedstr(cstr(p)); }
	if (size == 2)
	{
		int16 n;
		memcpy(&n, p, 2);
		return usingstr("int16 = %i", n);
	}
	if (size == 4)
	{
		int32 n;
		memcpy(&n, p, 4);
		return usingstr("int32 = %i", n);
	}
	cstr s = "data = ";
	for (uint i = 0; i < size; i++) s = usingstr("%s %02x", s, p[i]);
	return s;
}

void Preferences::dump_store()
{
	Array<ptr> list(256);

	for (cuptr p = pending_data.getData(); p < pending_data.getData() + pending_data.count(); p += 2 + p[1])
	{
		if (list[*p] == nullptr) list[*p] = usingstr("%03i: %s (not synced)", *p, tostr(p));
	}
	for (cuptr p = first_tag; p < xip_flash_end; p += 2 + p[1])
	{
		if (list[*p] == nullptr) list[*p] = usingstr("%03i: %s", *p, tostr(p));
	}
	for (uint i = 0; i < 256; i++)
	{
		if (list[i]) printf("%s\n", list[i]);
	}
}

} // namespace kio

#endif

/*


































*/
