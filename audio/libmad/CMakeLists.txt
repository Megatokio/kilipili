###############################################
# extern/libmad/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)


set(libmad_sources)
set(libmad_includes)
set(libmad_defines)
set(libmad_libraries)

if(CMAKE_SYSTEM_NAME STREQUAL "PICO")
else()
endif()


add_library(kilipili_libmad STATIC  
	${libmad_sources}
	Mp3Decoder.cpp
	Mp3Decoder.h
	mad_bitptr.cpp
	fixed.cpp
	MadFrame.cpp
	huffman.cpp
	layer12.cpp
	layer3.cpp
	MadStream.cpp
	MadSynth.cpp
	timer.cpp
	version.cpp
	mad_types.h
	mad_bitptr.h
	fixed.h
	MadFrame.h
	huffman.h
	MadStream.h
	MadSynth.h
	timer.h
	version.h
)

target_compile_definitions(kilipili_libmad PUBLIC  
	${libmad_defines}  
	SIZEOF_INT=4
	FPM_DEFAULT		# most portable
	#OPT_SPEED		# option for FPM_DEFAULT
	OPT_ACCURACY	# option for FPM_DEFAULT
)

target_include_directories(kilipili_libmad PUBLIC  
	${libmad_includes} 
	${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(kilipili_libmad PUBLIC  
	${libmad_libraries}
	kilipili_common
)




