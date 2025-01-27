// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause


#include "Flash.h"
#include <string.h>

#if defined UNIT_TEST && UNIT_TEST
  #include "glue.h"
#else
  #include <hardware/flash.h>
  #include <hardware/sync.h>
extern char __flash_binary_end;
#endif


namespace kio::Flash
{

#if defined UNIT_TEST && UNIT_TEST

static cuptr  flash_start;
static cuptr  flash_start_nocache;
static uint32 xip_flash_size;
uint32		  binary_size() noexcept { return xip_flash_size / 8 - 220; } // dummy

void setupMockFlash(uint8* flash, uint32 size) noexcept
{
	flash_start			= flash;
	flash_start_nocache = flash;
	xip_flash_size		= size;
}

#else

static_assert(wsize == FLASH_PAGE_SIZE);
static_assert(esize == FLASH_SECTOR_SIZE);
static constexpr uint32 xip_flash_size = PICO_FLASH_SIZE_BYTES;

  #define flash_start		  cuptr(XIP_BASE)				  // cached:   used for reading in write()
  #define flash_start_nocache cuptr(XIP_NOCACHE_NOALLOC_BASE) // uncached: used for reading in read()

uint32 binary_size() noexcept { return size_t(&__flash_binary_end) - XIP_BASE; }

#endif


static constexpr uint32 wmask = wsize - 1;
static constexpr uint32 emask = esize - 1;


cuptr  flash_base() noexcept { return flash_start; }
uint32 flash_size() noexcept { return xip_flash_size; }

void flash_erase(uint32 addr, uint32 size) noexcept
{
	assert(get_core_num() == 0);

	if (set_disk_light) set_disk_light(on);
	if (suspend_core1) suspend_core1();
	uint32 o = save_and_disable_interrupts();
	flash_range_erase(addr, size);
	restore_interrupts(o);
	if (resume_core1) resume_core1();
	if (set_disk_light) set_disk_light(off);
}

void flash_program(uint32 addr, cuptr bu, uint32 size) noexcept
{
	assert(get_core_num() == 0);

	if (set_disk_light) set_disk_light(on);
	if (suspend_core1) suspend_core1();
	uint32 o = save_and_disable_interrupts();
	flash_range_program(addr, bu, size);
	restore_interrupts(o);
	if (resume_core1) resume_core1();
	if (set_disk_light) set_disk_light(off);
}

static constexpr uint	left_fract(uint32 addr, uint mask) { return addr & mask; }
static constexpr uint	right_fract(uint32 addr, uint mask) { return -addr & mask; }
static constexpr uint32 left_aligned(uint32 addr, uint mask) { return addr & ~mask; }
static constexpr uint32 right_aligned(uint32 addr, uint mask) { return (addr + mask) & ~mask; }
static constexpr bool	is_aligned(uint32 addr, uint mask) { return (addr & mask) == 0; }

static_assert(left_fract(esize, emask) == 0);
static_assert(right_fract(esize, emask) == 0);
static_assert(left_aligned(esize, emask) == esize);
static_assert(right_aligned(esize, emask) == esize);

static_assert(left_fract(esize + 7, emask) == 7);
static_assert(right_fract(esize - 7, emask) == 7);
static_assert(left_aligned(esize + 7, emask) == esize);
static_assert(right_aligned(esize - 7, emask) == esize);

static_assert(left_aligned(esize + 99, emask) + left_fract(esize + 99, emask) == esize + 99);
static_assert(right_aligned(esize - 99, emask) - right_fract(esize - 99, emask) == esize - 99);


static bool is_erased(cuptr z, uint size) noexcept
{
	cuptr e = z + size;
	while (z < e && *z == 0xff) z++;
	return z == e;
}

static bool is_erased(uint32 addr, uint size) noexcept
{
	return is_erased(flash_start + addr, size); //
}

static bool is_overwritable_with(uint32 addr, cuptr q, uint size) noexcept
{
	cuptr z = flash_start + addr;
	cuptr e = q + size;
	while (q < e && (*z & *q) == *q) q++, z++;
	return q == e;
}

static bool is_same_as(uint32 addr, cuptr q, uint size) noexcept
{
	cuptr z = flash_start + addr;
	return memcmp(q, z, size) == 0;
}

static void erase_partial_sector(uint32 addr, uint size)
{
	assert(right_aligned(addr + size, emask) - left_aligned(addr, emask) == esize);

	if (is_erased(addr, size)) return;

	uint l = left_fract(addr, emask);
	uint r = right_fract(addr + size, emask);
	addr -= l;
	assert(l + size + r == esize);
	assert(is_aligned(addr, emask));

	uint8* bu = uptr(malloc(esize));
	if (!bu) throw OUT_OF_MEMORY;

	memcpy(bu, flash_start + addr, l);
	memset(bu + l, 0xff, size);
	memcpy(bu + l + size, flash_start + addr + l + size, r);
	flash_erase(addr, esize);

	for (l = 0; l < esize && is_erased(bu + l, wsize); l += wsize) {}
	for (r = 0; r < esize - l && is_erased(bu + esize - wsize - r, wsize); r += wsize) {}
	if (esize > l + r) flash_program(addr + l, bu + l, esize - (l + r));
	free(bu);
}

static void erase_sectors(uint32 addr, uint size)
{
	assert(is_aligned(addr, emask));
	assert(is_aligned(size, emask));

	while (size && is_erased(addr, esize)) addr += esize, size -= esize;
	while (size && is_erased(addr + size - esize, esize)) size -= esize;
	//TODO: we could also look for inner gaps and split there
	if (size) flash_erase(addr, size);
}

void eraseData(uint32 addr, uint32 size) throws
{
	// Erase data[size] at address

	if (size == 0) return;
	assert(get_core_num() == 0);
	assert(addr <= xip_flash_size && size <= xip_flash_size - addr);

	if (is_erased(addr, size)) return;

	if (uint d = right_fract(addr, emask))
	{
		if (size <= d) return erase_partial_sector(addr, size);
		erase_partial_sector(addr, d);
		addr += d;
		size -= d;
	}
	assert(is_aligned(addr, emask));
	if (uint d = left_fract(size, emask))
	{
		size -= d;
		erase_partial_sector(addr + size, d);
	}
	assert(is_aligned(size, emask));
	erase_sectors(addr, size);
}

static void write_partial_sector(uint32 addr, cuptr data, uint size)
{
	assert(right_aligned(addr + size, emask) - left_aligned(addr, emask) == esize);

	if (is_same_as(addr, data, size)) return;

	bool is_overwritable = is_overwritable_with(addr, data, size);

	if (is_overwritable && is_aligned((addr | size), wmask))
	{
		// no need to save old contents:
		while (is_same_as(addr, data, wsize)) addr += wsize, data += wsize, size -= wsize;
		while (is_same_as(addr + size - wsize, data + size - wsize, wsize)) size -= wsize;
		assert(size != 0 && addr + size <= xip_flash_size);
		flash_program(addr, data, size);
	}
	else
	{
		uint8* bu = uptr(malloc(esize));
		if (!bu) throw OUT_OF_MEMORY;

		uint r, l = left_fract(addr, emask); // bytes left of addr
		addr -= l;							 // aligned address

		memcpy(bu, flash_start + addr, l);
		memcpy(bu + l, data, size);
		memcpy(bu + size + l, flash_start + addr + size + l, esize - (size + l));
		if (!is_overwritable) flash_erase(addr, esize);

		for (l = 0; l < esize && is_same_as(addr + l, bu + l, wsize); l += wsize) {}
		for (r = 0; r < esize - l && is_same_as(addr + esize - wsize - r, bu + esize - wsize - r, wsize); r += wsize) {}
		if (esize > l + r) flash_program(addr + l, bu + l, esize - (l + r));
		free(bu);
	}
}

static void write_sectors(uint32 addr, cuptr data, uint size)
{
	assert(is_aligned(addr, emask));
	assert(is_aligned(size, emask));

	while (size && is_same_as(addr, data, esize)) data += esize, addr += esize, size -= esize;
	while (size && is_same_as(addr + size - esize, data + size - esize, esize)) size -= esize;
	//TODO: we could also look for inner gaps and split there
	if (size && !is_overwritable_with(addr, data, size)) flash_erase(addr, size);
	if (size) flash_program(addr, data, size);
}

void writeData(uint32 addr, const void* _data, uint32 size) throws
{
	// Write data[size] to address

	if (size == 0) return;
	assert(get_core_num() == 0);
	assert(addr <= xip_flash_size && size <= xip_flash_size - addr);

	cuptr data = cuptr(_data);
	if (is_same_as(addr, data, size)) return;

	if (uint d = right_fract(addr, emask))
	{
		if (size <= d) return write_partial_sector(addr, data, size);
		write_partial_sector(addr, data, d);
		addr += d;
		data += d;
		size -= d;
	}
	assert(is_aligned(addr, emask));
	if (uint d = left_fract(size, emask))
	{
		size -= d;
		write_partial_sector(addr + size, data + size, d);
	}
	assert(is_aligned(size, emask));
	write_sectors(addr, data, size);
}

void readData(uint32 addr, void* data, uint32 size) noexcept
{
	if (size == 0) return;
	assert(addr <= xip_flash_size && size <= xip_flash_size - addr);

	// reading uncached in the hope not to flush the program cache
	// and in the hope that data is mostly read only once.
	memcpy(data, flash_start_nocache + uint32(addr), size);
}


} // namespace kio::Flash
