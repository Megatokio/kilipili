
cmake_minimum_required(VERSION 3.13)


####################################
# utilities/CMakeLists.txt 


set(utilities_sources
	LoadSensor.h
	LoadSensor.cpp
	utilities.h
	utilities.cpp
	Led.h
	#malloc.cpp  <-- must be included in add_executable directly
	Logger.h 
	Logger.cpp
	Trace.cpp
	Trace.h
	stack_guard.h stack_guard.cpp
	system_clock.h system_clock.cpp
	Dispatcher.h Dispatcher.cpp
	malloc.h
)

set(utilities_defines
	ON=1 OFF=0
)

set(utilities_includes PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
)

set(utilities_libraries
	kilipili_common
	pico_stdlib
	pico_multicore
	hardware_pwm
)


if(${MAKE_TOOLS})
	#set(utilities_sources)
	#set(utilities_headers)
	#set(utilities_defines)
	#set(utilities_libraries)
else()
	#set(utilities_sources)
	#set(utilities_headers)
	#set(utilities_defines)
	#set(utilities_libraries)	
endif()


add_library               (kilipili_utilities STATIC  ${utilities_sources}  )
target_compile_definitions(kilipili_utilities PUBLIC  ${utilities_defines}  )
target_include_directories(kilipili_utilities PUBLIC  ${utilities_includes} )
target_link_libraries     (kilipili_utilities PUBLIC  ${utilities_libraries})
	

	


 
