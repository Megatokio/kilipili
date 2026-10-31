###########################################
# kilipili/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

###########################################
# library with all kilipili subdirectories:
	
set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/boards")
set(PICO_BOARD_HEADER_DIRS ${PICO_BOARD_HEADER_DIRS} CACHE STRING "PICO board header directories" FORCE)
set(PICO_BOARD "vgaboard-i2s" CACHE STRING "the target board, e.g. \"vgaboard-i2s\"")


# dependencies:
add_subdirectory(common)
add_subdirectory(graphics)
add_subdirectory(video)
add_subdirectory(audio)
add_subdirectory(audio/libmad)
add_subdirectory(devices)
add_subdirectory(usb_host)
add_subdirectory(pico_hw)

add_library(kilipili STATIC
	common/common.h
	graphics/graphics.h
	video/video.h
	audio/audio.h
	devices/devices.h
	usb_host/usb_host.h
	pico_hw/pico_hw.h
	kilipili.h
)

# add current dir to 'include search path':
target_include_directories(kilipili PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies. this also adds the include paths:
target_link_libraries(kilipili PUBLIC
	kilipili_common
	kilipili_graphics
	kilipili_video
	kilipili_audio
	kilipili_libmad
	kilipili_devices
	kilipili_usb_host
	kilipili_pico_hw
	)





