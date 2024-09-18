// Copyright (c) 2021 - 2024 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "BlockDevice.h"
#include "basic_math.h"

/*
	BlockDevice for I²C EEprom on the Raspberry Pico / RP2040
*/

struct i2c_inst;

namespace kio::Devices
{

class I2cEeprom final : public BlockDevice
{
	i2c_inst*	 i2c_port;
	const uint8	 i2c_addr;		// 7-bit i2c address
	const uint8	 erased_byte;	// 0xff
	const uint16 write_time_us; // AT24C32: 10ms
	CC			 write_started_us;

public:
	I2cEeprom(
		i2c_inst* i2c_port, uint8 i2c_addr, uint32 total_size, uint8 ss, uint write_time_ms = 10,
		Flags flags = readwrite | overwritable) noexcept;

	virtual void writeSectors(LBA, const void* data, SIZE count) throws override;
	virtual void readSectors(LBA, void*, SIZE count) throws override;
	virtual void writeData(ADDR addr, const void* data, SIZE size) throws override;
	virtual void readData(ADDR addr, void* data, SIZE size) throws override;
	//virtual uint32 ioctl(IoCtl cmd, void* arg1 = nullptr, void* arg2 = nullptr) throws;

private:
	void _write(uint, const uint8*, uint);
	void _read(uint, uint8*, uint);
	void i2c_write(uint32 addr, uint8* data, uint cnt);
};

} // namespace kio::Devices

/*
































*/
