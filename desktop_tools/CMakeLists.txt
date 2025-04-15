###########################################
# kilipili/desktop_tools/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

project(DesktopTools CXX C ASM)

MESSAGE (STATUS "System = ${CMAKE_SYSTEM_NAME}")   # Linux, PICO
MESSAGE (STATUS "CPU = ${CMAKE_SYSTEM_PROCESSOR}") # x86_64, cortex-m0plus

set(CMAKE_CXX_STANDARD 20)
set(PICO_CXX_ENABLE_EXCEPTIONS 1)
##set(PICO_CXX_ENABLE_RTTI 1)

set(CMAKE_CXX_FLAGS_DEBUG   "${CMAKE_CXX_FLAGS_DEBUG}   -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE -DNDEBUG")
set(CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS}         -DMAKE_TOOLS")
set(CMAKE_CXX_FLAGS          "${CMAKE_CXX_FLAGS}         -DDEBUG -DUNIT_TEST")


set(MAKE_TOOLS ON)
set(PICO_BOARD "vgaboard" CACHE STRING "the target board, e.g. \"vgaboard\"")

add_subdirectory(kilipili/common)
add_subdirectory(kilipili/Graphics)
add_subdirectory(kilipili/USBHost)
add_subdirectory(kilipili/Devices)


add_executable(RsrcFileWriter
	rsrc_writer/main_rsrc_file_writer.cpp
	rsrc_writer/RsrcFileEncoder.cpp
	rsrc_writer/RsrcFileEncoder.h
	rsrc_writer/YMFileConverter.cpp
	rsrc_writer/YMFileConverter.h
	kilipili/common/Array.h
	kilipili/Graphics/color_options.h
	kilipili/Graphics/Color.h
	kilipili/Audio/AudioSource.h
	kilipili/Audio/audio_options.h
	kilipili/Audio/AudioSample.h
	kilipili/Audio/AudioSource.cpp
	kilipili/Audio/Ay38912.cpp
	kilipili/Audio/Ay38912.h
	kilipili/extern/StSoundLibrary/StSoundLibrary.h
	kilipili/extern/StSoundLibrary/digidrum.cpp
	kilipili/extern/StSoundLibrary/digidrum.h
	kilipili/extern/StSoundLibrary/YmTypes.h
	kilipili/extern/StSoundLibrary/Ym2149Ex.cpp
	kilipili/extern/StSoundLibrary/Ym2149Ex.h
	kilipili/extern/StSoundLibrary/Ymload.cpp
	kilipili/extern/StSoundLibrary/YmLoad.h
	kilipili/extern/StSoundLibrary/YmMusic.cpp
	kilipili/extern/StSoundLibrary/YmMusic.h
	kilipili/extern/StSoundLibrary/YmTypes.h
	kilipili/extern/StSoundLibrary/YmUserInterface.cpp
	kilipili/extern/StSoundLibrary/LZH/LZH.H
	kilipili/extern/StSoundLibrary/LZH/LzhLib.cpp
	rsrc_writer/ImageFileWriter.h 
	rsrc_writer/ImageFileWriter.cpp
	rsrc_writer/exportStSoundWavFile.cpp
	rsrc_writer/exportStSoundWavFile.h
	rsrc_writer/RgbImageCompressor.cpp
	rsrc_writer/RgbImageCompressor.h
	rsrc_writer/YMMFileConverter.h 
	rsrc_writer/YMMFileConverter.cpp
	)

target_compile_definitions(RsrcFileWriter PUBLIC
	MAKE_TOOLS=1
	PICO_BOARD_H="boards/${PICO_BOARD}.h"
	)

# add current dir to 'include search path':
target_include_directories(RsrcFileWriter PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/kilipili
	${CMAKE_CURRENT_LIST_DIR}/rsrc_writer
	)

# dependencies. this also adds the include paths:
target_link_libraries(RsrcFileWriter PUBLIC
	kilipili_common 
	kilipili_devices
	z
	)



add_executable(UnitTest
	unit_test/main_unit_test.cpp
	unit_test/Pixmap_unit_test.cpp
	unit_test/Canvas_unit_test.cpp
	unit_test/BitBlit_unit_test.cpp
	unit_test/TextVDU_unit_test.cpp
	unit_test/AnsiTerm_unit_test.cpp
	unit_test/AudioSource_unit_test.cpp
	unit_test/AudioSample_unit_test.cpp
	unit_test/tempmem.test.cpp
	unit_test/cstrings.test.cpp
	unit_test/relational_operators.test.cpp
	unit_test/Array.test.cpp
	unit_test/Ay38912_unit_test.cpp
	unit_test/QspiFlash_unit_test.cpp
	unit_test/common_unit_test.cpp
	kilipili/Devices/Flash.h
	kilipili/Devices/Flash.cpp
	kilipili/Devices/BlockDevice.cpp
	kilipili/Devices/QspiFlashDevice.cpp
	kilipili/Devices/QspiFlashDevice.h
	kilipili/Devices/Preferences.cpp
	kilipili/Devices/Preferences.h
	kilipili/Audio/AudioSource.h
	kilipili/Audio/AudioSource.cpp
	kilipili/Audio/audio_options.h
	kilipili/Audio/AudioSample.h
	kilipili/Audio/Ay38912.cpp
	kilipili/Audio/Ay38912.h
	unit_test/Mock/MockFlash.h
	unit_test/Mock/MockFlash.cpp
	unit_test/Mock/MockPixmap.cpp
	unit_test/Mock/MockPixmap.h
	unit_test/Mock/MockTextVDU.cpp
	unit_test/Mock/MockTextVDU.h
	unit_test/Mock/mock_hid_handler.cpp
	unit_test/Mock/mock_hid_handler.h
	)

target_compile_definitions(UnitTest PUBLIC
	MAKE_TOOLS=1
	UNIT_TEST=1
	FLASH_PREFERENCES=${FLASH_PREFERENCES}
	YM_FILE="${CMAKE_CURRENT_LIST_DIR}/test_files/Ninja Spirits  5.ym"
	)

# add current dir to 'include search path':
target_include_directories(UnitTest PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/kilipili
	${CMAKE_CURRENT_LIST_DIR}/unit_test
	${CMAKE_CURRENT_LIST_DIR}/unit_test/Mock
	)

# dependencies. this also adds the include paths:
target_link_libraries(UnitTest PUBLIC
	kilipili_common
	kilipili_graphics
	kilipili_usb_host
	z
	)



add_executable(GifCompressionTest
	compression_test/main_gif_compression_test.cpp
	kilipili/Graphics/color_options.h
	kilipili/Graphics/Color.h
	)

target_compile_definitions(GifCompressionTest PUBLIC
	MAKE_TOOLS=1
	PICO_BOARD="${PICO_BOARD_HEADER_DIRS}/${PICO_BOARD}.h"
	)

# add current dir to 'include search path':
target_include_directories(GifCompressionTest PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/kilipili
	${CMAKE_CURRENT_LIST_DIR}/compression_test
	${CMAKE_CURRENT_LIST_DIR}/rsrc_writer
	)

# dependencies. this also adds the include paths:
target_link_libraries(GifCompressionTest PUBLIC
	kilipili_common
	kilipili_graphics
	kilipili_devices
	z
	)



add_executable(RGBCompressionTest
	kilipili/Graphics/color_options.h
	kilipili/Graphics/Color.h
	compression_test/main_rgb_compression_test.cpp
	rsrc_writer/RgbImageCompressor.cpp
	rsrc_writer/RgbImageCompressor.h
	)

target_compile_definitions(RGBCompressionTest PUBLIC
	MAKE_TOOLS=1
	PICO_BOARD="${PICO_BOARD_HEADER_DIRS}/${PICO_BOARD}.h"
	)

# add current dir to 'include search path':
target_include_directories(RGBCompressionTest PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/kilipili
	${CMAKE_CURRENT_LIST_DIR}/compression_test
	${CMAKE_CURRENT_LIST_DIR}/rsrc_writer
	)

# dependencies. this also adds the include paths:
target_link_libraries(RGBCompressionTest PUBLIC
	kilipili_common
	kilipili_graphics
	kilipili_devices
	z
	)



























