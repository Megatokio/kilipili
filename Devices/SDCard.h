// Copyright (c) 2021 - 2023 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "BlockDevice.h"
#include <hardware/spi.h>
//#include "errors.h"
#include "CSD.h"
#include "standard_types.h"


namespace kio::Devices
{

struct CID
{				  // note: MSB first!
	uint8 mid;	  // Manufacturer ID
	uint8 oid[2]; // OEM/Application ID
	char  pnm[5]; // Product name: ascii[5]
	uint8 prv;	  // Product revision: bcd_digits[2] -> x.y
	uint8 psn[4]; // Product serial number: uint32
	uint8 mdt[2]; // reserved nibble + Manufacturing date: hex_nibbles[3] -> yy.m
	uint8 crc;	  // crc7 << 1 | 0x01
};
static_assert(sizeof(CID) == 16, "");


constexpr char DEVICE_NOT_RESPONDING[]	 = "Device not responding";
constexpr char CRC_ERROR[]				 = "CRC error";
constexpr char HARD_WRITE_ERROR[]		 = "Hard write error";
constexpr char DEVICE_INVALID_RESPONSE[] = "Device illegal response";
constexpr char HARD_READ_ERROR[]		 = "Hard read error";
constexpr char CONTROLLER_FAILURE[]		 = "Controller failure";
constexpr char UNKNOWN_ERROR[]			 = "Unknown error";
constexpr char ILLEGAL_COMMAND[]		 = "Illegal command";
constexpr char ADDRESS_ERROR[]			 = "Address error";
constexpr char ERASE_CMD_ERROR[]		 = "Erase cmd error";
constexpr char DEVICE_NOT_SUPPORTED[]	 = "Device not supported";


class SDCard : public BlockDevice
{
	spi_inst* spi; // the spi block
	uint8	  rx_pin;
	uint8	  cs_pin; // gpio pin number of the CSn pin
	uint8	  clk_pin;
	uint8	  tx_pin;

	static_assert(sizeof(LBA) == sizeof(uint32), "");
	static_assert(sizeof(SIZE) == sizeof(SIZE), "");

public:
	enum CardType : uint8 {
		SD_unknown,
		SD_v1,	 // standard capacity SD card (16MB..2GB)
		SD_v2,	 // standard capacity SD card (16MB..2GB)
		SDHC_v2, // SDHC high capacity /2GB..32GB) or SDXC extended capacity (32GB..2TB)
		SDUC_v3, // SDUC ultra capacity SD card (2TB..128TB) (no SPI support!)
		MMC,	 // need 400kHz for initialization, different CSD and CID
	};

	CardType card_type	 = SD_unknown;
	bool	 ccs		 = false; // 1 = SDHC or SDXC: uses sector addresses
	uint8	 erased_byte = 0xff;
	char	 _padding;
	CSD		 csd;
	CID		 cid;
	uint32	 ocr;
	ADDR	 total_size = 0;

	SDCard(uint8 rx, uint8 cs, uint8 clk, uint8 tx);

	void connect();
	void disconnect();

	//virtual void read (ADDR q, uint8* bu, SIZE) override;
	//virtual void write (ADDR z, const uint8* bu, SIZE) override;
	//virtual void copy_blocks (ADDR z, ADDR q, SIZE sz) override { copy_hd(z,q,sz); }
	virtual void   readSectors(LBA, char* bu, SIZE) override;
	virtual void   writeSectors(LBA, const char* bu, SIZE) override;
	virtual uint32 ioctl(IoCtl, void* arg1 = nullptr, void* arg2 = nullptr) override;

	void print_scr(uint v = 1);
	void print_cid(uint v = 1);
	void print_csd(uint v = 1);
	void print_ocr(uint v = 1);
	void print_card_info(uint v = 1);

private:
	void init_spi() noexcept;

	inline void select() const noexcept;
	inline void deselect() const noexcept;
	inline void read_spi(uint8*, uint32) const noexcept;
	inline void write_spi(const uint8*, uint32) const noexcept;

	uint8  read_byte() noexcept;
	uint8  receive_byte(uint retry_count = 100) noexcept;
	bool   wait_ready(uint retry_count = 100000) noexcept;
	void   select_and_wait_ready(uint retry_count = 100000);
	void   stop_transmission() noexcept;					 // CMD12
	uint16 read_status(bool keep_selected = false) noexcept; // CMD13

	enum { no_deselect = 1, f_acmd = 2, f_idle = 4 };
	uint8 send_cmd(uint8 cmd, uint32 args = 0, uint flags = 0);
	uint8 send_acmd(uint8 cmd, uint32 args = 0, uint flags = 0) { return send_cmd(cmd, args, flags | f_acmd); }

	void initialize_card_and_wait_ready();
	void read_ocr();										   // CMD58
	void read_scr();										   // ACMD51
	void read_card_info(uint8 cmd);							   // CMD9+10: CSD+CID
	void write_csd();										   // CMD27  TODO
	void set_blocklen(uint);								   // CMD16
	void read_single_block(uint32 blkidx, uint8* data);		   // CMD17
	void write_single_block(uint32 blkidx, const uint8* data); // CMD24
};

} // namespace kio::Devices

/*



















*/
