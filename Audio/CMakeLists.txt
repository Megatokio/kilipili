
cmake_minimum_required(VERSION 3.13)


###############################################
# Audio/CMakeLists.txt


####################################################################
# OPTIONS:

set(AUDIO_DEFAULT_SAMPLE_FREQUENCY 44100 CACHE STRING "default sample frequency")
set(AUDIO_DMA_BUFFER_NUM_FRAMES 256 CACHE STRING "number of audio frames in dma buffer. 256 ~ 5msec")

set_property(CACHE AUDIO_DMA_BUFFER_NUM_FRAMES PROPERTY STRINGS 64 128 256 512 1024)

add_library(kilipili_audio STATIC
	audio_options.h
	AudioSample.h
	AudioSource.h 
	AudioController.h 
	AudioController.cpp
	i2s_audio.pio
	pwm_audio.pio
	sid_audio.pio
)

target_compile_definitions(kilipili_audio PUBLIC
	AUDIO_DMA_BUFFER_NUM_FRAMES=${AUDIO_DMA_BUFFER_NUM_FRAMES}
	AUDIO_DEFAULT_SAMPLE_FREQUENCY=${AUDIO_DEFAULT_SAMPLE_FREQUENCY}
)

# add current dir to 'include search path':
target_include_directories(kilipili_audio PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
)

# if there is a PIO assembler file:
pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/i2s_audio.pio)
pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/pwm_audio.pio)
pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/sid_audio.pio)

# dependencies:
target_link_libraries(kilipili_audio PUBLIC 
	kilipili_common
	kilipili_utilities
	pico_stdlib
	hardware_dma
	hardware_pio
	#pico_multicore
	#hardware_interp
	#hardware_exception
)


 



