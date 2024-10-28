// Copyright (c) 2021 - 2021 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#include "CSD.h"

namespace kio::Devices
{

const uint8			CSD::v[16] = {0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80};
static const uint32 u[8]	   = {1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100};

uint32 CSDv1::read_access_time_cc(uint32 f_clk) // in clock cycles
{
	return nsac() * 100 + v[taac_value()] * f_clk / u[taac_unit()] / 10;
}

uint32 CSDv1::read_access_time_us(uint32 f_clk) // in micro seconds
{
	return 1000000 * nsac() / f_clk * 100 + 100000 * v[taac_value()] / u[taac_unit()];
}

uint32 CSD::read_access_time_cc(uint32 f_clk)
{
	return csd_structure() ? reinterpret_cast<CSDv2*>(this)->read_access_time_cc(f_clk) :
							 reinterpret_cast<CSDv1*>(this)->read_access_time_cc(f_clk);
}

uint32 CSD::read_access_time_us(uint32 f_clk)
{
	return csd_structure() ? reinterpret_cast<CSDv2*>(this)->read_access_time_us(f_clk) :
							 reinterpret_cast<CSDv1*>(this)->read_access_time_us(f_clk);
}

uint64 CSD::disk_size()
{
	if (csd_structure() == 0) return reinterpret_cast<CSDv1*>(this)->disk_size();
	if (csd_structure() == 1) return reinterpret_cast<CSDv2*>(this)->disk_size();
	if (csd_structure() == 2) return reinterpret_cast<CSDv3*>(this)->disk_size();
	return 0;
}

uint32 CSD::erase_sector_size() { return (erase_blk_en() ? 1 : 1 + erase_sector_blks()) << write_bl_bits(); }

uint32 CSD::max_clock()
{
	// min 100kBit/s, max 800MBit/s
	// note: this function ignores the reserved bits
	// note: max. clock for SPI mode is 25MHz (class 0)

	static const uint32 u[4] = {10000, 100000, 1000000, 10000000};
	return v[get(102, 99)] * u[get(97, 96)];
}

} // namespace kio::Devices
