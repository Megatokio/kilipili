
cmake_minimum_required(VERSION 3.13)

###############################################
# USBHost/CMakeLists.txt


option(USB_ENABLE_HOST   "Enable Tiny USB host mode" ON)

set(USB_KEY_DELAY1		 "600"  CACHE STRING "USB keyboard: auto repeat delay for first repeat [msec]")
set(USB_KEY_DELAY		 "60"   CACHE STRING "USB keyboard: auto repeat delay for following repeats [msec]")
set(USB_DEFAULT_KEYTABLE "key_table_ger" CACHE STRING "USB keyboard: default key translation table")

set_property(CACHE USB_DEFAULT_KEYTABLE PROPERTY STRINGS key_table_us key_table_ger)


if(${UNIT_TEST})
	set(USB_ENABLE_HOST OFF)
	set(more_usb_sources 
		hid_handler_mock.cpp
	)
elseif(${PICO_STDIO_USB}) 
	# if USB is used for STDIO then we are obviously in Device Mode:	
	set(USB_ENABLE_HOST OFF)
	set(more_usb_sources  dummy.cpp) 
else()
	set(more_usb_sources 
		hid_handler.cpp 		
		msc_handler.cpp
		cdc_handler.cpp 
	)
	set(more_usb_libraries 
		tinyusb_host 
	)
endif()

add_library(kilipili_usb_host STATIC
	${more_usb_sources}
	hid_handler.h
	USBKeyboard.h
	USBKeyboard.cpp
	USBGamePad.h
	USBGamePad.cpp
	USBMouse.cpp
	USBMouse.h
	HidKeys.h
	HidKeys.cpp
	HidKeyTables.cpp
	HidKeyTables.h
	)
	
target_compile_definitions(kilipili_usb_host PUBLIC
	ON=1 OFF=0
	USB_ENABLE_HOST=${USB_ENABLE_HOST}
	USB_KEY_DELAY1=${USB_KEY_DELAY1}
	USB_KEY_DELAY=${USB_KEY_DELAY}
	USB_DEFAULT_KEYTABLE=${USB_DEFAULT_KEYTABLE}
	)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(kilipili_usb_host PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies. this also adds the include paths:
target_link_libraries(kilipili_usb_host PUBLIC
	kilipili_common
	${more_usb_libraries}
	)


	






