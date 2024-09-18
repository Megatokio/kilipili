// Copyright (c) 2021 - 2024 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#include "QspiFlash.h"
#include <hardware/flash.h>
#include <memory>
#include <pico/multicore.h>
#include <pico/stdlib.h>
#include <string.h>


// BlockDevice for the QSPI flash on the Raspi Pico board

#ifndef QSPI_WRITE_PAGE_SIZE
  #define QSPI_WRITE_PAGE_SIZE FLASH_PAGE_SIZE // 1<<8
#endif
#ifndef QSPI_ERASE_SECTOR_SIZE
  #define QSPI_ERASE_SECTOR_SIZE FLASH_SECTOR_SIZE // 1<<12
#endif
#ifndef QSPI_FLASH_SIZE
  #define QSPI_FLASH_SIZE PICO_FLASH_SIZE_BYTES
#endif
#ifndef QSPI_BASE // cached: used for reading in write()
  #define QSPI_BASE XIP_BASE
#endif
#ifndef QSPI_NOCACHE_BASE // uncached: used for reading in read()
  #define QSPI_NOCACHE_BASE XIP_NOCACHE_NOALLOC_BASE
#endif

extern char __flash_binary_end;


namespace kio::Devices
{
static constexpr uint log2(uint n) { return n == 1 ? 0 : 1 + log2(n >> 1); }
static_assert(log2(1) == 0);
static_assert(log2(32) == 5);
static_assert(log2(QSPI_WRITE_PAGE_SIZE) == 8);

static constexpr uint32 wsize = QSPI_WRITE_PAGE_SIZE;
static constexpr uint32 esize = QSPI_ERASE_SECTOR_SIZE;
static constexpr uint32 wmask = wsize - 1;
static constexpr uint32 emask = esize - 1;
static constexpr int	ssw	  = log2(wsize);
static constexpr int	sse	  = log2(esize);

SuspendSystemProc* suspend_system = save_and_disable_interrupts;
ResumeSystemProc*  resume_system  = restore_interrupts;

// clang-format off
struct SuspendSystem { uint32 o; SuspendSystem() : o(suspend_system()) {} ~SuspendSystem() { resume_system(o); } };
// clang-format on


QspiFlash::QspiFlash(uint32 start, uint32 size, Flags flags) :
	BlockDevice((size ? size : QSPI_FLASH_SIZE - start) >> ssw, log2(1), ssw, sse, flags),
	start(start),
	start_cached(uptr(QSPI_BASE + start)),
	start_nocache(uptr(QSPI_NOCACHE_BASE + start))
{
	if (start % QSPI_ERASE_SECTOR_SIZE != 0) panic("Qspi:69");			// must be aligned to erase sector size
	if (size % QSPI_ERASE_SECTOR_SIZE != 0) panic("Qspi:70");			// must be aligned to erase sector size
	if (&__flash_binary_end > ptr(QSPI_BASE + start)) panic("Qspi:71"); // The program overwrote the flash disk.
}

void QspiFlash::readSectors(LBA block, void* data, SIZE count) throws
{
	clamp_blocks(block, count);
	memcpy(data, start_nocache + (block << ssw), count << ssw);
}

void QspiFlash::readData(ADDR addr, void* data, SIZE count) throws
{
	clamp(addr, count);
	memcpy(data, start_nocache + SIZE(addr), count);
}

void QspiFlash::erase_sectors(uint32 addr, uint32 count) throws
{
	// erase whole pages (write sectors)
	// allocates up to 4k on heap if address or count is not aligned to erase sector size
	// if allocation failed then nothing is erased and error OUT_OF_MEMORY is returned

	assert(addr + count <= totalSize());
	assert((addr & wmask) == 0);
	assert((count & wmask) == 0);

	if (count == 0) return;

	std::unique_ptr<uchar[]> bu1;
	std::unique_ptr<uchar[]> bu2;

	uint a = addr & emask;			  // blocks to preserve at start of erase sector
	uint e = -(addr + count) & emask; // blocks to preserve at end of erase sector

	assert((((addr - a) | (count + a + e)) & emask) == 0); // both aligned

	if (a + e >= esize)
	{
		assert(a && e);
		// if a + e >= esize then erase in 2 parts to avoid allocating more than 4k buffer
		// this also guarantees that a and e are in different erase sectors

		bu2.reset(new uchar[std::max(a, e)]);

		memcpy(bu2.get(), start_cached + addr - a, a);
		flash_range_erase(start + addr - a, esize);
		flash_range_program(start + addr - a, bu2.get(), a);
		memcpy(bu2.get(), start_cached + addr + count, e);
		a -= esize; // => a<0
	}
	else
	{
		if (a)
		{
			bu1.reset(new uchar[a]);
			memcpy(bu1.get(), start_cached + addr - a, a);
		}

		if (e)
		{
			bu2.reset(new uchar[e]);
			memcpy(bu2.get(), start_cached + addr + count, e);
		}
	}

	flash_range_erase(start + addr - a, count + a + e);
	if (bu1) flash_range_program(start + addr - a, bu1.get(), a);
	if (bu2) flash_range_program(start + addr + count, bu2.get(), e);
}

static uint l_0xff(cuptr a, uint n) noexcept
{
	uint i = 0;
	while (i < n && a[i] == 0xff) i++;
	return i;
}

static uint r_0xff(cuptr a, uint n) noexcept
{
	for (uint i = n; i--;)
		if (a[i] != 0xff) return n - i - 1;
	return n;
}

static uint l_same(cuptr a, cuptr b, uint n) noexcept
{
	if (!b) return l_0xff(a, n);
	uint i = 0;
	while (i < n && a[i] == b[i]) i++;
	return i;
}

static uint r_same(cuptr a, cuptr b, uint n) noexcept
{
	if (!b) return r_0xff(a, n);
	for (uint i = n; i--;)
		if (a[i] != b[i]) return n - i - 1;
	return n;
}

static uint l_same_or_0xff(cuptr a, cuptr b, uint n) noexcept
{
	if (!b) return l_0xff(a, n);
	uint i = 0;
	while (i < n && (a[i] == b[i] || a[i] == 0xff)) i++;
	return i;
}

static uint r_same_or_0xff(cuptr a, cuptr b, uint n) noexcept
{
	if (!b) return r_0xff(a, n);
	for (uint i = n; i--;)
		if (a[i] != b[i] && a[i] != 0xff) return n - i - 1;
	return n;
}


void QspiFlash::write_sectors(uint32 addr, const uchar* data, uint32 count) throws
{
	// erase and write data
	// if data==nullptr then erase only
	// tries to optimize what needs to be done

	assert((addr & wmask) == 0);
	assert((count & wmask) == 0);

	if (count == 0) return;

	// try to reduce range for writing:
	// each full wsize block at start and end with identical data doesn't need to be written.

	uint32 l = l_same(start_cached + addr, data, count) & ~wmask;
	if (l == count) return; // no change
	uint32 r = r_same(start_cached + addr, data, count) & ~wmask;

	addr += l;
	count -= l + r;
	if (data) data += l;

	uint32 eaddr  = addr;  // erase start address
	uint32 ecount = count; // erase byte count

	// try to reduce range for erasing:
	// each full esize sector at start and end with 0xff or identical data doesn't need to be erased.
	// trim only to full esize sectors, because then erase_sector() does not need to save and restore the odd blocks.

	l = l_same_or_0xff(start_cached + addr, data, count);
	if (l == count) goto x; // nothing to erase
	r = r_same_or_0xff(start_cached + addr, data, count);

	if (l)
	{
		uint32 a = (eaddr + l) & ~emask; // new eaddr aligned to esize
		if (a > eaddr)					 // improved!
		{
			ecount -= a - eaddr;
			eaddr = a;
		}
	}

	if (r)
	{
		uint32 a = (eaddr + ecount - r + emask) & ~emask; // new end address aligned to esize
		if (a < eaddr + ecount)							  // improved!
		{
			ecount -= eaddr + ecount - a;
		}
	}

	// doit:

	erase_sectors(eaddr, ecount);
x:
	if (data) flash_range_program(start + addr, data, count);
}

void QspiFlash::write_bytes(uint32 addr, const uchar* data, uint32 count) throws
{
	// Write data[size] to address
	// If data==nullptr then only erase

	if (count == 0) return;

	uint32 a = addr & wmask;		   // a = bytes before addr in first page
	uint32 b = (addr + count) & wmask; // b = bytes in last page

	if (a + count < wsize)
	{
		uchar bu[wsize];
		memcpy(bu, start_cached + addr - a, wsize);
		if (data) memcpy(bu + a, data, count);
		else memset(bu + a, 0xff, count);
		return write_sectors(addr - a, bu, wsize);
	}

	uchar bu1[wsize];
	uchar bu2[wsize];

	if (a)
	{
		memcpy(bu1, start_cached + addr - a, a);
		if (data) memcpy(bu1 + a, data, wsize - a);
		else memset(bu1 + a, 0xff, wsize - a);
	}

	if (b)
	{
		if (data) memcpy(bu2, data + count - b, b);
		else memset(bu2, 0xff, b);
		memcpy(bu2 + b, start_cached + addr + count, wsize - b);
	}

	assert((((addr - a) | (count + a + wsize - b)) & wmask) == 0);
	write_sectors(addr - a, nullptr, count + a + wsize - b); // throws

	if (a)
	{
		flash_range_program(start + addr - a, bu1, wsize);
		addr += wsize - a;
		if (data) data += wsize - a;
		count -= wsize - a;
	}

	if (b)
	{
		flash_range_program(start + addr + count - b, bu2, wsize);
		count -= b;
	}

	assert(((addr | count) & wmask) == 0);
	if (count && data) flash_range_program(start + addr, data, count);
}

void QspiFlash::writeSectors(LBA block, const void* data, SIZE count) throws
{
	// Write data[size] to address
	// If data==nullptr then erase data

	if (count == 0) return;
	clamp_blocks(block, count);
	SuspendSystem _;
	write_sectors(block << ssw, reinterpret_cast<cuptr>(data), count << ssw);
}

void QspiFlash::writeData(ADDR addr, const void* data, SIZE count) throws
{
	// Write data[size] to address
	// If data==nullptr then erase data

	if (count == 0) return;
	clamp(addr, count);
	SuspendSystem _;
	write_bytes(uint32(addr), reinterpret_cast<cuptr>(data), count);
}

uint32 QspiFlash::ioctl(IoCtl cmd, void* arg1, void* arg2) throws
{
	switch (cmd.cmd)
	{
		//case IoCtl::CTRL_SYNC: return 0;
		//case IoCtl::GET_SECTOR_SIZE: return 1u << ss_write;
		//case IoCtl::GET_BLOCK_SIZE: return 1u << ss_erase;
		//case IoCtl::FLUSH_IN: return 0;
		//case IoCtl::CTRL_RESET: return 0;
		//case IoCtl::CTRL_CONNECT: return 0;
		//case IoCtl::CTRL_DISCONNECT: return 0;

	case IoCtl::CTRL_TRIM:
	{
		// blkdev->ioctl(IoCtl(IoCtl::CTRL_TRIM, IoCtl::LBA, IoCtl::SIZE), &lba, &count);
		if (cmd.arg1 != cmd.LBA || cmd.arg2 != cmd.SIZE || !arg1 || !arg2) throw INVALID_ARGUMENT;
		LBA	 lba   = *reinterpret_cast<LBA*>(arg1);
		SIZE count = *reinterpret_cast<SIZE*>(arg2);
		clamp_blocks(lba, count);
		writeSectors(lba, nullptr, count);
		return 0;
	}
	default: //
		return BlockDevice::ioctl(cmd, arg1, arg2);
	}
}


} // namespace kio::Devices

/*


























*/
