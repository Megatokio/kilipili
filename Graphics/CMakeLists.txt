
cmake_minimum_required(VERSION 3.13)


###############################################
# Graphics/CMakeLists.txt


add_library(kilipili_graphics STATIC
	fixint.h
	geometry.h
	geometry.cpp
	Canvas.h
	Canvas.cpp
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
	rsrc/font_12x8.h
	PicoTerm.h
	PicoTerm.cpp
	)

target_compile_definitions(kilipili_graphics PUBLIC
	)

# add current dir to 'include search path':
target_include_directories(kilipili_graphics PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kilipili_graphics PUBLIC
	kilipili_common
	kilipili_utilities
	kilipili_usb_host
	pico_stdlib
	hardware_dma
	hardware_pwm
	)




