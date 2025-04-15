
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
	#malloc.cpp  <-- must be included in add_executable directly
	Array.h
	DiskLight.h
	Dispatcher.h 
	Led.h
	LoadSensor.h
	Logger.cpp
	Logger.h 
	Mutex.h
	Queue.h
	RCPtr.h
	Trace.cpp
	Trace.h
	Xoshiro128.cpp
	Xoshiro128.h	
	Xoshiro256.cpp
	Xoshiro256.h	
	algorithms.h
	basic_math.h
	cdefs.h
	cstrings.cpp
	cstrings.h
	dummy.cpp
	geometry.cpp
	geometry.h
	malloc.h
	memory.h
	no_copy_move.h
	relational_operators.h
	sdbm_hash.h
	sort.h
	stack_guard.h 
	standard_types.h
	system_clock.h 
	template_helpers.h
	tempmem.h
	timing.h
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







 
 

