
cmake_minimum_required(VERSION 3.13)

add_library(kipili_common STATIC
	cdefs.h
	standard_types.h
	errors.cpp
	errors.h
	PwmLoadSensor.h
	PwmLoadSensor.cpp
	utilities.h
	utilities.cpp
	Led.h
	Led.cpp
	malloc.cpp
	BucketList.h
	)

target_compile_definitions(kipili_common PUBLIC
	PICO_MALLOC_PANIC=0
	PICO_DEBUG_MALLOC=0
	PICO_STDIO_ENABLE_CRLF_SUPPORT=1
	)

if(NOT ${PICO_STDIO_USB})
	target_compile_definitions(kipili_common INTERFACE ENABLE_USB_HOST=1)
endif()
if(${PICO_STDIO_UART})
	target_compile_definitions(kipili_common INTERFACE ENABLE_STDIO_UART=1)
endif()
if(${PICO_STDIO_USB})
	target_compile_definitions(kipili_common INTERFACE ENABLE_STDIO_USB=1)
endif()
if(${USE_WIDECHARS})
	target_compile_definitions(kipili_common INTERFACE USE_WIDECHARS=1)
endif()
if(${USE_STDIO_UTF8})
	target_compile_definitions(kipili_common INTERFACE USE_STDIO_UTF8=1)
endif()


# add current dir to 'include search path':
target_include_directories(kipili_common PUBLIC
	${PROJECT_SOURCE_DIR}
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kipili_common PUBLIC
	pico_stdlib
	pico_multicore
	hardware_pwm
	)




