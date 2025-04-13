
cmake_minimum_required(VERSION 3.13)


###############################################
# Devices/CMakeLists.txt


option(DEVICES_LARGE_FILE_SUPPORT "support files larger than 4GB" OFF)

set(FLASH_PREFERENCES "4*1024" CACHE STRING "size of preferences in program flash")
set_property(CACHE FLASH_PREFERENCES PROPERTY STRINGS "0" "4*1024" "8*1024" "12*1024" "16*1024")

set(FLASH_BLOCKDEVICE "300*1024" CACHE STRING "size of flash disk in program flash")
set_property(CACHE FLASH_BLOCKDEVICE PROPERTY STRINGS "0" "300*1024" "500*1024" "720*1024" "1000*1024" "1400*1024")

set(devices_sources)
set(devices_includes)
set(devices_defines)
set(devices_libraries)


if(${MAKE_TOOLS})
	set(devices_sources 
		StdFile.cpp
		StdFile.h)
else()
	set(devices_sources 
		internal/CSD.h 
		internal/CSD.cpp
		internal/crc.h 
		internal/crc.cpp
		internal/FatFile.h 
		internal/FatFile.cpp
		internal/FatFS.h 
		internal/FatFS.cpp
		internal/FatDir.h 
		internal/FatDir.cpp
		internal/RsrcFile.h 
		internal/RsrcFile.cpp
		internal/RsrcFS.h 
		internal/RsrcFS.cpp	
		internal/ff15/source/ff.h
		internal/ff15/source/ff.c
		internal/ff15/source/ffconf.h
		internal/ff15/source/diskio.h
		internal/ff15/source/ffunicode.c
		FileSystem.h 
		FileSystem.cpp	
		Stdio.cpp    
		Stdio.h
		SDCard.cpp   
		SDCard.h
	)
	set(devices_defines
		PICO_FLASH_ASSUME_CORE1_SAFE=1
		PICO_FLASH_SAFE_EXECUTE_PICO_SUPPORT_MULTICORE_LOCKOUT=0	
	)
	set(devices_libraries 
		pico_stdlib
		pico_stdio
		hardware_spi
		hardware_flash
	)
endif()


add_library(kilipili_devices STATIC  
	${devices_sources}  
	devices_types.h
	RamFile.cpp 	
	RamFile.h
	DevNull.cpp		
	DevNull.h
	BlockDevice.h	
	BlockDevice.cpp
	SerialDevice.h	
	SerialDevice.cpp
	File.h       	
	File.cpp
	PicoTerm.h  	
	PicoTerm.cpp
	BlockDeviceFile.cpp 
	BlockDeviceFile.h
	Directory.h	 	
	Directory.cpp
	HeatShrinkDecoder.h 
	HeatShrinkDecoder.cpp
	HeatShrinkEncoder.h 
	HeatShrinkEncoder.cpp
	LzhDecoder.cpp	
	LzhDecoder.h
	QspiFlashDevice.h 
	QspiFlashDevice.cpp
	Flash.h 		
	Flash.cpp	
	Preferences.h	
	Preferences.cpp
	devices.h
)

target_compile_definitions(kilipili_devices PUBLIC  
	${devices_defines}  
	ON=1 OFF=0
	DEVICES_LARGE_FILE_SUPPORT=${DEVICES_LARGE_FILE_SUPPORT}
	FLASH_PREFERENCES=${FLASH_PREFERENCES}
	FLASH_BLOCKDEVICE=${FLASH_BLOCKDEVICE}
)

target_include_directories(kilipili_devices PUBLIC  
	${devices_includes}  
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/internal
)

target_link_libraries(kilipili_devices PUBLIC  
	${devices_libraries}
	kilipili_common
)







