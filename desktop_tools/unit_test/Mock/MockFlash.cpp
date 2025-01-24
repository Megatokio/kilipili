// Copyright (c) 2025 - 2025 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#include "MockFlash.h"
#include "Array.h"

namespace kio
{

static constexpr int	ssw	  = 8;
static constexpr int	sse	  = 12;
static constexpr uint32 wsize = 1 << ssw;
static constexpr uint32 esize = 1 << sse;
static constexpr uint32 wmask = wsize - 1;
static constexpr uint32 emask = esize - 1;

static MockFlash* qspi = nullptr;

#define LOG(...)	   qspi->log.append(usingstr(__VA_ARGS__))
#define LOG_ERROR(...) qspi->error_log.append(usingstr(__VA_ARGS__))


extern void flash_range_erase(uint32 addr, uint32 size)
{
	assert(qspi);
	LOG("erase 0x%08x + 0x%08x -> 0x%08x", addr, size, addr + size);

	if (addr & emask || size & emask || addr + size > qspi->data.count()) qspi->error = 1;
	else memset(&qspi->data[addr], 0xff, size);
}

extern void flash_range_program(uint32 addr, const uint8* data, uint32 size)
{
	assert(qspi);
	LOG("write 0x%08x + 0x%08x -> 0x%08x", addr, size, addr + size);

	if (!data)
	{
		qspi->error = 1;
		LOG("data==nullptr");
	}
	else if (addr & wmask || size & wmask || addr + size > qspi->data.count()) //
	{
		qspi->error = 1;
	}
	else
	{
		for (uint i = 0; i < size; i++) qspi->data[addr + i] &= data[i];
	}
}

MockFlash::MockFlash(Array<uint8>& data, Array<cstr>& log, bool& error) : //
	data(data),
	log(log),
	error(error)
{
	assert((data.count() & emask) == 0);

	log.purge();
	error = 0;
	qspi  = this;
}

MockFlash::~MockFlash()
{
	if (qspi == this) qspi = nullptr;
}

} // namespace kio

/*





























*/
