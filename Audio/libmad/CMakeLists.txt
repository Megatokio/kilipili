###############################################
# extern/libmad/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

#set(AUDIO_DEFAULT_SAMPLE_FREQUENCY 44100 CACHE STRING "default sample frequency")
#set(AUDIO_DMA_BUFFER_SIZE 256 CACHE STRING "number of audio frames in dma buffer. 256 ~ 5msec")
#option(AUDIO_INPUT_BUFFER_ON_STACK "no static input_buffer[] in fill_buffer() but allocated on stack" OFF)
#set(AUDIO_INPUT_BUFFER_SIZE 64 CACHE STRING "size of input_buffer[] in fill_buffer()")
#
#set_property(CACHE AUDIO_DMA_BUFFER_SIZE PROPERTY STRINGS 64 128 256 512 1024)
#set_property(CACHE AUDIO_INPUT_BUFFER_SIZE PROPERTY STRINGS 32 48 64)


set(libmad_sources)
set(libmad_includes)
set(libmad_defines)
set(libmad_libraries)


if(${MAKE_TOOLS})
	#set(libmad_sources)
	#set(libmad_includes)
	set(libmad_defines 	
		#FPM_ARM		# v4, but cortex-M0+ which is v6 is not supported
		#FPM_64BIT		# highest precision
		FPM_DEFAULT		# most portable
		OPT_SPEED		# option for FPM_DEFAULT
		#OPT_ACCURACY	# option for FPM_DEFAULT
	)
	#set(libmad_libraries)
else()
	#set(libmad_sources)
	#set(libmad_includes)
	set(libmad_defines 	
		#FPM_ARM		# v4, but cortex-M0+ which is v6 is not supported
		#FPM_64BIT		# highest precision
		FPM_DEFAULT		# most portable
		OPT_SPEED		# option for FPM_DEFAULT
		#OPT_ACCURACY	# option for FPM_DEFAULT
	)
	#set(libmad_libraries)
endif()


add_library(kilipili_libmad STATIC  
	${libmad_sources}
	bit.c
	decoder.c
	fixed.c
	frame.c
	huffman.c
	layer12.c
	layer3.c
	stream.c
	synth.c
	timer.c
	version.c
	bit.h
	decoder.h
	fixed.h
	frame.h
	global.h
	huffman.h
	layer12.h
	layer3.h
	mad.h
	stream.h
	synth.h
	timer.h
	version.h
	#D.D.dat
	#imdct_s.dat
	#qc_table.dat
	#rq_table.dat
	#sf_table.dat
)

target_compile_definitions(kilipili_libmad PUBLIC  
	${libmad_defines}  
	SIZEOF_INT=4
)

target_include_directories(kilipili_libmad PUBLIC  
	${libmad_includes} 
	${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(kilipili_libmad PUBLIC  
	${libmad_libraries}
	# kilipili_common
)




