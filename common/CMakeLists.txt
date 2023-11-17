
cmake_minimum_required(VERSION 3.13)


####################################
# common/CMakeLists.txt 


add_library(kilipili_common STATIC
	dummy.cpp
	kilipili_common.h
	kilipili_cdefs.h
	standard_types.h
	no_copy_move.h
	basic_math.h
	tempmem.h
	cstrings.h
	cstrings.cpp
	Queue.h
	geometry.h
	geometry.cpp
	)
 
target_compile_definitions(kilipili_common PUBLIC
	ON=1 OFF=0
	#
	PICO_MALLOC_PANIC=0
	PICO_DEBUG_MALLOC=0
	PICO_STDIO_ENABLE_CRLF_SUPPORT=1
	)

# add current dir to 'include search path':
target_include_directories(kilipili_common PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
	)

# dependencies:
target_link_libraries(kilipili_common PUBLIC
	)

 
 

