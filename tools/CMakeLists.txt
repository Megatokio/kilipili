###########################################
# kilipili/tools/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

#project(RsrcFileWriter CXX C ASM)

#MESSAGE (STATUS "System = ${CMAKE_SYSTEM_NAME}")   # Linux, PICO
#MESSAGE (STATUS "CPU = ${CMAKE_SYSTEM_PROCESSOR}") # x86_64, cortex-m0plus

#set(CMAKE_CXX_STANDARD 20)
#set(PICO_CXX_ENABLE_EXCEPTIONS 1)
##set(PICO_CXX_ENABLE_RTTI 1)

#set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -DDEBUG")
#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE -DNDEBUG")
set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS}         -DMAKE_TOOLS")

	 
# why has cmake no protection from double inclusion of a sub directory?
if(NOT UNIT_TEST)
add_subdirectory(common)
endif() 

add_executable(RsrcFileWriter
	main_rsrc_file_writer.cpp
	../Audio/AudioSource.h
	../Audio/audio_options.h
	../Audio/AudioSample.h
	../Audio/AudioSource.cpp 		
	../Audio/Ay38912.cpp
	../Audio/Ay38912.h
	RsrcFileWriter.cpp
	RsrcFileWriter.h
	YMFileConverter.cpp
	YMFileConverter.h
	StSoundLibrary/StSoundLibrary.h
	StSoundLibrary/digidrum.cpp
	StSoundLibrary/digidrum.h
	StSoundLibrary/YmTypes.h
	StSoundLibrary/Ym2149Ex.cpp
	StSoundLibrary/Ym2149Ex.h
	StSoundLibrary/Ymload.cpp
	StSoundLibrary/YmLoad.h
	StSoundLibrary/YmMusic.cpp
	StSoundLibrary/YmMusic.h
	StSoundLibrary/YmTypes.h
	StSoundLibrary/YmUserInterface.cpp
	StSoundLibrary/LZH/LZH.H
	StSoundLibrary/LZH/LzhLib.cpp		
	)

target_compile_definitions(RsrcFileWriter PUBLIC
	#YM_FILE="test_files/solom3.ym"
	YM_FILE="test_files/Ninja Spirits  5.ym"
	YM_DIR="Audio/rsrc/ym_files/"
	#YM_DIR="/mnt/WD2/ym-jukebox.git/data"
	YM_WHAT=What::RSRC
	)

# add current dir to 'include search path':
target_include_directories(RsrcFileWriter PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	${CMAKE_CURRENT_LIST_DIR}/..
	)

# dependencies. this also adds the include paths:
target_link_libraries(RsrcFileWriter PUBLIC
	kilipili_common 
	z
	)





