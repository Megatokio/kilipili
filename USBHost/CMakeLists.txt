
cmake_minimum_required(VERSION 3.13)

###############################################
# USBHost/CMakeLists.txt


option(USB_ENABLE_HOST   "Enable Tiny USB host mode" ON)

set(USB_KEY_DELAY1		 "600"  CACHE STRING "USB keyboard: auto repeat delay for first repeat [msec]")
set(USB_KEY_DELAY		 "60"   CACHE STRING "USB keyboard: auto repeat delay for following repeats [msec]")
set(USB_DEFAULT_KEYTABLE "key_table_ger" CACHE STRING "USB keyboard: default key translation table")

set_property(CACHE USB_DEFAULT_KEYTABLE PROPERTY STRINGS key_table_us key_table_ger)


set(usb_sources
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

set(usb_defines
	ON=1 OFF=0
	USB_ENABLE_HOST=${USB_ENABLE_HOST}
	USB_KEY_DELAY1=${USB_KEY_DELAY1}
	USB_KEY_DELAY=${USB_KEY_DELAY}
	USB_DEFAULT_KEYTABLE=${USB_DEFAULT_KEYTABLE}
)

set(usb_includes
	${CMAKE_CURRENT_LIST_DIR}
)

set(usb_libraries
	kilipili_common
)


if(${MAKE_TOOLS})
	set(USB_ENABLE_HOST OFF)
elseif(${PICO_STDIO_USB})    
	# if USB is used for STDIO then we are obviously in Device Mode:
	set(USB_ENABLE_HOST OFF)
else()
	set(usb_sources
		${usb_sources}
		hid_handler.cpp 		
		msc_handler.cpp
		cdc_handler.cpp 
	)
	set(usb_libraries
		${usb_libraries}
		tinyusb_host 
	)
endif()


add_library               (kilipili_usb_host STATIC  ${usb_sources}  )
target_compile_definitions(kilipili_usb_host PUBLIC  ${usb_defines}  )
target_include_directories(kilipili_usb_host PUBLIC  ${usb_includes} )
target_link_libraries     (kilipili_usb_host PUBLIC  ${usb_libraries})











	

















