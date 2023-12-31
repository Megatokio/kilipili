// Copyright (c) 2022 - 2024 kio@little-bat.de
// BSD-2-Clause license
// https://opensource.org/licenses/BSD-2-Clause

/*
* The MIT License (MIT)
*
* Copyright (c) 2021, Ha Thach (tinyusb.org)
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

#if USB_ENABLE_HOST

/*
	TUH = TinyUSB Host
	HID = Human Interface Device
*/

  #include "hid_handler.h"
  #include "standard_types.h"
  #include <class/hid/hid_host.h>
  #include <tusb.h>

// Each HID instance can have multiple reports
static constexpr uint MAX_REPORT = 4;

static struct
{
	uint8				  report_count;
	uint8				  _;
	tuh_hid_report_info_t report_info[MAX_REPORT];
} hid_info[CFG_TUH_HID];


namespace kio::USB
{
static HidMouseEventHandler*	mouse_event_handler	   = defaultHidMouseEventHandler;
static HidKeyboardEventHandler* keyboard_event_handler = defaultHidKeyboardEventHandler;

void setHidMouseEventHandler(HidMouseEventHandler* handler) noexcept
{
	mouse_event_handler = handler ? handler : defaultHidMouseEventHandler;
}

void setHidKeyboardEventHandler(HidKeyboardEventHandler* handler) noexcept
{
	keyboard_event_handler = handler ? handler : defaultHidKeyboardEventHandler;
}

static void process_generic_report(uint8 dev_addr, uint8 instance, const uint8* report, uint16 len)
{
	(void)dev_addr;

	const uint8			   rpt_count	= hid_info[instance].report_count;
	tuh_hid_report_info_t* rpt_info_arr = hid_info[instance].report_info;
	tuh_hid_report_info_t* rpt_info		= nullptr;

	if (rpt_count == 1 && rpt_info_arr[0].report_id == 0)
	{
		// Simple report without report ID as 1st byte
		rpt_info = &rpt_info_arr[0];
	}
	else
	{
		// Composite report, 1st byte is report ID, data starts from 2nd byte
		const uint8 rpt_id = report[0];

		// Find report id in the arrray
		for (uint8 i = 0; i < rpt_count; i++)
		{
			if (rpt_id == rpt_info_arr[i].report_id)
			{
				rpt_info = &rpt_info_arr[i];
				break;
			}
		}

		report++;
		len--;
	}

	if (!rpt_info)
	{
		printf("Couldn't find the report info for this report!\n");
		return;
	}

	// For complete list of Usage Page & Usage checkout src/class/hid/hid.h. For examples:
	// - Keyboard                     : Desktop, Keyboard
	// - Mouse                        : Desktop, Mouse
	// - Gamepad                      : Desktop, Gamepad
	// - Consumer Control (Media Key) : Consumer, Consumer Control
	// - System Control (Power key)   : Desktop, System Control
	// - Generic (vendor)             : 0xFFxx, xx

	if (rpt_info->usage_page == HID_USAGE_PAGE_DESKTOP)
	{
		switch (rpt_info->usage)
		{
  #if USB_ENABLE_KEYBOARD
		case HID_USAGE_DESKTOP_KEYBOARD:
			//printf("HID receive keyboard report\n");
			// Assume keyboard follow boot report layout
			keyboard_event_handler(reinterpret_cast<const HidKeyboardReport&>(*report));
			break;
  #endif

  #if USB_ENABLE_MOUSE
		case HID_USAGE_DESKTOP_MOUSE:
			//printf("HID receive mouse report\n");
			// Assume mouse follow boot report layout
			mouse_event_handler(reinterpret_cast<const HidMouseReport&>(*report));
			break;
  #endif

		default: break;
		}
	}
}

} // namespace kio::USB


//--------------------------------------------------------------------
// callbacks
//--------------------------------------------------------------------

extern "C" void tuh_hid_mount_cb(uint8 dev_addr, uint8 instance, const uint8* desc_report, uint16 desc_len)
{
	// Device with hid interface is mounted
	// Report descriptor is also available for use.
	// tuh_hid_parse_report_descriptor() can be used to parse common/simple descriptor.
	// Note: if report descriptor length > CFG_TUH_ENUMERATION_BUFSIZE, it will be skipped
	//		 therefore report_desc = NULL, desc_len = 0

	printf("HID device address = %i, instance = %i mounted\n", dev_addr, instance);

	// Interface protocol (hid_interface_protocol_enum_t)
	constexpr char protocol_str[3][9] = {"None", "Keyboard", "Mouse"};
	const uint8	   itf_protocol		  = tuh_hid_interface_protocol(dev_addr, instance);

	printf("HID Interface Protocol = %s\n", protocol_str[itf_protocol]);

	// By default host stack will activate boot protocol on supported interface.
	// Therefore for this simple example, we only need to parse generic report descriptor (with built-in parser)
	if (itf_protocol == HID_ITF_PROTOCOL_NONE)
	{
		hid_info[instance].report_count =
			tuh_hid_parse_report_descriptor(hid_info[instance].report_info, MAX_REPORT, desc_report, desc_len);
		printf("HID has %u reports\n", hid_info[instance].report_count);
	}

	// request to receive report
	// tuh_hid_report_received_cb() will be invoked when report is available
	if (!tuh_hid_receive_report(dev_addr, instance)) { printf("Error: cannot request to receive report\n"); }
}

extern "C" void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance)
{
	// Device with hid interface is unmounted

	printf("HID device address = %d, instance = %d is unmounted\n", dev_addr, instance);
}

extern "C" void tuh_hid_report_received_cb(uint8 dev_addr, uint8 instance, const uint8* report, uint16 len)
{
	// Received report from device via interrupt endpoint

	using namespace kio::USB;

	switch (tuh_hid_interface_protocol(dev_addr, instance))
	{
  #if USB_ENABLE_KEYBOARD
	case HID_ITF_PROTOCOL_KEYBOARD:
		//printf("HID receive boot keyboard report\n");
		keyboard_event_handler(reinterpret_cast<const HidKeyboardReport&>(*report));
		break;
  #endif

  #if USB_ENABLE_MOUSE
	case HID_ITF_PROTOCOL_MOUSE:
		//printf("HID receive boot mouse report\n");
		mouse_event_handler(reinterpret_cast<const HidMouseReport&>(*report));
		break;
  #endif

	default:
		// Generic report requires matching ReportID and contents with previous parsed report info
		process_generic_report(dev_addr, instance, report, len);
		break;
	}

	// continue to request to receive report
	if (!tuh_hid_receive_report(dev_addr, instance)) { printf("Error: cannot request to receive report\n"); }
}


#endif // CFG_TUH_HID
