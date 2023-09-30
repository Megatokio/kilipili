
cmake_minimum_required(VERSION 3.13)


#########################################
# kilipili/CMakeLists.txt
#
# all kilipili subdirectories except USBHost


if(CMAKE_BUILD_TYPE STREQUAL "TEST")

	add_subdirectory(common)
	add_subdirectory(Graphics)
	
	add_executable(unit_tests
		unit_tests.cpp
		common/unit_tests.cpp
		Graphics/Canvas_unit_test.cpp
		)
	
	# add current dir to 'include search path':
	target_include_directories(unit_tests PUBLIC
		${CMAKE_CURRENT_LIST_DIR}
		)
	
	# dependencies. this also adds the include paths:
	target_link_libraries(unit_tests PUBLIC
		kilipili_common 
		kilipili_graphics 
		)
	
else() 
	
	add_library(kilipili STATIC
	)

	# dependencies:
	add_subdirectory(common)
	add_subdirectory(utilities)
	add_subdirectory(Graphics)
	add_subdirectory(Video)
	
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
		)
 
endif() 
 


