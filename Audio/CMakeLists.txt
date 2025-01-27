
cmake_minimum_required(VERSION 3.13)


###############################################
# Audio/CMakeLists.txt


set(AUDIO_DEFAULT_SAMPLE_FREQUENCY 44100 CACHE STRING "default sample frequency")
set(AUDIO_DMA_BUFFER_NUM_FRAMES 256 CACHE STRING "number of audio frames in dma buffer. 256 ~ 5msec")

set_property(CACHE AUDIO_DMA_BUFFER_NUM_FRAMES PROPERTY STRINGS 64 128 256 512 1024)

set(audio_sources
	audio_options.h
	AudioSample.h
	AudioSource.h 
	AudioController.h 
	AudioController.cpp
	i2s_audio.pio
	pwm_audio.pio
	sid_audio.pio
	Ay38912.cpp 
	Ay38912.h
	YMMusicPlayer.h 
	YMMusicPlayer.cpp 
)

set(audio_defines
	AUDIO_DMA_BUFFER_NUM_FRAMES=${AUDIO_DMA_BUFFER_NUM_FRAMES}
	AUDIO_DEFAULT_SAMPLE_FREQUENCY=${AUDIO_DEFAULT_SAMPLE_FREQUENCY}
)

set(audio_includes
	${CMAKE_CURRENT_LIST_DIR}
)

set(audio_libraries
	kilipili_common
	kilipili_utilities
	pico_stdlib
	hardware_dma
	hardware_pio
)


if(${MAKE_TOOLS})
	#set(audio_sources)
	#set(audio_headers)
	#set(audio_defines)
	#set(audio_libraries)
else()
	#set(audio_sources)
	#set(audio_headers)
	#set(audio_defines)
	#set(audio_libraries)
endif()


add_library               (kilipili_audio STATIC  ${audio_sources}  )
target_compile_definitions(kilipili_audio PUBLIC  ${audio_defines}  )
target_include_directories(kilipili_audio PUBLIC  ${audio_includes} )
target_link_libraries     (kilipili_audio PUBLIC  ${audio_libraries})


if(NOT ${MAKE_TOOLS})
	pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/i2s_audio.pio)
	pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/pwm_audio.pio)
	pico_generate_pio_header(kilipili_audio ${CMAKE_CURRENT_LIST_DIR}/sid_audio.pio)
endif()
