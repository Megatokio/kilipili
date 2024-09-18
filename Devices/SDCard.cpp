// Copyright (c) 2021 - 2024 kio@little-bat.de
// BSD 2-clause license
// https://spdx.org/licenses/BSD-2-Clause.html

#include "SDCard.h"
#include "basic_math.h"
#include "cdefs.h"
#include "crc.h"
#include "stdio.h"
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <pico/stdlib.h>
#include <string.h>

#undef debugstr
#define debugstr(...) void(0)


static constexpr uint log2(uint n) { return n == 1 ? 0 : 1 + log2(n >> 1); }
static_assert(log2(32) == 5, "");


// clang-format off
// assert unchanged definition, then replace c-style casting macros with c++ version:
#define spi0_hw ((spi_hw_t *)SPI0_BASE)
#define spi1_hw ((spi_hw_t *)SPI1_BASE)
#define spi0 ((spi_inst_t *)spi0_hw)
#define spi1 ((spi_inst_t *)spi1_hw)
#undef spi0_hw
#undef spi1_hw
#undef spi0
#undef spi1
#define spi0_hw reinterpret_cast<spi_hw_t*>(SPI0_BASE)
#define spi1_hw reinterpret_cast<spi_hw_t*>(SPI1_BASE)
#define spi0 reinterpret_cast<spi_inst_t*>(spi0_hw)
#define spi1 reinterpret_cast<spi_inst_t*>(spi1_hw)
// clang-format on

// some CRCs:
// CMD0  GOTO_IDLE_STATE   0x95
// CMD8  SEND_IF_COND      0x87
// CMD55 APPLICATION PFX   0x65
// CMD41 arg=0x40000000    0x77
// CMD59 arg=0x00000001    0x83
// CMD12 STOP_TRANSMISSION 0x61
// CMD13 SEND_STATUS       0x0d


__attribute__((weak)) void set_disk_light(bool onoff)
{
#ifdef PICO_DEFAULT_LED_PIN
	gpio_put(PICO_DEFAULT_LED_PIN, onoff);
#endif
}


namespace kio::Devices
{


// ==================================================
//			SD Card Block Device
// ==================================================


inline void SDCard::read_spi(uint8* data, uint32 cnt) const noexcept { spi_read_blocking(spi, 0xff, data, cnt); }
inline void SDCard::write_spi(const uint8* data, uint32 cnt) const noexcept { spi_write_blocking(spi, data, cnt); }
inline void SDCard::select() const noexcept
{
	set_disk_light(on);
	gpio_put(cs_pin, 0);
}
inline void SDCard::deselect() const noexcept
{
	set_disk_light(off);
	gpio_put(cs_pin, 1);
	static uint8 u1;
	read_spi(&u1, 1); // flush card's shift register (!SanDisk!)
}

static spi_inst* inst_for_pin(uint pin) { return pin < 20 ? pin & 8 ? spi1 : spi0 : nullptr; }
static bool		 is_rx_pin(uint pin) { return (pin & 3) == 0; }
static bool		 is_clk_pin(uint pin) { return (pin & 3) == 2; }
static bool		 is_tx_pin(uint pin) { return (pin & 3) == 3; }

SDCard::SDCard(uint8 rx, uint8 cs, uint8 clk, uint8 tx) noexcept :
	BlockDevice(0, 9, 9, 9 /*ss*/, readwrite /*flags*/),
	spi(inst_for_pin(rx)),
	rx_pin(rx),
	cs_pin(cs),
	clk_pin(clk),
	tx_pin(tx)
{
	assert(inst_for_pin(rx) == inst_for_pin(tx));
	assert(inst_for_pin(rx) == inst_for_pin(clk));
	assert(is_rx_pin(rx));
	assert(is_tx_pin(tx));
	assert(is_clk_pin(clk));

	init_spi();
}

void SDCard::init_spi() noexcept
{
	// chip select CSn:
	gpio_init(cs_pin);
	gpio_put(cs_pin, 1); // deselect
	gpio_set_dir(cs_pin, GPIO_OUT);

	// spi:
	spi_init(spi, 20 * 1000 * 1000);
	spi_set_format(spi, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
	gpio_set_function(rx_pin, GPIO_FUNC_SPI);
	gpio_pull_up(rx_pin);
	gpio_set_function(clk_pin, GPIO_FUNC_SPI);
	gpio_set_function(tx_pin, GPIO_FUNC_SPI);
}

void SDCard::disconnect() noexcept
{
	deselect();
	//spi_deinit(spi);
	sector_count = 0;
	flags		  = Flags(0);
	card_type	  = SD_unknown;
}


enum R1Response {
	IdleState		   = 1, // initializing
	EraseReset		   = 2, // non-erase cmd in erase cmd sequence
	IllegalCommand	   = 4,
	CommandCrcError	   = 8,
	EraseSequenceError = 16, // erase commands in wrong order
	AddressError	   = 32,
	ParameterError	   = 64
};
enum Token {
	DataToken		= 0xff - 1, // in CMD17,18,24
	DataToken25		= 0xff - 3, // in CMD25
	StopTranToken25 = 0xff - 2, // in CMD25
};
enum WriteDataResponseToken {
	// response sent by Card after receiving write data block:

	DataResponseMask = 0b00011111, // only lower 5 bits
	DataAccepted	 = 0b00000101,
	DataCrcError	 = 0b00001011,
	DataWriteError	 = 0b00001101
};
enum ReadDataErrorToken {
	// sent by Card instead of the expected DataToken:

	ErrorError	 = 0b00000001, // may be ORed
	CC_Error	 = 0b00000010,
	ECC_Failed	 = 0b00000100,
	RangeError	 = 0b00001000,
	CardIsLocked = 0b00010000,
};

static void __attribute__((noreturn)) throwWriteDataErrorToken(uint8 n)
{
	if (n == 0xff) throw DEVICE_NOT_RESPONDING;
	n &= DataResponseMask;
	if (n == DataCrcError) throw CRC_ERROR;
	if (n == DataWriteError) throw HARD_WRITE_ERROR;
	throw DEVICE_INVALID_RESPONSE;
}

static void __attribute__((noreturn)) throwReadDataErrorToken(uint8 n)
{
	if (n == 0xff) throw DEVICE_NOT_RESPONDING;
	if (n == RangeError) throw INVALID_ARGUMENT;
	if (n == ECC_Failed) throw HARD_READ_ERROR;
	if (n == CC_Error) throw CONTROLLER_FAILURE;
	if (n == ErrorError) throw UNKNOWN_ERROR;
	// if (n&CardIsLocked) throw NOT_WRITABLE;
	throw DEVICE_INVALID_RESPONSE;
}

void __attribute__((noreturn)) throwR1ResponseError(uint8 r1)
{
	if (r1 == 0xff) throw DEVICE_NOT_RESPONDING;
	r1 &= ~IdleState;
	if (r1 == IllegalCommand) throw ILLEGAL_COMMAND;
	if (r1 == CommandCrcError) throw CRC_ERROR;
	if (r1 == AddressError) throw ADDRESS_ERROR;
	if (r1 == ParameterError) throw INVALID_ARGUMENT;
	if (r1 == EraseReset || r1 == EraseSequenceError) throw ERASE_CMD_ERROR;
	throw DEVICE_INVALID_RESPONSE; // 0x80 or 0x01
}

__unused static void dump(uint8* bu, uint count) noexcept
{
	for (uint i = 0; i < count; i++)
	{
		if (i % 8 == 0)
		{
			if (i % 32 == 0) printf("\n  ");
			else printf(" ");
		}
		if (bu[i] == 0xff) printf("__");
		else printf("%02x", bu[i]);
	}
	printf("\n");
}

inline uint8 SDCard::read_byte() noexcept
{
	uint8 byte;
	read_spi(&byte, 1);
	return byte;
}

uint8 SDCard::receive_byte(uint retry) noexcept
{
	// receive byte with retry count
	// for 10MHz retry count ~ timeout in µs.

	uint8 byte;
	do {
		read_spi(&byte, 1);
	}
	while (byte == 0xff && retry--);
	return byte;
}

bool SDCard::wait_ready(uint retry) noexcept
{
	// wait for card.DO = 1 with timeout
	// uses timeout value as retry count. for 10MHz this is similar to µs.

	do {
		if (read_byte() == 0xff) return true;
	}
	while (--retry);
	deselect();
	return false; // not ready
}

void SDCard::select_and_wait_ready(uint __unused retry) throws
{
	// wait for card.DO = 1 with timeout
	// uses timeout value as retry count. for 10MHz this is similar to µs.

	select();
	if (wait_ready()) return;
	else throw TIMEOUT;
}

uint8 SDCard::send_cmd(uint8 cmd, uint32 arg, uint flags) throws
{
	// send command, receive R1, retry or throw

	// SanDisk loses byte synchronization in acmd
	// therefore it is essential to always deselect + select before sending commands

	bool  is_acmd = flags & f_acmd;
	bool  keep_on = flags & no_deselect;
	uint8 r1_mask = flags & f_idle ? 0xff - IdleState : 0xff;

	if (arg) debugstr("{%scmd%u %08x} ", is_acmd ? "a" : "", cmd, arg);
	else debugstr("{%scmd%u} ", is_acmd ? "a" : "", cmd);

	uint8 bu2[6];
	bu2[0] = 0x40 | cmd;
	poke_u32(bu2 + 1, arg);
	bu2[5] = uint8(crc7(bu2, 5));

	uint8 retry = 1;
a:
	select_and_wait_ready();
	uint8 r1;
	if (is_acmd)
	{
		static const uint8 bu1[6] {0x40 | 55, 0, 0, 0, 0, 0x65}; // CMD55
		write_spi(bu1, 6);
		r1 = receive_byte();
		deselect(); // <-- required for SanDisk ... don't buy again.
		if ((r1 & r1_mask) != 0) goto x;
		select();
	}
	write_spi(bu2, 6);
	r1 = receive_byte();

	if (!keep_on || (r1 & r1_mask) != 0) deselect();
	if ((r1 & r1_mask) == 0) return r1;
x:
	if ((r1 & r1_mask) == CommandCrcError && retry--) goto a;
	throwR1ResponseError(r1);
}

void SDCard::initialize_card_and_wait_ready() throws
{
	// ACMD41: send host supports SDHC and wait card ready
	// must be sent after CMD8 which enabled SDHC support in the card itself.

	// Tell card that host supports SDHC mode (arg.bit30=1) and start card initialization.
	// This command must be repeated as long as the car returns R1 = 0x01 = IdleState.
	// This may take several seconds. (esp. for SanDisk!)

	// For SDUC cards (>2TB), card may indicate that the card is still initializing in the
	// R3 response of ACMD41 continuously to let host know SDUC card cannot use SPI mode.

	for (uint start = time_us_32(); time_us_32() - start < 5 * 1000 * 1000;)
	{
		uint8 r1 = send_acmd(41, 0x40000000, f_idle);
		if (r1 == 0) return;
		sleep_ms(5);
	}
	throw TIMEOUT;
}

uint16 SDCard::read_status(bool keep_selected) noexcept
{
	// CMD13 returns R2 response
	// note: R1 is in the LSB, the 2nd byte is in the MSB
	// cmd13 can be issued while card is holding DO low

	debugstr("{CMD13} ");
	static const uint8 cmd13[6] {0x40 | 13, 0, 0, 0, 0, 0x0D};
	select();
	write_spi(cmd13, 6);
	uint8 r1 = receive_byte(100);
	uint8 r2 = receive_byte(0);
	if (!keep_selected) deselect();
	return r1 + r2 * 256;
}

void SDCard::read_ocr() throws
{
	// READ_OCR (CMD58): After initialization is completed, the host should get
	// CCS information in the response of CMD58. CCS is valid when the card accepted CMD8
	// and after the completion of initialization. CCS=0 means that the card is SDSD.
	// CCS=1 means that the card is SDHC or SDXC.
	//
	// "OCR" = Operation Condition Register
	// ocr.bit30 = CCS = "Card Capacity Status"
	// if CCS bit in OCR == 1: => SD v2+ with block addresses, block size = 512 fixed
	// if CCS bit in OCR == 0: => SD v1 or v2+ with byte addresses

	uint8 bu[4];
	send_cmd(58, 0, no_deselect);
	read_spi(bu, 4);
	deselect();
	ocr = peek_u32(bu);
}

void SDCard::read_scr()
{
	// ACMD51
	// the only interesting data in here is the value of erased bits

	send_acmd(51, 0, no_deselect);
	uint8 bu[10];
	uint8 token = receive_byte(10000);
	read_spi(bu, 10);
	deselect();
	if (token != DataToken) throwReadDataErrorToken(token);
	if (crc16(bu, 8) != peek_u16(bu + 8)) debugstr("crc_error "); //xxx throw CRC_ERROR;
	erased_byte = bu[1] & 0x80 ? 0xff : 0x00;
}

void SDCard::read_card_info(uint8 cmd) throws
{
	assert(cmd == 9 || cmd == 10);

	// Reading CSD (CMD9) and CID (CMD10)
	// These are same as Single Block Read except for the data block length.
	// The CSD and CID are sent to the host as 16 byte data block.

	send_cmd(cmd, 0, no_deselect);
	uint8 token = receive_byte();
	if (token != DataToken)
	{
		deselect();
		throwReadDataErrorToken(token);
	}
	uint8 bu[18];
	read_spi(bu, 18);
	deselect();

	if (crc16(bu, 16) != peek_u16(bu + 16))
	{
		// attn: intenso year 2023 4GB sdcards return crc = 0x00 and 0x0000
		debugstr("crc16 failed (%04x != %04x)\n", crc16(bu, 16), peek_u16(bu + 16));
		if (peek_u16(bu + 16) || bu[15]) throw CRC_ERROR; // TODO: retry once
	}
	if (crc7(bu, 15) != bu[15])
	{
		// attn: intenso year 2023 4GB sdcards return crc = 0x00 and 0x0000
		debugstr("crc7 failed (%02x != %02x)", crc7(bu, 15), bu[15]);
		if (peek_u16(bu + 16) || bu[15]) throw CRC_ERROR; // TODO: retry once
	}

	// store data but take care for our little Endian:
	if (cmd == 9)
	{
		for (uint i = 0; i < 4; i++) { csd.data[i] = peek_u32(bu + 4 * i); }
	}
	else { memcpy(&cid, bu, 16); }
}

void SDCard::connect() throws
{
	printf("SDCard::connect\n");

	// attach to a card
	// connect() may take several seconds if called directly after inserting the card

	//while (time_us_32() < 1000) {}		// delay after power-up

	//deselect();
	//spi_set_baudrate(spi, 400*1000); // MMC=400kHz, SDCard=25MHz, cable length~10MHz

	try
	{
		uint8 r1	= 0;
		uint  retry = 3; // => 2 retries
	retry:
		deselect();
		if (retry-- == 0)
		{
			debugstr("r1=%02x ", r1);
			if (r1 == 0xff) throw DEVICE_NOT_RESPONDING;
			throw DEVICE_INVALID_RESPONSE;
		}

		// send 80 clock pulses with CS=1 and DI=1
		// flush card's shift register and fifo
		uint8 bu[10];
		read_spi(bu, 10);

		// CMD0 GOTO_IDLE_STATE:
		// CRC must be valid.
		// The SD Card will enter SPI mode if the CS signal is asserted during this command.
		// expected response: R1 = 0x01 = IdleState

		r1 = send_cmd(0x00, 0, f_idle);
		if (r1 != IdleState) goto retry;

		// CMD8 SEND_IF_COND:
		// CRC must be valid.
		// used to activate SDHC support and to distinguish between SD v1 and SD v2.
		// Also check support for 2.7 - 3.6V supply voltage, but 3.3V is required for SD cards.
		// argument: [11:8] supply voltage "VHS": 0b0001 = 2.7-3.6V
		//           [7:0]  check pattern: 0xAA
		// expected response: R7 = R1 + 0x000001AA = same as sent

		debugstr("{cmd8} ");
		select();
		//wait_ready(1000);
		static const uint8 cmd8[6] {8 | 0x40, 0, 0, 1, 0xAA, 0x87};
		write_spi(cmd8, 6);
		r1 = receive_byte();
		if (r1 == IdleState)
		{
			read_spi(bu, 4);
			if (bu[3] != 0xAA || bu[2] != 1) goto retry;
			deselect();
			debugstr("SD_2x ");
			card_type = SD_v2;
		}
		else if (r1 == IdleState + IllegalCommand)
		{
			deselect();
			debugstr("SD_1x ");
			card_type = SD_v1;
		}
		else goto retry;

		// CMD58 READ_OCR:
		// read OCR to check voltage
		// note: 3.3V support is required for SD cards
		//read_ocr(IdleState);

		// CMD59 CRC_ON_OFF:
		// enable CRC
		send_cmd(59, 0, f_idle);

		// ACMD41 SD_SEND_OP_COND, wait until card left IdleState:
		debugstr("\nwait_ready ");
		initialize_card_and_wait_ready();

		// CMD58 READ_OCR:
		// read OCR to get CCS "Card Capacity Status".
		read_ocr();
		ccs = (ocr >> 30) & 1;
		if (ccs)
		{
			if (card_type == SD_v1) throw DEVICE_INVALID_RESPONSE;
			card_type = SDHC_v2;
			debugstr("SDHC_2x ");
		}
		else
		{
			// SDHC and SDXC use fixed block size = 512 bytes.
			// Set block size to 512 for SCSD card as well:
			send_cmd(16, 512);
		}

		// get card specific data CSD:
		read_card_info(9);

		// set spi to higher speed
		// high speed not guaranteed in SPI mode: => 25MHz max
		// but use 10MHz wg. cable anyway
		//spi_set_baudrate(spi,10*1000*1000);

		// get card identification CID and test working at baudrate:
		read_card_info(10);

		// get value of erased bits
		read_scr();

		ss_read = ss_write = 9;
		if (1 << ss_read != csd.erase_sector_size()) throw DEVICE_NOT_SUPPORTED;
		//if (ss != csd.read_bl_bits()) throw DEVICE_NOT_SUPPORTED;
		//if (ss != csd.write_bl_bits()) throw DEVICE_NOT_SUPPORTED;
		flags = csd.write_prot() ? partition | readable : partition | readwrite | overwritable;

		debugstr("\n");
		uint64 total_size = csd.disk_size();
		sector_count	  = SIZE(total_size >> ss_write);
		if (ADDR(total_size) != total_size) debugstr("WARNING: disk size >= 4GB\n");
		debugstr("ready\n");
	}
	catch (cstr& e)
	{
		printf("exception: %s\n", e);
		disconnect();
		throw e;
	}
	catch (...)
	{
		printf("other exception\n");
		disconnect();
		throw;
	}
}

void SDCard::set_blocklen(uint blen) throws
{
	// CMD16
	send_cmd(16, blen);
}

void SDCard::read_single_block(uint32 blkidx, uint8* data) throws
{
	// CMD17: read single block

	uint retry = 1;
r:
	uint8 crc[2];
	send_cmd(17, ccs ? blkidx : blkidx << 9, no_deselect);
	uint8 token = receive_byte(10000);
	if (token != DataToken)
	{
		deselect();
		throwReadDataErrorToken(token);
	}
	read_spi(data, 512);
	read_spi(crc, 2);
	deselect();
	if (crc16(data, 512) == peek_u16(crc)) return;
	debugstr("crc_error ");
	if (retry--) goto r;
	throw CRC_ERROR;
}

void SDCard::write_single_block(uint32 blkidx, const uint8* data) throws
{
	// CMD24: write single block

	uint retry = 1;
r:
	uint8 crc[2];
	uint8 token = DataToken;
	poke_u16(crc, crc16(data, 512));
	send_cmd(24, ccs ? blkidx : blkidx << 9, no_deselect);
	wait_ready();
	write_spi(&token, 1);
	write_spi(data, 512);
	write_spi(crc, 2);
	token = receive_byte(10000) & DataResponseMask;
	deselect();
	if (token == DataAccepted) return;
	if (token == DataCrcError && retry--) goto r;
	throwWriteDataErrorToken(token);
}

void SDCard::stop_transmission() noexcept
{
	// CMD12
	// The received byte immediately following CMD12 is a stuff byte,
	// it should be discarded prior to receive the response of the CMD12.
	// For MMC, if number of transfer blocks has been specified by a CMD23 prior to CMD18,
	// the read transaction is initiated as a pre-defined multiple block transfer
	// and the read operation is terminated at last block transfer.

	debugstr("{cmd12} ");
	//select();
	static const uint8 cmd[8] = {12 | 0x40, 0, 0, 0, 0, 0x61, 0xff, 0xff};
	uint8			   rx[8];
	spi_write_read_blocking(spi, cmd, rx, 8);
	deselect();
	//dump(rx,8);	// -> FExxxxxxxxxx7F00	(xx = data from sector)
	uint8 r1 = rx[7];
	if (r1 != 0) printf("\nERROR: cmd12 r1=0x%02x  ", r1);
}

void SDCard::readSectors(LBA blkidx, void* data, SIZE blkcnt) throws
{
	// CMD18: read multiple blocks

	uchar* udata = reinterpret_cast<uchar*>(data);
	if (blkcnt == 1) return read_single_block(blkidx, udata);

	uint retry = blkcnt + 1;
r:
	send_cmd(18, ccs ? blkidx : blkidx << 9, no_deselect);

	while (blkcnt)
	{
		uint8 crc[2];
		uint8 token = receive_byte(10000);
		if (token != DataToken)
		{
			stop_transmission();
			throwReadDataErrorToken(token);
		}
		read_spi(udata, 512);
		read_spi(crc, 2);
		if (crc16(udata, 512) == peek_u16(crc))
		{
			blkidx++;
			udata += 512;
			blkcnt--;
			continue;
		}
		stop_transmission();
		debugstr("crc_error ");
		if (retry--) goto r;
		else throw CRC_ERROR;
	}

	stop_transmission();
}

void SDCard::writeSectors(LBA blkidx, const void* data, SIZE blkcnt) throws
{
	// CMD25: write multiple blocks

	//  For MMC, the number of blocks to write can be pre-defined by CMD23 prior to CMD25
	// and the write transaction is terminated at last data block.
	// For SDC, a StopTran token is always required to treminate the multiple block write transaction.
	// Number of sectors to pre-erased at start of the write transaction can be specified
	// by an ACMD23 prior to CMD25. It may able to optimize write strategy in the card
	// and it can also be terminated not at the pre-erased blocks but the content of the
	// pre-erased area not written will get undefined.

	if unlikely (data == nullptr) // erase: CMD32,33,38
	{
		// As for block write, the card will indicate that an erase is in progress by holding DAT0 low.
		// The actual erase time may be quite long, and the host may deselect the card.
		// The data at the card after an erase operation is either '0' or '1'.
		// The SCR register bit DATA_STAT_AFTER_ERASE (bit 55) defines whether it is '0' or '1'.
		send_cmd(32, ccs ? blkidx : blkidx << 9);
		send_cmd(33, ccs ? blkidx + blkcnt - 1 : (blkidx + blkcnt - 1) << 9);
		send_cmd(38);
		return;
	}

	const uchar* udata = reinterpret_cast<const uchar*>(data);
	uint		 retry = blkcnt + 1;
r:
	send_acmd(23, blkcnt); // pre erase blocks
	send_cmd(25, ccs ? blkidx : blkidx << 9, no_deselect);

	while (blkcnt)
	{
		uint8 crc[2];
		uint8 token = DataToken25;
		poke_u16(crc, crc16(udata, 512));

		wait_ready();
		write_spi(&token, 1);
		write_spi(udata, 512);
		write_spi(crc, 2);

		token = receive_byte(10000) & DataResponseMask;
		if (token == DataAccepted)
		{
			blkidx++;
			udata += 512;
			blkcnt--;
			continue;
		}

		uint8 bu[2] = {StopTranToken25, 0xff};
		write_spi(bu, 2);
		deselect();
		if (token == DataCrcError && retry--) goto r;
		throwWriteDataErrorToken(token);
	}

	uint8 bu[2] = {StopTranToken25, 0xff};
	write_spi(bu, 2);
	deselect();
}

#if 0
void SDCard::read(ADDR qpos, uint8* data, SIZE size)
{
	if (uint(qpos) & 511 || size < 512)
	{
		uint8 bu[512];
		read_single_block(uint32(qpos >> 9), bu);
		SIZE cnt = min(size, 512u - (SIZE(qpos) & 511));
		memcpy(data, bu + (qpos & 511), cnt);
		qpos += cnt;
		data += cnt;
		size -= cnt;
	}

	if (size >= 512)
	{
		SIZE cnt = size & ~511u;
		readSectors(BLKIDX(qpos >> 9), ptr(data), SIZE(cnt >> 9));
		qpos += cnt;
		data += cnt;
		size -= cnt;
	}

	if (size)
	{
		uint8 bu[512];
		read_single_block(uint32(qpos >> 9), bu);
		memcpy(data, bu, size);
	}
}
#endif

#if 0
void SDCard::write(ADDR zpos, const uint8* data, SIZE size)
{
	if (uint(zpos) & 511 || size < 512)
	{
		uint8 bu[512];
		read_single_block(uint32(zpos >> 9), bu);
		SIZE cnt = min(size, 512u - (SIZE(zpos) & 511));
		if (data)
		{
			memcpy(bu + (zpos & 511), data, cnt);
			data += cnt;
		}
		else memset(bu + (zpos & 511), erased_byte, cnt);
		write_single_block(uint32(zpos >> 9), bu);
		zpos += cnt;
		size -= cnt;
	}

	if (size >= 512)
	{
		SIZE cnt = size & ~511u;
		writeSectors(BLKIDX(zpos >> 9), ptr(data), SIZE(cnt >> 9));
		zpos += cnt;
		if (data) data += cnt;
		size -= cnt;
	}

	if (size)
	{
		uint8 bu[512];
		read_single_block(uint32(zpos >> 9), bu);
		if (data) memcpy(bu, data, size);
		else memset(bu, erased_byte, size);
		write_single_block(uint32(zpos >> 9), bu);
	}
}
#endif

uint32 SDCard::ioctl(IoCtl ctl, void* a1, void* a2) throws
{
	switch (ctl.cmd)
	{
	case IoCtl::CTRL_CONNECT: // connect to hardware, load removable disk
		connect();			  // throws
		print_card_info();
		return 0;
	case IoCtl::CTRL_DISCONNECT: // disconnect from hardware, unload removable disk
		disconnect();
		return 0;
	default: //
		return BlockDevice::ioctl(ctl, a1, a2);
	}
}

static inline bool is_ascii(char c) { return c >= 32 && c <= 126; }
static inline char hexchar(uint8 n)
{
	n &= 15;
	return n > 9 ? 'A' + n - 10 : '0' + n;
}
static inline cstr tostr(bool f) { return f ? "YES" : "NO"; }

void SDCard::print_scr(uint /*verbose*/)
{
	// SCR Structure				     4  [63:60]  all 0
	// SD Memory Card - Spec. Version    4  [59:56]  all 2
	// data_status_after erases		     1  [55:55]  1/0/0/0
	// CPRM Security Support		     3  [54:52]  all 3
	// DAT Bus widths supported		     4  [51:48]  all 5 = 4 + 1
	// Spec. Version 3.00 or higher	     1  [47]     all 1
	// Extended Security Support	     4  [46:43]  all 0
	// Spec. Version 4.00 or higher	     1  [42]     0/1/0/0
	// Spec. Version 5.00 or higher	     4  [41:38]  all 0
	// Reserved						     2  [37:36]  all 0
	// Command Support bits			     4  [35:32]  0/3/3/0		pg.206
	// reserved for manufacturer usage   32 [31:0]

	// Intenso 4GB:  02b58000 00000000
	// SanDisk 16GB: 02358043 00000000
	// SanDisk 16GB: 02358003 00000000
	// Kingston 4GB: 02358000 00000000

	printf("\nSCR: SD Card Configuration Register\n");
	//if (verbose) printf("%08X\n",scr);
	printf("  Erased data value:  0x%02X\n", erased_byte);
}

void SDCard::print_ocr(uint v)
{
	/* OCR Register
	The 32-bit Operation Conditions Register stores the V DD voltage profile and 2 status bits.

		[0:23] Voltage Info:
			[0:14] reserved, 7: reserved for LowVoltage range
			15: 2.7 - 2.8V
			16: 2.8 -
			17: 2.9 -
			18: 3.0 -
			19: 3.1 -
			20: 3.2 -
			21: 3.3 -
			22: 3.4 -
			23: 3.5 - 3.6V
		24: 1.8V support
		27: over 2TB support
		29: UHS-II card status
		30: Card Capacity Status "CCS": valid if bit31=1. 0: SDSC card. 1: SDHC/SDXC/SDUC card.
		31: Card power up status: set if the card power up procedure has been finished.
	*/

	printf("\nOCR: Operation Condition Register\n");

	if (v) printf("%08X\n", ocr);

	if ((ocr & 0x00ff8000) == 0) { printf("  Voltage range 2.7 .. 3.6V: not supported %%-)\n"); }
	else
	{
		static const char v[10][4] = {"2.7", "2.8", "2.9", "3.0", "3.1", "3.2", "3.3", "3.4", "3.5", "3.6"};
		uint			  a		   = 15;
		while ((ocr & (1 << a)) == 0) a++;
		uint e = 23;
		while ((ocr & (1 << e)) == 0) e--;
		printf("  Voltage range: %s .. %sV\n", v[a - 15], v[e - 14]);
	}
	printf("  Switching to 1.8V accepted: %s\n", ocr & (1 << 24) ? "YES" : "NO");
	printf("  Over 2TB support:           %s\n", ocr & (1 << 27) ? "YES" : "NO");
	if (v || ocr & (1 << 29)) printf("  UHS-II Card Status:         %s\n", ocr & (1 << 29) ? "YES" : "NO");
	printf(
		"  Card Capacity Status CCS:   %s\n", ocr & (1 << 30) ? "YES (block address mode)" : "NO (byte address mode)");
	if (v || !ocr >> 31) printf("  Card powered up:            %s\n", ocr >> 31 ? "YES" : "NO");
}

void SDCard::print_cid(uint v)
{
	/* CID Register
	The Card IDentification Register is 128 bits wide. It contains the card identification
	information used during the card identification phase. Every individual Read/Write (RW)
	card shall have a unique identification number.

		[127:120] 8  MID: Manufacturer ID
		[119:104] 16 OID: OEM/Application ID
		[103:64]  40 PNM: Product name: ascii_char[5]
		[63:56]   8  PRV: Product revision: bcd_digits[2] -> x.y
		[55:24]   32 PSN: Product serial number: uint32
		[19:8]    12 MDT: Manufacturing date: hex_nibbles[3] -> yy.m
		[7:1]     7  CRC: crc7 checksum
		[0:0]     1       always 1
	*/

	printf("\nCID: Card Identification\n");

	if (v)
	{
		uint8* p = uint8ptr(&cid);
		for (uint i = 0; i < 16; i++) { printf("%02X", *p++); }
		printf("\n");
	}

	char pnm[6] = "?????";
	memcpy(pnm, cid.pnm, 5);
	for (uint i = 0; i < 5; i++)
		if (!is_ascii(pnm[i])) pnm[i] = '?';
	char prv[4] = "x.y";
	prv[0]		= hexchar(cid.prv >> 4);
	prv[2]		= hexchar(cid.prv);

	// The manufacturing date is composed of two hexadecimal digits,
	// one is 8 bits representing the year(y) and the other is 4 bits representing the month (m).
	// The "m" field [11:8] is the month code. 1 = January.
	// The "y" field [19:12] is the year code. 0 = 2000.

	uint mdt = peek_u16(cid.mdt);
	uint y	 = uint8(mdt >> 4);
	uint m	 = mdt & 15;

	printf("  MID: Manufacturer     %u\n", cid.mid);
	printf("  OID: OEM/Application  %u\n", peek_u16(cid.oid));
	printf("  PNM: Product Name     %s\n", pnm);
	printf("  PRV: Product Revision %s\n", prv);
	printf("  PSN: Prod. Serial No. %u\n", peek_u32(cid.psn));
	printf("  MDT: Manufactured     20%02u/%02u (0x%03x)\n", y, m, mdt & 0xfff);
}

void SDCard::print_csd(uint v)
{
	/* CSD Register
	The Card-Specific Data register provides information regarding access to the card contents.
	The CSD defines the data format, error correction type, maximum data access time, whether
	the DSR register can be used, etc. The programmable part of the register can be changed by CMD27.

	Field structures of the CSD register are different depend on the Physical Layer Specification
	Version and Card Capacity. The CSD_STRUCTURE field indicates its structure version.
		CSD  Version  Capacity
		0    1.0      Standard Capacity
		1    2.0      High Capacity and Extended Capacity
		2    3.0      Ultra Capacity (SDUC) (does not support SPI!)
		3    reserved
	*/

	printf("\nCSD: Card-Specific Data\n");

	if (v)
	{
		for (uint i = 0; i < 4; i++) printf("%08X", csd.data[i]);
		printf("\n");
	}

	static const cstr cccs[12] = {
		"Basic",	 "Comm and Queue",		 "Block read", "res.",	 "Block write", "Erase", "Write protection",
		"Lock card", "Application specific", "I/O mode",   "Switch", "Extension"};

	uint rat10us	 = csd.read_access_time_us(10 * 1000 * 1000); // @10MHz in µs
	uint ccc		 = csd.ccc();
	uint csd_version = csd.csd_structure() + 1;

	printf("  CSD Structure:             Version %u\n", csd_version);
	printf("  Disk size                  %u MB\n", uint(csd.disk_size() / 1000000));
	printf("  max. data clock:           %u MHz\n", csd.max_clock() / 1000000);
	printf("  read access time at 10MHz: %u ms\n", rat10us / 1000);
	printf("  r2w time factor:           %u\n", csd.r2w_factor());
	printf("  supported card command classes:\n");
	for (uint i = 0; i < 12; i++)
	{
		if (ccc & (1 << i)) printf("     %2u: %s\n", i, cccs[i]);
	}
	printf("  DSR implemented            %s\n", tostr(csd.dsr_imp()));

	printf("  read block length:         %u\n", 1 << csd.read_bl_bits());
	printf("  write block length:        %u\n", 1 << csd.write_bl_bits());
	printf("  read block partial:        %s\n", tostr(csd.read_bl_partial()));
	printf("  write block partial:       %s\n", tostr(csd.write_bl_partial()));
	printf("  read accross block bounds  %s\n", tostr(csd.read_bl_misalign()));
	printf("  write accross block bounds %s\n", tostr(csd.write_bl_misalign()));
	printf("  erase per block enabled    %s\n", tostr(csd.erase_blk_en()));
	printf("  erase sector size          %u\n", csd.erase_sector_size());
	if (csd_version == 1)
	{
		printf("  wprot group size           %u\n", reinterpret_cast<CSDv1&>(csd).wp_grp_size());
		printf("  wprot groups enabled       %s\n", tostr(reinterpret_cast<CSDv1&>(csd).wp_grp_enable()));
	}

	printf("  this disk is a copy        %s\n", tostr(csd.copy()));
	printf("  permanent write protection %s\n", tostr(csd.perm_write_prot()));
	printf("  temporary write protection %s\n", tostr(csd.tmp_write_prot()));
}

void SDCard::print_card_info(uint v)
{
	//	SD_unknown,
	//	SD_v1,		// standard capacity SD card (16MB..2GB)
	//	SD_v2,		// standard capacity SD card (16MB..2GB)
	//	SDHC_v2,	// SDHC high capacity /2GB..32GB) or SDXC extended capacity (32GB..2TB)
	//	SDUC_v3,	// SDUC ultra capacity SD card (2TB..128TB) (no SPI support!)
	//	MMC,		// need 400kHz for initialization, different CSD and CID

	static const cstr card_type_str[] = {
		"no card",
		"SCSD standard capacity card, CSDv1",
		"SCSD standard capacity card, CSDv2",
		"SDHC high capacity card, CSDv2",
		"SDUC ultra capacity card, CSDv3 (no SPI - not supported!)",
		"MMC Multimedia card - not supported"};

	printf("\nCard type = %s\n", card_type_str[card_type]);
	print_ocr(v);
	print_cid(v);
	print_csd(v);
	print_scr(v);
	printf("\n");
}

} // namespace kio::Devices
