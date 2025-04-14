
cmake_minimum_required(VERSION 3.13)


###############################################
# Audio/CMakeLists.txt


set(AUDIO_DEFAULT_SAMPLE_FREQUENCY 44100 CACHE STRING "default sample frequency")
set(AUDIO_DMA_BUFFER_SIZE 256 CACHE STRING "number of audio frames in dma buffer. 256 ~ 5msec")
option(AUDIO_INPUT_BUFFER_ON_STACK "no static input_buffer[] in fill_buffer() but allocated on stack" OFF)
set(AUDIO_INPUT_BUFFER_SIZE 64 CACHE STRING "size of input_buffer[] in fill_buffer()")

set_property(CACHE AUDIO_DMA_BUFFER_SIZE PROPERTY STRINGS 64 128 256 512 1024)
set_property(CACHE AUDIO_INPUT_BUFFER_SIZE PROPERTY STRINGS 32 48 64)


set(audio_sources)
set(audio_includes)
set(audio_defines)
set(audio_libraries)


if(${MAKE_TOOLS})
	#set(audio_sources)
	#set(audio_includes)
	#set(audio_defines)
	#set(audio_libraries)
else()
	#set(audio_sources)
	#set(audio_includes)
	#set(audio_defines)
	#set(audio_libraries)
endif()


add_library(kilipili_audio STATIC  
	${audio_sources}
	audio_options.h
	AudioSample.h
	AudioSource.h 
	Audio.h 
	Audio.cpp
	i2s_audio.pio
	pwm_audio.pio
	sid_audio.pio
	Ay38912.cpp 
	Ay38912.h
	YMMusicPlayer.h 
	YMMusicPlayer.cpp
)

target_compile_definitions(kilipili_audio PUBLIC  
	${audio_defines}  
	AUDIO_DMA_BUFFER_SIZE=${AUDIO_DMA_BUFFER_SIZE}
	AUDIO_DEFAULT_SAMPLE_FREQUENCY=${AUDIO_DEFAULT_SAMPLE_FREQUENCY}
	AUDIO_INPUT_BUFFER_ON_STACK=${AUDIO_INPUT_BUFFER_ON_STACK}
	AUDIO_INPUT_BUFFER_SIZE=${AUDIO_INPUT_BUFFER_SIZE}
)

target_include_directories(kilipili_audio PUBLIC  
	${audio_includes} 
	${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(kilipili_audio PUBLIC  
	${audio_libraries}
	kilipili_common
	pico_stdlib
	hardware_dma
	hardware_pio
)

if(NOT ${MAKE_TOOLS})
	pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/i2s_audio.pio)
	pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/pwm_audio.pio)
	pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/sid_audio.pio)
endif()



