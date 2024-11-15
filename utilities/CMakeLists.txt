
cmake_minimum_required(VERSION 3.13)


####################################
# utilities/CMakeLists.txt 


add_library(kilipili_utilities STATIC 
	LoadSensor.h
	LoadSensor.cpp
	utilities.h
	utilities.cpp
	Led.h
	#malloc.cpp  <-- must be included in add_executable directly
	Logger.h 
	Logger.cpp
	sm_utilities.h 
	sm_utilities.cpp
	Trace.cpp
	Trace.h
	sm_macros.h
	stack_guard.h stack_guard.cpp
	system_clock.h system_clock.cpp
	Dispatcher.h Dispatcher.cpp
	)

target_compile_definitions(kilipili_utilities PUBLIC 
	ON=1 OFF=0
	)

# add current dir to 'include search path':
target_include_directories(kilipili_utilities PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kilipili_utilities PUBLIC
	kilipili_common
	pico_stdlib
	pico_multicore
	hardware_pwm
	)


	


 
