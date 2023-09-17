
cmake_minimum_required(VERSION 3.13)


#########################################
# kipili/CMakeLists.txt
#
# all kipili subdirectories except USBHost


add_library(kipili STATIC
	)

# dependencies:
add_subdirectory(common)
add_subdirectory(utilities)
add_subdirectory(Graphics)
add_subdirectory(Video)

# add current dir to 'include search path':
target_include_directories(kipili PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies. this also adds the include paths:
target_link_libraries(kipili PUBLIC
	kipili_common
	kipili_utilities
	kipili_graphics
	kipili_video
	)




