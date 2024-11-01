
cmake_minimum_required(VERSION 3.13)


###############################################
# Devices/CMakeLists.txt


add_library(kilipili_devices STATIC
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
	internal/RamFile.cpp 
	internal/RamFile.h
	internal/RsrcFile.h 
	internal/RsrcFile.cpp
	internal/RsrcFS.h 
	internal/RsrcFS.cpp	
	internal/ff15/source/ff.h
	internal/ff15/source/ff.c
	internal/ff15/source/ffconf.h
	internal/ff15/source/diskio.h
	internal/ff15/source/ffunicode.c

	DevNull.cpp
	DevNull.h
	BlockDevice.h
	BlockDevice.cpp
	SerialDevice.h
	SerialDevice.cpp
	File.h
	File.cpp
	FileSystem.h
	SDCard.cpp
	SDCard.h
	Stdio.cpp
	Stdio.h
	PicoTerm.h
	PicoTerm.cpp
	BlockDeviceFile.cpp
	BlockDeviceFile.h
	devices_types.h
	Directory.h 
	Directory.cpp
	FileSystem.cpp	
)

target_compile_definitions(kilipili_devices PUBLIC
	)

# add current dir to 'include search path':
target_include_directories(kilipili_devices PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/internal
	)

# dependencies:
target_link_libraries(kilipili_devices PUBLIC
	kilipili_common
	kilipili_utilities
	pico_stdlib
	pico_stdio
	hardware_spi
	)







