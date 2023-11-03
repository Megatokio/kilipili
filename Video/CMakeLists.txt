
cmake_minimum_required(VERSION 3.13)


###############################################
# Video/CMakeLists.txt


####################################################################
# OPTIONS:

option(VIDEO_OPTIMISTICAL_A1W8_RGB "optimize a1w8_rgb mode for moderate color usage. makes 1280x768 work. poor display of many colors." OFF)
option(VIDEO_RECOVERY_PER_LINE "check for missed scanlines after every line. failing screens look better. makes 1024x768 fail." OFF)

add_library(kilipili_video STATIC
	FrameBuffer.h
	Mouse.cpp
	Mouse.h
	ScanlineRenderFu.cpp
	ScanlineRenderFu.h
	VideoPlane.h
	VideoController.cpp
	VideoController.h
	Sprites.cpp
	Sprites.h
	VgaMode.h
	scanline.pio
	video_options.h
	timing.pio
	vga_types.h
	ScanlineBuffer.h 
	ScanlineBuffer.cpp
	VideoBackend.cpp
	VideoBackend.h
	ColorMap.cpp
	ColorMap.h
	Color.h
)

target_compile_definitions(kilipili_video PUBLIC
	VIDEO_OPTIMISTICAL_A1W8_RGB=${VIDEO_OPTIMISTICAL_A1W8_RGB}
	VIDEO_RECOVERY_PER_LINE=${VIDEO_RECOVERY_PER_LINE}
)

# add current dir to 'include search path':
target_include_directories(kilipili_video PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
)

# if there is a PIO assembler file:
pico_generate_pio_header(kilipili_video ${CMAKE_CURRENT_LIST_DIR}/timing.pio)
pico_generate_pio_header(kilipili_video ${CMAKE_CURRENT_LIST_DIR}/scanline.pio)

# dependencies:
target_link_libraries(kilipili_video PUBLIC 
	kilipili_common
	kilipili_utilities
	kilipili_graphics
	kilipili_usb_host  # wg. mouse
	pico_stdlib
	hardware_pio
	hardware_dma
	pico_multicore
	hardware_interp
)


 



