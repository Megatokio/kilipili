// Copyright (c) 2022 - 2023 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

#if CFG_TUH_CDC

/*
	TUH = TinyUSB Host
	CDC = Communications Device Class
 */

  #include "common/Led.h"
  #include "common/PwmLoadSensor.h"
  #include "common/utilities.h"
  #include "main.h"
  #include <tusb.h>


static char serial_in_buffer[64] = {0};


// -------------------------------------------------------------
//	callbacks and ISR
// -------------------------------------------------------------

//	extern "C" void tuh_mount_cb(uint8 dev_addr)
//	{
//		printf("Device with address %d mounted\r\n", dev_addr);
//
//		// schedule first transfer:
//		tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true);
//	}
//
//	extern "C" void tuh_umount_cb(uint8 dev_addr)
//	{
//		printf("Device with address %d unmounted\r\n", dev_addr);
//	}
//
//	extern "C" void tuh_cdc_xfer_isr(uint8 dev_addr, xfer_result_t event, cdc_pipeid_t pipe_id, uint32 xferred_bytes)
//	{
//		// ISR
//
//		(void) event;
//		(void) pipe_id;
//		(void) xferred_bytes;
//
//		printf("%s",serial_in_buffer);
//		memset(serial_in_buffer, 0, sizeof(serial_in_buffer));
//
//		// schedule next transfer:
//		tuh_cdc_receive(dev_addr, serial_in_buffer, sizeof(serial_in_buffer), true);
//	}

#endif // CFG_TUH_CDC
