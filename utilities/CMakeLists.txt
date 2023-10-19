
cmake_minimum_required(VERSION 3.13)


####################################
# utilities/CMakeLists.txt 


add_library(kilipili_utilities STATIC 
	PwmLoadSensor.h
	PwmLoadSensor.cpp
	utilities.h
	utilities.cpp
	Led.h
	Led.cpp
	#malloc.cpp  <-- must be included in add_executable directly
	BucketList.h
	tempmem.cpp
	tempmem.h
	Logger.h 
	Logger.cpp
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


	


 
