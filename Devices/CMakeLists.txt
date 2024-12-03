
cmake_minimum_required(VERSION 3.13)


###############################################
# Devices/CMakeLists.txt


option(DEVICES_LARGE_FILE_SUPPORT "support files larger than 4GB" OFF)


if(${MAKE_TOOLS})
	set(distinct_sources 
		StdFile.cpp
		StdFile.h)
	set(distinct_headers )
	set(distinct_libraries )
else()
	set(distinct_sources 
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
		SDCard.h)
	set(distinct_headers 
		${CMAKE_CURRENT_LIST_DIR}/internal)
	set(distinct_libraries 
		pico_stdlib
		pico_stdio
		hardware_spi
		kilipili_utilities)
endif()


add_library(kilipili_devices STATIC
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
	devices_types.h
	Directory.h 
	Directory.cpp
	HeatShrinkDecoder.h 
	HeatShrinkDecoder.cpp
	HeatShrinkEncoder.h 
	HeatShrinkEncoder.cpp
	LzhDecoder.cpp
	LzhDecoder.h
	${distinct_sources}
)

target_compile_definitions(kilipili_devices PUBLIC
	ON=1 OFF=0
	DEVICES_LARGE_FILE_SUPPORT=${DEVICES_LARGE_FILE_SUPPORT}
	)

# add current dir to 'include search path':
target_include_directories(kilipili_devices PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${distinct_headers}
	)

# dependencies:
target_link_libraries(kilipili_devices PUBLIC
	kilipili_common
	${distinct_libraries}
	)







