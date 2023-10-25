
cmake_minimum_required(VERSION 3.13)

###############################################
# USBHost/CMakeLists.txt


# if USB is used for STDIO then we are obviously in Device Mode:
if(${PICO_STDIO_USB})
	
	add_library(kilipili_usb_host STATIC
		dummy.cpp
		)
	
	target_compile_definitions(kilipili_usb_host PUBLIC
		ON=1 OFF=0
		ENABLE_USB_HOST=OFF
		ENABLE_USB_MOUSE=OFF
		ENABLE_USB_KEYBOARD=OFF
		)
	
	target_link_libraries(kilipili_usb_host PUBLIC
		)

else() 
		
	add_library(kilipili_usb_host STATIC
		hid_handler.cpp
		hid_handler.h
		msc_handler.cpp
		cdc_handler.cpp
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
	
	
	option(ENABLE_USB_HOST     "Enable USB host mode" ON)
	option(ENABLE_USB_KEYBOARD "Enable USB keyboard in USB host mode" ON)
	option(ENABLE_USB_MOUSE    "Enable USB mouse in USB host mode" ON)

	set(USB_KEY_DELAY1 "600" CACHE STRING "USB keyboard: auto repeat delay for first repeat [msec]")
	set(USB_KEY_DELAY  "60"  CACHE STRING "USB keyboard: auto repeat delay for following repeats [msec]")
	
	
	target_compile_definitions(kilipili_usb_host PUBLIC
		ON=1 OFF=0
		ENABLE_USB_HOST=${ENABLE_USB_HOST}
		ENABLE_USB_MOUSE=${ENABLE_USB_MOUSE}
		ENABLE_USB_KEYBOARD=${ENABLE_USB_KEYBOARD}
		USB_KEY_DELAY1=${USB_KEY_DELAY1}
		USB_KEY_DELAY=${USB_KEY_DELAY}
		)

	# Make sure TinyUSB can find tusb_config.h
	target_include_directories(kilipili_usb_host PUBLIC
		${CMAKE_CURRENT_LIST_DIR}
		)
	
	# additional dependencies on tinyusb_host for TinyUSB device support
	target_link_libraries(kilipili_usb_host PUBLIC
		kilipili_common
		tinyusb_host
		pico_stdlib
		)
	
endif()








