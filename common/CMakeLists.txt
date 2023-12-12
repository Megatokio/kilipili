
cmake_minimum_required(VERSION 3.13)


####################################
# common/CMakeLists.txt 


if(${UNIT_TEST})
	set(distinct_sources 
		linux/glue.cpp
		linux/glue.h
		tempmem-posix.cpp)
	set(distinct_headers 
		${CMAKE_CURRENT_LIST_DIR}/linux)
	set(distinct_libraries )
else()
	set(distinct_sources 
		pico/glue.cpp
		pico/glue.h
		tempmem-pico.cpp)
	set(distinct_headers 
		${CMAKE_CURRENT_LIST_DIR}/pico)
	set(distinct_libraries 
		pico_stdlib)
endif()

add_library(kilipili_common STATIC
	dummy.cpp
	cdefs.h
	standard_types.h
	no_copy_move.h
	basic_math.h
	tempmem.h
	cstrings.h
	cstrings.cpp
	Queue.h
	geometry.h
	geometry.cpp
	RCPtr.h
	algorithms.h
	Xoshiro256.cpp
	Xoshiro256.h	
	Xoshiro128.cpp
	Xoshiro128.h	
	sdbm_hash.h
	${distinct_sources}
	)

target_compile_definitions(kilipili_common PUBLIC
	ON=1 OFF=0
	PICO_MALLOC_PANIC=0
	PICO_STDIO_ENABLE_CRLF_SUPPORT=1
	)

# add current dir to 'include search path':
target_include_directories(kilipili_common PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
	${distinct_headers}
	)

# dependencies:
target_link_libraries(kilipili_common PUBLIC
	${distinct_libraries}
	)

 
 

