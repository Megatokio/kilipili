
cmake_minimum_required(VERSION 3.13)


####################################
# utilities/CMakeLists.txt 


set(TEMPMEM_POOL_SIZE_CORE1 "2000" CACHE STRING "size of pool for temporary strings")
set(TEMPMEM_POOL_SIZE_CORE2 "0" CACHE STRING "size of pool for temporary strings")


add_library(kilipili_utilities STATIC 
	PwmLoadSensor.h
	PwmLoadSensor.cpp
	utilities.h
	utilities.cpp
	Led.h
	#malloc.cpp  <-- must be included in add_executable directly
	BucketList.h
	tempmem.cpp
	Logger.h 
	Logger.cpp
	sm_utilities.h 
	sm_utilities.cpp
	)

target_compile_definitions(kilipili_utilities PUBLIC 
	ON=1 OFF=0
	TEMPMEM_POOL_SIZE_CORE1=${TEMPMEM_POOL_SIZE_CORE1}
	TEMPMEM_POOL_SIZE_CORE2=${TEMPMEM_POOL_SIZE_CORE2}
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


	


 
