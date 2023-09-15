
cmake_minimum_required(VERSION 3.13)

set(FAMILY rp2040)
set(BOARD pico_sdk)
set(TINYUSB_FAMILY_PROJECT_NAME_PREFIX "tinyusb_host_")

add_library(kipili_usb_host STATIC
	hid_handler.cpp
	hid_handler.h
	msc_handler.cpp
	cdc_handler.cpp
	USBKeyboard.h
	USBKeyboard.cpp
	USBGamePad.h
	USBGamePad.cpp
	HidKeys.h
	HidKeys.cpp
	HidKeyTables.cpp
	HidKeyTables.h
	)

target_compile_definitions(kipili_usb_host PRIVATE
	CFG_TUH_HID=4	# number of mouse+kbd   [4]
	CFG_TUH_MSC=1	# number of ubs sticks  [1]
	CFG_TUH_CDC=1	# number of communication devices [1]
	CFG_TUH_HUB=1	# number of hubs		[1]
	)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(kipili_usb_host PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
	)

# additional dependencies on tinyusb_host for TinyUSB device support
target_link_libraries(kipili_usb_host PUBLIC
	tinyusb_host
	kipili_common
	kipili_terminal
	)







