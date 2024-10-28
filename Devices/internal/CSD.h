// Copyright (c) 2021 - 2024 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "standard_types.h"


namespace kio::Devices
{

inline uint16 peek_u16(uint8* p) { return (p[0] * 256 + p[1]); }
inline uint32 peek_u32(uint8* p) { return peek_u16(p) * 0x10000 + peek_u16(p + 2); }
inline void	  poke_u16(uint8* p, uint n)
{
	p[0] = uint8(n >> 8);
	p[1] = uint8(n);
}
inline void poke_u32(uint8* p, uint32 n)
{
	poke_u16(p, n >> 16);
	poke_u16(p + 2, uint16(n));
}
inline constexpr uint mask(uint bits) { return (1 << bits) - 1; }


/*
	The "Card Specific Data" Structure
	is 16 bytes long and comes in 3 versions
*/

struct CSD
{
	// the actual data:
	uint32 data[4] = {0};

	// get/set bitfield with right-aligned bit numbers, e.g. foo[7:0] as used in SDcard specs
	// note: can't read accross uint32 boundary!
	// note: data[] are byte swapped for LittleEndian in read_card_info() to make get() work as expected

	uint get(uint a, uint e) const noexcept { return data[3 - e / 32] >> (e & 31) & mask(1 + a - e); }

	void set(uint a, uint e, uint value) noexcept
	{
		uint32& n = data[3 - e / 32];
		uint	m = mask(1 + a - e) << (e & 31);
		n		  = (n & ~m) | (value << (e & 31));
	}

	// CSDv1: get/set use bitfild spec as printed in SDCard PhyLayerSpec §5.3.2 pg.188
	// CSDv2: get/set use bitfild spec as printed in SDCard PhyLayerSpec §5.3.3 pg.195
	// CSDv2: get/set use bitfild spec as printed in SDCard PhyLayerSpec §5.3.3 pg.198

	// the super class provides getters for those bits which are somehow supported in all versions
	// only in CSDv1: taac, nsac, min/max currents, wp_group, file_format, s_size_mult
	// c_size: for disk size use disk_size()

	uint csd_structure() { return get(127, 126); } // pg187 0/1/2 = Version 1/2/3

	uint tran_speed() { return get(103, 96); }		 // pg189 max. clock speed: 0x32=25, 0x5A=50, 0x0B=100, 0x2B=200MHz
	uint ccc() { return get(95, 84); }				 // pg190 card command class: 0b01x110110101
	uint read_bl_bits() { return get(83, 80); }		 // pg190 max. read block length (bits): 9…11
	bool read_bl_partial() { return get(79, 79); }	 // pg190 read partial blocks allowed?
	bool write_bl_misalign() { return get(78, 78); } // pg190 crossing block bounds allowed?
	bool read_bl_misalign() { return get(77, 77); }	 // pg190 crossing block bounds allowed?
	bool dsr_imp() { return get(76, 76); }			 // pg190 configurable Driver State Register DSR implemented?

	bool erase_blk_en()
	{
		return get(46, 46);
	} // pg192 erase multiple of write block size (instead of sector size) enabled
	uint  erase_sector_blks() { return get(45, 39); } // pg193 erase sector size as a multiple of write block size
	uint  r2w_factor() { return get(28, 26); } // pg193 write speed factor, bitshift for read_access_time, typical value
	uint  write_bl_bits() { return get(25, 22); } // pg193 write block length (bits). SDcard: write_bl_len = read_bl_len
	bool  write_bl_partial() { return get(21, 21); } // pg193 write partial blocks allowed?
	bool  copy() { return get(14, 14); }			 // pg194 r/w1 this disk is a copy HAHA! (write once)
	bool  perm_write_prot() { return get(13, 13); }	 // pg194 r/w1 permanent write protected (write once)
	bool  tmp_write_prot() { return get(12, 12); }	 // pg194 r/w  temporarily write protected
	bool  write_prot() { return get(13, 12); }
	uint8 crc() { return uint8(get(7, 0)); } // pg194 r/w  crc7 << 1 | 0x01

	static const uint8 v[16]; // 0,10,12,...,80

	uint32 max_clock(); // min 100kBit/s, max 800MBit/s
	uint64 disk_size();
	uint   read_block_size() { return 1 << read_bl_bits(); }
	uint   write_block_size() { return 1 << write_bl_bits(); }
	uint32 erase_sector_size();
	uint32 read_access_time_cc(uint32 f_clk); // in clock cycles
	uint32 read_access_time_us(uint32 f_clk); // in micro seconds
};

struct CSDv1 : public CSD // standard capacity SD card
{
	// get/set use bitfild spec as printed in SDCard PhyLayerSpec §5.3.2 pg.188

	uint csd_structure() { return get(127, 126); } // pg187 version 1: 0b00
	uint taac() { return get(119, 112); }		   // pg189 data read access time¹
	uint taac_unit() { return get(114, 112); }
	uint taac_value() { return get(118, 115); }
	uint nsac() { return get(111, 104); }					// pg189 data read access-time² in cc (nsac*100)
	uint tran_speed() { return get(103, 96); }				// pg189 max. data rate: 0x32 (25MHz) or 0x5A (50MHz)
	uint ccc() { return get(95, 84); }						// pg190 card command class: 0b01x110110101
	uint read_bl_bits() { return get(83, 80); }				// pg190 max. read block length (bits): 9..11
	bool read_bl_partial() { return get(79, 79); }			// pg190 read partial blocks allowed: 0b1 (SDcard: required)
	bool write_bl_misalign() { return get(78, 78); }		// pg190 crossing block boundary allowed?
	bool read_bl_misalign() { return get(77, 77); }			// pg190 crossing block boundary allowed?
	bool dsr_imp() { return get(76, 76); }					// pg190 configurable Driver State Register DSR implemented?
	uint c_size() { return get(73, 64) * 4 + get(63, 62); } // pg191 device size
	uint vdd_r_curr_min() { return get(61, 59); }			// pg191 max. read current at Vdd_min
	uint vdd_r_curr_max() { return get(58, 56); }			// pg191 max. read current at Vdd_max
	uint vdd_w_curr_min() { return get(55, 53); }			// pg191 max. write current at Vdd_min
	uint vdd_w_curr_max() { return get(52, 50); }			// pg191 max. write current at Vdd_max
	uint c_size_mult() { return get(49, 47); }				// pg192 device size multiplier

	bool erase_blk_en() { return get(46, 46); }	 // pg192 erase multiple of write block size enabled
	uint sector_size() { return get(45, 39); }	 // pg193 size of erasable sector as a multiple of write block size
	uint wp_grp_size() { return get(38, 32); }	 // pg193 write protect group size
	bool wp_grp_enable() { return get(31, 31); } // pg193 write protect enable
	uint r2w_factor() { return get(28, 26); } // pg193 write speed factor, bitshift for read_access_time, typical value
	uint write_bl_bits()
	{
		return get(25, 22);
	} // pg193 write block length bitshift. SDcard: write_bl_len = read_bl_len
	bool write_bl_partial() { return get(21, 21); } // pg193 write partial blocks allowed
	uint file_format_grp() { return get(15, 15); }	// pg194 r/w1 (better ignore this)
	bool copy() { return get(14, 14); }				// pg194 r/w1 this disk is a copy HAHA!
	bool perm_write_prot() { return get(13, 13); }	// pg194 r/w1 permanent write protected
	bool tmp_write_prot() { return get(12, 12); }	// pg194 r/w  temporarily write protected
	uint file_format() { return get(11, 10); }		// pg194 r/w1 (better ignore this)
	uint reserved_rw() { return get(9, 8); }		//       r/w  0b00
	uint crc() { return get(7, 0); }				// pg194 r/w  crc7 << 1 | 0x01

	uint32 disk_size() // 2GB max
	{
		return uint32(c_size() + 1) << (2 + c_size_mult() + read_bl_bits());
	}

	uint32 read_access_time_cc(uint32 f_clk); // in clock cycles
	uint32 read_access_time_us(uint32 f_clk); // in micro seconds
};

struct CSDv2 : public CSDv1 // SDHC and SDXC card
{
	// get/set use bitfild spec as printed in SDCard PhyLayerSpec §5.3.3 pg.195 - 197
	// changes to v1:
	// current settings removed
	// disk size calculation changed
	// many values now fixed: read&write_bl_len, no misaligned or partial r/w, no wp_grp

	uint c_size()
	{
		return get(69, 64) * 0x10000 + get(63, 48); // device size: total_size=(c_size+1)<<9
	}

	uint64 disk_size() // 2TB max
	{
		// The Minimum user area size of SDHC Card is 4,211,712 sectors (2GB + 8.5MB).
		// The Minimum value of C_SIZE for SDHC in CSD Version 2.0 is 001010h (4112).
		// The maximum user area size of SDHC Card is (32GB - 80MB)
		// The maximum value of C_SIZE for SDHC in CSD Version 2.0 is 00FF5Fh (65375).
		//
		// The Minimum user area size of SDXC Card is 67,108,864 sectors (32GB).
		// The Minimum value of C_SIZE for SDXC in CSD Version 2.0 is 00FFFFh (65535).
		// The maximum user area size of SDXC Card is 4,294,705,152 sectors (2TB - 128MB).
		// The maximum value of C_SIZE for SDXC in CSD Version 2.0 is 3FFEFFh (4194047).

		return uint64(c_size() + 1) << 19;
	}

	uint32 read_access_time_cc(uint32 f_clk) // in clock cycles
	{
		return f_clk / 1000;
	}
	uint32 read_access_time_us(uint32 __unused f_clk) // in micro seconds
	{
		return 1000;
	}
};

struct CSDv3 : public CSDv2 // SDUC card
{
	// ***NOTE***
	// SDUC cards don't support SPI.
	// They need more than 32 bits for sector addresses.

	// get/set use bitfild spec as printed in SDCard PhyLayerSpec §5.3.3 pg.198 - 200
	// changes to v2:
	// 6 more bits added to c_size

	uint c_size()
	{
		return get(75, 64) * 0x10000 + get(63, 48); // device size: total_size=(c_size+1)<<9
	}

	uint64 disk_size() // 128TB max
	{
		// The Minimum user area size of SDUC Card is 4,294,968,320 sectors (2TB+0.5MB).
		// The Minimum value of C_SIZE for SDUC in CSD Version 3.0 is 0400000h (4194304).
		// The Maximum user area size of SDUC Card is 274,877,906,944 sectors (128TB).
		// The Maximum value of C_SIZE for SDUC in CSD Version 3.0 is FFFFFFFh (268435455).

		return uint64(c_size() + 1) << 9;
	}
};

} // namespace kio::Devices
