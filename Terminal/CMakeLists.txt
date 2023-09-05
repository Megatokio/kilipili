
cmake_minimum_required(VERSION 3.13)

add_library(kipili_terminal STATIC
	font_12x8.h
	PicoTerm.h
	PicoTerm.cpp
	)

target_compile_definitions(kipili_terminal PRIVATE
	)

# add current dir to 'include search path':
target_include_directories(kipili_terminal PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kipili_terminal PRIVATE
	kipili_common
	kipili_graphics
	kipili_usb_host
	)




