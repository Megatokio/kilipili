// Copyright (c) 2021 - 2024 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#include "I2cEeprom.h"
#include "utilities.h"
#include <hardware/i2c.h>
#include <string.h>


namespace kio::Devices
{

I2cEeprom::I2cEeprom(
	i2c_inst* i2c_port, uint8 i2c_addr, uint32 total_size, uint8 ss, uint write_time_ms, Flags flags) noexcept :
	BlockDevice(total_size >> ss, 0, ss, 0, flags),
	i2c_port(i2c_port),
	i2c_addr(i2c_addr),
	erased_byte(0xff), // 0x00 is good as well..
	write_time_us(uint16(std::min(write_time_ms * 1000, 65535u)))
{
	assert(ss <= 7); // 64kB eeproms have 128 byte sectors
}

void I2cEeprom::i2c_write(uint32 addr, uint8* data, uint size)
{
	// Write data[size] to address
	// data[] must be 2 bytes larger than size to hold the address.
	// data[0…1] will be overwritten with address.
	// If size==0 then the connection is left open for reading from this address.

	data[0] = uint8(addr >> 8);
	data[1] = uint8(addr);

	for (;;)
	{
		int n = i2c_write_blocking(i2c_port, i2c_addr, data, size + 2, !size);
		if (n == int(size + 2))
		{
			if (size) { write_started_us = now(); }
			return;
		}
		if (n != 0) throw HARD_WRITE_ERROR;
		if (uint32(now() - write_started_us) > uint32(write_time_us)) throw DEVICE_NOT_RESPONDING;
	}
}

void I2cEeprom::_read(uint addr, uint8* data, uint size)
{
	// Read data

	uint8 bu[2];
	i2c_write(uint32(addr), bu, 0); // write the address

	int n = i2c_read_blocking(i2c_port, i2c_addr, data, size, false);
	if (n == int(size)) return; // OK
	throw HARD_READ_ERROR;
}

void I2cEeprom::_write(uint offs, const uint8* data, uint size)
{
	// Write data
	// If data==nullptr then erase with this->erased_byte

	while (size)
	{
		uint cnt = (1 << ss_write) - (offs & ((1 << ss_write) - 1));
		if (cnt > size) cnt = size;

		uint8 bu[128 + 2];
		assert(ss_write <= 7);

		if (data)
		{
			memcpy(bu + 2, data, size);
			data += cnt;
		}
		else
		{
			memset(bu + 2, erased_byte, size); //
		}

		i2c_write(uint32(offs), bu, size);

		offs += cnt;
		size -= cnt;
	}
}

void I2cEeprom::readData(ADDR addr, void* data, SIZE size) throws
{
	clamp(uint32(addr), size);
	if (size) _read(uint32(addr), reinterpret_cast<uptr>(data), uint32(size));
}

void I2cEeprom::writeData(ADDR addr, const void* data, SIZE size) throws
{
	clamp(uint32(addr), size);
	if (size) _write(uint32(addr), reinterpret_cast<cuptr>(data), uint32(size));
}

void I2cEeprom::writeSectors(LBA blk, const void* data, SIZE cnt) throws
{
	uint32 addr = blk << ss_write;
	uint32 size = cnt << ss_write;
	clamp(addr, size);
	if (size) _write(addr, reinterpret_cast<cuptr>(data), size);
}

void I2cEeprom::readSectors(LBA blk, void* data, SIZE cnt) throws
{
	uint32 addr = blk << ss_write;
	uint32 size = cnt << ss_write;
	clamp(addr, size);
	if (size) _read(addr, reinterpret_cast<uptr>(data), size);
}


} // namespace kio::Devices


/*



























*/
