// Copyright (c) 2021 - 2025 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#pragma once
#include "BlockDevice.h"
#include "CSD.h"
#include "SerialDevice.h"
#include "cdefs.h"
#include "standard_types.h"
#include <hardware/spi.h>


namespace kio::Devices
{

struct CID
{						// note: MSB first!
	uint8 mid	 = 0;	// Manufacturer ID
	uint8 oid[2] = {0}; // OEM/Application ID
	char  pnm[5] = {0}; // Product name: ascii[5]
	uint8 prv	 = 0;	// Product revision: bcd_digits[2] -> x.y
	uint8 psn[4] = {0}; // Product serial number: uint32
	uint8 mdt[2] = {0}; // reserved nibble + Manufacturing date: hex_nibbles[3] -> yy.m
	uint8 crc	 = 0;	// crc7 << 1 | 0x01
};
static_assert(sizeof(CID) == 16, "");


constexpr char CRC_ERROR[]				 = "CRC error";
constexpr char DEVICE_INVALID_RESPONSE[] = "Device illegal response";
constexpr char CONTROLLER_FAILURE[]		 = "Controller failure";
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
	bool	 no_crc		 = false;
	CSD		 csd;
	CID		 cid;
	uint32	 ocr = 0;

	static SDCard* defaultInstance();

	SDCard(uint8 rx, uint8 cs, uint8 clk, uint8 tx) noexcept;

	//virtual void read (ADDR q, uint8* bu, SIZE) override;
	//virtual void write (ADDR z, const uint8* bu, SIZE) override;
	//virtual void copy_blocks (ADDR z, ADDR q, SIZE sz) override { copy_hd(z,q,sz); }
	virtual void   readSectors(LBA, void* bu, SIZE) throws override;
	virtual void   writeSectors(LBA, const void* bu, SIZE) throws override;
	virtual uint32 ioctl(IoCtl, void* arg1 = nullptr, void* arg2 = nullptr) throws override;

	void printSCR(SerialDevice*, bool v = 1);
	void printCID(SerialDevice*, bool v = 1);
	void printCSD(SerialDevice*, bool v = 1);
	void printOCR(SerialDevice*, bool v = 1);
	void printCardInfo(SerialDevice*, bool v = 1);

private:
	SDCard() noexcept;
	void init_spi() noexcept;

	void connect() throws;
	void disconnect() noexcept;

	inline void select() const noexcept;
	inline void deselect() const noexcept;
	inline void read_spi(uint8*, uint32) const noexcept;
	inline void write_spi(const uint8*, uint32) const noexcept;

	uint8  read_byte() noexcept;
	uint8  receive_byte_or_throw(int timeout_us) throws;
	void   wait_ready_or_throw() throws;
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

	void __attribute__((noreturn)) throwDeviceNotResponding();
	void __attribute__((noreturn)) throwWriteDataErrorToken(uint8 n);
	void __attribute__((noreturn)) throwReadDataErrorToken(uint8 n);
	void __attribute__((noreturn)) throwR1ResponseError(uint8 r1);
};

} // namespace kio::Devices

/*



















*/
