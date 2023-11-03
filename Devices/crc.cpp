#include "crc.h"

// ==================================================
//			CRC7 and CRC16 checksums
// ==================================================

#if defined(FAST_CRC7) && FAST_CRC7

static const u8 crc7_table[256] = {
	0x00, 0x12, 0x24, 0x36, 0x48, 0x5a, 0x6c, 0x7e, 0x90, 0x82, 0xb4, 0xa6, 0xd8, 0xca, 0xfc, 0xee, 0x32, 0x20, 0x16,
	0x04, 0x7a, 0x68, 0x5e, 0x4c, 0xa2, 0xb0, 0x86, 0x94, 0xea, 0xf8, 0xce, 0xdc, 0x64, 0x76, 0x40, 0x52, 0x2c, 0x3e,
	0x08, 0x1a, 0xf4, 0xe6, 0xd0, 0xc2, 0xbc, 0xae, 0x98, 0x8a, 0x56, 0x44, 0x72, 0x60, 0x1e, 0x0c, 0x3a, 0x28, 0xc6,
	0xd4, 0xe2, 0xf0, 0x8e, 0x9c, 0xaa, 0xb8, 0xc8, 0xda, 0xec, 0xfe, 0x80, 0x92, 0xa4, 0xb6, 0x58, 0x4a, 0x7c, 0x6e,
	0x10, 0x02, 0x34, 0x26, 0xfa, 0xe8, 0xde, 0xcc, 0xb2, 0xa0, 0x96, 0x84, 0x6a, 0x78, 0x4e, 0x5c, 0x22, 0x30, 0x06,
	0x14, 0xac, 0xbe, 0x88, 0x9a, 0xe4, 0xf6, 0xc0, 0xd2, 0x3c, 0x2e, 0x18, 0x0a, 0x74, 0x66, 0x50, 0x42, 0x9e, 0x8c,
	0xba, 0xa8, 0xd6, 0xc4, 0xf2, 0xe0, 0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x70, 0x82, 0x90, 0xa6, 0xb4, 0xca,
	0xd8, 0xee, 0xfc, 0x12, 0x00, 0x36, 0x24, 0x5a, 0x48, 0x7e, 0x6c, 0xb0, 0xa2, 0x94, 0x86, 0xf8, 0xea, 0xdc, 0xce,
	0x20, 0x32, 0x04, 0x16, 0x68, 0x7a, 0x4c, 0x5e, 0xe6, 0xf4, 0xc2, 0xd0, 0xae, 0xbc, 0x8a, 0x98, 0x76, 0x64, 0x52,
	0x40, 0x3e, 0x2c, 0x1a, 0x08, 0xd4, 0xc6, 0xf0, 0xe2, 0x9c, 0x8e, 0xb8, 0xaa, 0x44, 0x56, 0x60, 0x72, 0x0c, 0x1e,
	0x28, 0x3a, 0x4a, 0x58, 0x6e, 0x7c, 0x02, 0x10, 0x26, 0x34, 0xda, 0xc8, 0xfe, 0xec, 0x92, 0x80, 0xb6, 0xa4, 0x78,
	0x6a, 0x5c, 0x4e, 0x30, 0x22, 0x14, 0x06, 0xe8, 0xfa, 0xcc, 0xde, 0xa0, 0xb2, 0x84, 0x96, 0x2e, 0x3c, 0x0a, 0x18,
	0x66, 0x74, 0x42, 0x50, 0xbe, 0xac, 0x9a, 0x88, 0xf6, 0xe4, 0xd2, 0xc0, 0x1c, 0x0e, 0x38, 0x2a, 0x54, 0x46, 0x70,
	0x62, 0x8c, 0x9e, 0xa8, 0xba, 0xc4, 0xd6, 0xe0, 0xf2};

uint crc7(const u8* q, uint count, uint crc, bool finalize)
{
	while (count--) { crc = crc7_table[crc ^ *q++]; }
	return crc | finalize;
}

#else

uint crc7(const uint8* q, uint count, uint crc, bool finalize)
{
	// the crc7 is in bits [7:1]
	// bit 0 is normally 0 but must be set as a stopbit in the final result

	while (count--)
	{
		crc ^= *q++;
		for (uint bits = 8; bits--;)
		{
			crc <<= 1;
			if (crc >> 8) crc ^= 0x89 << 1;
		}
	}
	return crc | finalize;
}
#endif


#if defined(FAST_CRC16) && FAST_CRC16

  #include "hardware/dma.h"

  #if 0
static u16 crc_itu_t_table[256] =
{
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
  #endif

uint crc16(const u8* q, uint count, uint crc)
{
	// release: time for 512 bytes, dma: 8µs (2clocks/byte), cpu: 166µs (40clocks/byte)
	// debug:   time for 512 bytes, dma: 19µs,               cpu: 400µs

	u8				   byte;
	uint			   dma_channel = uint(dma_claim_unused_channel(true));
	dma_channel_config config	   = dma_channel_get_default_config(dma_channel);
	channel_config_set_read_increment(&config, true);
	channel_config_set_ring(&config, false /*read*/, 0);
	channel_config_set_write_increment(&config, false);
	channel_config_set_ring(&config, true /*write*/, 0);
	channel_config_set_transfer_data_size(&config, DMA_SIZE_8);
	channel_config_set_sniff_enable(&config, true);

	dma_sniffer_enable(dma_channel, DMA_SNIFF_CTRL_CALC_VALUE_CRC16, true);
	dma_hw->sniff_data = crc;

	dma_channel_configure(dma_channel, &config, &byte, q, count, true /*start now*/);
	dma_channel_wait_for_finish_blocking(dma_channel);

	uint sniffed_crc = dma_hw->sniff_data;
	dma_sniffer_disable();
	dma_channel_unclaim(dma_channel);

	return sniffed_crc;
}

#else

uint crc16(const unsigned char* q, uint count, uint32 crc)
{
	while (count--)
	{
		crc ^= *q++ * 256;
		for (uint bits = 8; bits--;)
		{
			crc <<= 1;
			if (crc >> 16) crc ^= 0x11021;
		}
	}
	return crc;
}

#endif
