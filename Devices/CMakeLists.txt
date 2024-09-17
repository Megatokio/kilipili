
cmake_minimum_required(VERSION 3.13)


###############################################
# Devices/CMakeLists.txt


add_library(kilipili_devices STATIC
	DevNull.cpp
	DevNull.h
	FatFile.cpp
	FatFile.h
	FatFS.cpp
	FatFS.h
	BlockDevice.h
	BlockDevice.cpp
	SerialDevice.h
	SerialDevice.cpp
	File.h
	File.cpp
	FileSystem.h
	SDCard.cpp
	SDCard.h
	CSD.cpp
	CSD.h
	crc.h 
	crc.cpp
	Stdio.cpp
	Stdio.h
	PicoTerm.h
	PicoTerm.cpp
	BlockDeviceFile.cpp
	BlockDeviceFile.h
	ff15/source/ff.h
	ff15/source/ff.c
	ff15/source/ffconf.h
	ff15/source/diskio.h
	ff15/source/ffunicode.c
	devices_types.h
	Directory.h Directory.cpp
	FatDir.h FatDir.cpp
	FileSystem.cpp
	RamFile.cpp RamFile.h
	)

target_compile_definitions(kilipili_devices PUBLIC
	)

# add current dir to 'include search path':
target_include_directories(kilipili_devices PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kilipili_devices PUBLIC
	kilipili_common
	kilipili_utilities
	pico_stdlib
	pico_stdio
	hardware_spi
	)







