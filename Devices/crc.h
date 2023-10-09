#pragma once
#include <stdint.h>

using uint8	 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using cstr	 = const char*;
using uint	 = unsigned;

extern uint crc16(const uint8* q, uint count, uint crc = 0x0000);
extern uint crc7(const uint8* q, uint count, uint crc = 0x00, bool finalize = true);
