###########################################
# kilipili/unit_test/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD_HEADER_DIRS  "${CMAKE_CURRENT_LIST_DIR}/boards")
set(PICO_BOARD_HEADER_DIRS ${PICO_BOARD_HEADER_DIRS} CACHE STRING "PICO board header directories" FORCE)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}   -DDEBUG -DUNIT_TEST")
	 
add_subdirectory(common)
add_subdirectory(Graphics)
add_subdirectory(USBHost)

add_executable(UnitTest
	main_unit_test.cpp
	tests/Pixmap_unit_test.cpp
	tests/Canvas_unit_test.cpp
	tests/BitBlit_unit_test.cpp
	tests/TextVDU_unit_test.cpp
	tests/AnsiTerm_unit_test.cpp
	tests/AudioSource_unit_test.cpp
	tests/AudioSample_unit_test.cpp
	tests/tempmem.test.cpp
	tests/cstrings.test.cpp
	tests/relational_operators.test.cpp
	tests/Array.test.cpp
	tests/Ay38912_unit_test.cpp
	../Audio/AudioSource.h
	../Audio/AudioSource.cpp 		
	../Audio/audio_options.h
	../Audio/AudioSample.h
	../Audio/Ay38912.cpp
	../Audio/Ay38912.h
	) 

target_compile_definitions(UnitTest PUBLIC
	)

# add current dir to 'include search path':
target_include_directories(UnitTest PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/tests
	)

# dependencies. this also adds the include paths:
target_link_libraries(UnitTest PUBLIC
	kilipili_common 
	kilipili_graphics 
	kilipili_usb_host
	z
	) 



