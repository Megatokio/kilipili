#pragma once
#include "standard_types.h"


extern uint crc16(const uint8* q, uint count, uint crc = 0x0000);
extern uint crc7(const uint8* q, uint count, uint crc = 0x00, bool finalize = true);
