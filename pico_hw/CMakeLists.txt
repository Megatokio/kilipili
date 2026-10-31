####################################
# common/CMakeLists.txt 

cmake_minimum_required(VERSION 3.13)

set(pico_hw_sources)
set(pico_hw_defines)
set(pico_hw_includes)
set(pico_hw_libraries)


if(${MAKE_TOOLS})
else()
	set(pico_hw_sources				
		DiskLight.h
		Interp.h
		Led.h
		LoadSensor.cpp
		LoadSensor.h
		memory.cpp
		memory.h
		pico_hw.h
		SpinLock.h
		system_clock.cpp
		system_clock.h
		temperature_sensor.h)
	set(pico_hw_libraries 
		#pico_multicore
		#hardware_pio
		#hardware_dma
		#hardware_irq
		#hardware_spi
		#hardware_rtc
		#hardware_i2c
		hardware_pwm
		hardware_adc
		hardware_interp		
		pico_stdlib)
endif()


add_library(kilipili_pico_hw STATIC  
	${pico_hw_sources}  
	SpinLock.h
	Interp.h
	LoadSensor.h
)

target_compile_definitions(kilipili_pico_hw PUBLIC  
	${pico_hw_defines}  
	ON=1 OFF=0
)

target_include_directories(kilipili_pico_hw PUBLIC  
	${pico_hw_includes} 
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(kilipili_pico_hw PUBLIC
	${pico_hw_libraries}
)







 
 

