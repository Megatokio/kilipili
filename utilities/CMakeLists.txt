
cmake_minimum_required(VERSION 3.13)


####################################
# utilities/CMakeLists.txt 


set(utilities_sources)
set(utilities_defines)
set(utilities_includes)
set(utilities_libraries)


if(${MAKE_TOOLS})
else()
	set(utilities_libraries
		pico_stdlib
		pico_multicore
		hardware_pwm
	)
endif()


add_library(kilipili_utilities STATIC  
	${utilities_sources}  
	LoadSensor.h
	LoadSensor.cpp
	utilities.h
	utilities.cpp
	Led.h
	malloc.h
	#malloc.cpp  <-- must be included in add_executable directly
	Logger.h 
	Logger.cpp
	Trace.cpp
	Trace.h
	stack_guard.h stack_guard.cpp
	system_clock.h system_clock.cpp
	Dispatcher.h Dispatcher.cpp
)

target_compile_definitions(kilipili_utilities PUBLIC  
	${utilities_defines}  
	ON=1 OFF=0
)

target_include_directories(kilipili_utilities PUBLIC  
	${CMAKE_CURRENT_LIST_DIR}
	${utilities_includes} 
)

target_link_libraries(kilipili_utilities PUBLIC  
	${utilities_libraries}
	kilipili_common
)

	

	


 
