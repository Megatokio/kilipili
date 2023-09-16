
cmake_minimum_required(VERSION 3.13)


####################################
# common/CMakeLists.txt 


add_library(kipili_common STATIC
	kipili_common.h
	kipili_cdefs.h
	standard_types.h
	no_copy_move.h
	basic_math.h
	)


option(USE_WIDECHARS "use wide chars (UCS2) in PicoTerm and for USB keyboard" ON)
option(USE_STDIO_UTF8 "use UTF-8 on serial line ((TODO))" ON)


target_compile_definitions(kipili_common PUBLIC
	ON=1 OFF=0
	#
	PICO_MALLOC_PANIC=0
	PICO_DEBUG_MALLOC=0
	PICO_STDIO_ENABLE_CRLF_SUPPORT=1
	#
	USE_WIDECHARS=${USE_WIDECHARS}
	USE_STDIO_UTF8=${USE_STDIO_UTF8}
	ENABLE_USB_HOST=!${PICO_STDIO_USB}
	)

# add current dir to 'include search path':
target_include_directories(kipili_common PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kipili_common PUBLIC
	)


 

