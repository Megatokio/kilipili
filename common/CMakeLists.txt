
cmake_minimum_required(VERSION 3.13)


####################################
# common/CMakeLists.txt 


set(common_sources)
set(common_defines)
set(common_includes)
set(common_libraries)


if(${MAKE_TOOLS})
	set(common_sources
		linux/little_big_endian.h
		linux/glue.cpp
		linux/glue.h
		linux/tempmem.cpp)
	set(common_includes
		${CMAKE_CURRENT_LIST_DIR}/linux)
else()
	set(common_sources 
		timing.cpp
		memory.cpp
		system_clock.cpp
		stack_guard.cpp
		Dispatcher.cpp
		LoadSensor.cpp
		pico/little_big_endian.h
		pico/glue.cpp
		pico/glue.h
		pico/tempmem.cpp)
	set(common_includes 
		${CMAKE_CURRENT_LIST_DIR}/pico)
	set(common_libraries 
		hardware_pwm
		pico_stdlib)
endif()


add_library(kilipili_common STATIC  
	${common_sources}  
	dummy.cpp
	timing.h
	memory.h
	system_clock.h 
	malloc.h
	#malloc.cpp  <-- must be included in add_executable directly
	Led.h
	stack_guard.h 
	Dispatcher.h 
	LoadSensor.h
	Mutex.h
	Logger.h 
	Logger.cpp
	Trace.cpp
	Trace.h
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
	LzhDecoder.cpp
	LzhDecoder.h
	DiskLight.h
)

target_compile_definitions(kilipili_common PUBLIC  
	${common_defines}  
	ON=1 OFF=0
	PICO_MALLOC_PANIC=0
	PICO_STDIO_ENABLE_CRLF_SUPPORT=1
)

target_include_directories(kilipili_common PUBLIC  
	${common_includes} 
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(kilipili_common PUBLIC
	${common_libraries}
)







 
 

