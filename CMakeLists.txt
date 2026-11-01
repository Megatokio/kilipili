###########################################
# kilipili/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)
set(CMAKE_CXX_STANDARD 20)


###########################################
# library with all kilipili subdirectories:
	

# dependencies:
add_subdirectory(common)
add_subdirectory(graphics)
add_subdirectory(video)
add_subdirectory(audio)
add_subdirectory(audio/libmad)
add_subdirectory(devices)
add_subdirectory(usb_host)

add_library(kilipili STATIC
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
	)





