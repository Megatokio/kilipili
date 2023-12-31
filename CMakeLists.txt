
cmake_minimum_required(VERSION 3.13)


###########################################
# kilipili/CMakeLists.txt

set(PICO_BOARD_HEADER_DIRS  "${CMAKE_CURRENT_LIST_DIR}/boards")


###########################################
# executable for unit tests:

if(${UNIT_TEST})

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -DDEBUG -DUNIT_TEST")
		 
	add_subdirectory(common)
	add_subdirectory(Graphics)
	
	add_executable(unit_tests
		main_unit_tests.cpp
		Graphics/Pixmap_unit_test.cpp
		Graphics/Canvas_unit_test.cpp
		Graphics/BitBlit_unit_test.cpp
		common/tempmem.test.cpp
		common/cstrings.test.cpp
		common/relational_operators.test.cpp
		common/Array.test.cpp 		
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
	
endif()


###########################################
# library with all kilipili subdirectories:

if(CMAKE_SYSTEM_NAME STREQUAL "PICO")
	
	# dependencies:
	add_subdirectory(common)
	add_subdirectory(utilities)
	add_subdirectory(Graphics)
	add_subdirectory(Video)
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
		kilipili_devices
		kilipili_usb_host
		)
 
endif() 
 


