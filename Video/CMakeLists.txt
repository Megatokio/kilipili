
cmake_minimum_required(VERSION 3.13)


###############################################
# Video/CMakeLists.txt


####################################################################
# OPTIONS:

option(VIDEO_OPTIMISTICAL_A1W8_RGB "optimize a1w8_rgb mode for moderate color usage. makes 1280x768 work. poor display of many colors." OFF)
option(VIDEO_RECOVERY_PER_LINE "check for missed scanlines after every line. failing screens look better. makes 1024x768 fail." OFF)
option(VIDEO_SUPPORT_200x150_A1W8_RGB "support int8 bitmap width in a1w8_rgb renderer. slightly slower than otherwise." ON)
option(VIDEO_SUPPORT_400x300_A1W8_RGB "support int16 bitmap width in a1w8_rgb renderer. slightly slower than otherwise." ON)

set(VIDEO_MAX_SYSCLOCK_MHz "290" CACHE STRING "maximum system clock set by VideoBackend.start()")

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
	Shape.h
	Shape.cpp
	SingleSprite.h
	SingleSprite.cpp
)

target_compile_definitions(kilipili_video PUBLIC
	VIDEO_OPTIMISTICAL_A1W8_RGB=${VIDEO_OPTIMISTICAL_A1W8_RGB}
	VIDEO_RECOVERY_PER_LINE=${VIDEO_RECOVERY_PER_LINE}
	VIDEO_SUPPORT_200x150_A1W8_RGB=${VIDEO_SUPPORT_200x150_A1W8_RGB}
	VIDEO_SUPPORT_400x300_A1W8_RGB=${VIDEO_SUPPORT_400x300_A1W8_RGB}
	VIDEO_MAX_SYSCLOCK_MHz=${VIDEO_MAX_SYSCLOCK_MHz}
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
	hardware_exception
)


 



