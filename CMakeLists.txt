###########################################
# kilipili/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)

###########################################
# library with all kilipili subdirectories:
	
set(PICO_BOARD_HEADER_DIRS  "${CMAKE_CURRENT_LIST_DIR}/boards")
set(PICO_BOARD_HEADER_DIRS ${PICO_BOARD_HEADER_DIRS} CACHE STRING "PICO board header directories" FORCE)

# dependencies:
add_subdirectory(common)
add_subdirectory(utilities)
add_subdirectory(Graphics)
add_subdirectory(Video)
add_subdirectory(Audio)
add_subdirectory(Devices)
add_subdirectory(USBHost)

add_library(kilipili STATIC
)

# add current dir to 'include search path':
target_include_directories(kilipili PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies. this also adds the include paths:
target_link_libraries(kilipili PUBLIC
	kilipili_common
	kilipili_utilities
	kilipili_graphics
	kilipili_video
	kilipili_audio
	kilipili_devices
	kilipili_usb_host
	)





