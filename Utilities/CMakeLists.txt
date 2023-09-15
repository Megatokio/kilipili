
cmake_minimum_required(VERSION 3.13)

####################################
# common/CMakeLists.txt 

add_library(kipili_utilities STATIC
	PwmLoadSensor.h
	PwmLoadSensor.cpp
	utilities.h
	utilities.cpp
	Led.h
	Led.cpp
	malloc.cpp
	BucketList.h
	)

#option(USE_WIDECHARS "use wide chars (UCS2) in PicoTerm and for USB keyboard" ON)
#option(USE_STDIO_UTF8 "use UTF-8 on serial line" ON)

#target_compile_definitions(kipili_utilities PUBLIC
#	ON=1 OFF=0
#	)

# add current dir to 'include search path':
target_include_directories(kipili_utilities PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kipili_utilities PUBLIC
	kipili_common
	pico_stdlib
	pico_multicore
	hardware_pwm
	)




