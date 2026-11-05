####################################
# common/CMakeLists.txt 

cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/../boards")
set(PICO_BOARD_HEADER_DIRS ${PICO_BOARD_HEADER_DIRS} CACHE STRING "PICO board header directories" FORCE)
set(PICO_BOARD "vgaboard-i2s" CACHE STRING "the target board, e.g. \"vgaboard-i2s\"")

set(common_sources)
set(common_defines)
set(common_includes)
set(common_libraries)


if(CMAKE_SYSTEM_NAME STREQUAL "PICO")
	set(common_sources 
		memory.cpp
		LoadSensor.cpp
		system_clock.cpp
		stack_guard.cpp)
	set(common_libraries 
		hardware_pwm
		pico_stdlib)
endif()


add_library(kilipili_common STATIC  
	${common_sources}  
	#malloc.cpp  <-- must be included in add_executable directly
	common.h
	Array.h
	Dispatcher.cpp	
	Dispatcher.h 
	Logger.cpp
	Logger.h 
	LoadSensor.h
	DiskLight.h
	Mutex.h
	Queue.h
	RCPtr.h
	SpinLock.h
	trace.cpp
	trace.h
	Xoshiro128.cpp
	Xoshiro128.h	
	Xoshiro256.cpp
	Xoshiro256.h	
	algorithms.h
	basic_math.h
	cdefs.h
	cstrings.cpp
	cstrings.h
	geometry.cpp
	geometry.h
	malloc.h
	no_copy_move.h
	relational_operators.h
	sdbm_hash.h
	sort.h
	stack_guard.h 
	standard_types.h
	template_helpers.h
	tempmem.cpp
	tempmem.h
	timing.cpp
	timing.h
	memory.h
	system_clock.h
	FlexQueue.h
	little_big_endian.h
	glue.cpp
	glue.h
)

target_compile_definitions(kilipili_common PUBLIC  
	${common_defines}  
	ON=1 OFF=0
	PICO_MALLOC_PANIC=0
	PICO_STDIO_ENABLE_CRLF_SUPPORT=1
	${CMAKE_SYSTEM_NAME}
)

target_include_directories(kilipili_common PUBLIC  
	${common_includes} 
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(kilipili_common PUBLIC
	${common_libraries}
)







 
 

