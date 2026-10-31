/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "standard_types.h"
#include <tusb.h>

#if CFG_TUH_MSC

/*
	TUH = TinyUSB, Host
	MSC = Mass Storage Class
 */

static scsi_inquiry_resp_t inquiry_resp;

//--------------------------------------------------------------------
//	callbacks
//--------------------------------------------------------------------

extern "C" bool inquiry_complete_cb(uint8 dev_addr, const msc_cbw_t* cbw, const msc_csw_t* csw)
{
	// cbw = Command Block Wrapper
	// csw = Command Status Wrapper

	if (csw->status != 0)
	{
		printf("Inquiry failed\r\n");
		return false;
	}

	// Print out Vendor ID, Product ID and Rev
	printf("%.8s %.16s rev %.4s\r\n", inquiry_resp.vendor_id, inquiry_resp.product_id, inquiry_resp.product_rev);

	// Get capacity of device
	const uint32 block_count = tuh_msc_get_block_count(dev_addr, cbw->lun);
	const uint32 block_size	 = tuh_msc_get_block_size(dev_addr, cbw->lun);

	printf("Disk Size: %u MB\r\n", block_count / ((1024 * 1024) / block_size));
	printf("Block Count = %u, Block Size: %u\r\n", block_count, block_size);

	return true;
}

extern "C" void tuh_msc_mount_cb(uint8 dev_addr)
{
	(void)dev_addr;
	printf("MassStorage device mounted\r\n");

	//	const uint8 lun = 0;
	//	tuh_msc_inquiry(dev_addr, lun, &inquiry_resp, inquiry_complete_cb);

	//  //------------- file system (only 1 LUN support) -------------//
	//  uint8_t phy_disk = dev_addr-1;
	//  disk_initialize(phy_disk);
	//
	//  if ( disk_is_ready(phy_disk) )
	//  {
	//    if ( f_mount(phy_disk, &fatfs[phy_disk]) != FR_OK )
	//    {
	//      puts("mount failed");
	//      return;
	//    }
	//
	//    f_chdrive(phy_disk); // change to newly mounted drive
	//    f_chdir("/"); // root as current dir
	//
	//    cli_init();
	//  }
}

extern "C" void tuh_msc_umount_cb(uint8 dev_addr)
{
	(void)dev_addr;
	printf("MassStorage device unmounted\r\n");

	//  uint8_t phy_disk = dev_addr-1;
	//
	//  f_mount(phy_disk, NULL); // unmount disk
	//  disk_deinitialize(phy_disk);
	//
	//  if ( phy_disk == f_get_current_drive() )
	//  { // active drive is unplugged --> change to other drive
	//    for(uint8_t i=0; i<CFG_TUH_DEVICE_MAX; i++)
	//    {
	//      if ( disk_is_ready(i) )
	//      {
	//        f_chdrive(i);
	//        cli_init(); // refractor, rename
	//      }
	//    }
	//  }
}

#endif // CFG_TUH_MSC
