
cmake_minimum_required(VERSION 3.13)

add_library(kipili STATIC
	)

add_subdirectory(common)
add_subdirectory(Utilities)
add_subdirectory(Graphics)
add_subdirectory(Video)
add_subdirectory(Terminal)

# add current dir to 'include search path':
target_include_directories(kipili PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# Add all libraries. this also adds the include paths:
target_link_libraries(kipili PUBLIC
	kipili_common
	kipili_utilities
	kipili_graphics
	kipili_video
	kipili_terminal
	)




