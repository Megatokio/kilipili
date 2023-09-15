
cmake_minimum_required(VERSION 3.13)


add_library(kipili_graphics STATIC
	DrawEngine.cpp
	DrawEngine.h
	fixint.h
	geometry.h
	geometry.cpp
	Pixmap.h
	Pixmap.cpp
	Pixmap_wAttr.h
	Pixmap_wAttr.cpp
	ColorMap.cpp
	ColorMap.h
	graphics_types.h
	Color.h
	BitBlit.h
	BitBlit.cpp
	)

target_compile_definitions(kipili_graphics PRIVATE
	)

# add current dir to 'include search path':
target_include_directories(kipili_graphics PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
	)

# dependencies:
target_link_libraries(kipili_graphics PRIVATE
	kipili_common
	kipili_utilities
	kipili_usb_host
	pico_stdlib
	hardware_dma
	hardware_pwm
	)




