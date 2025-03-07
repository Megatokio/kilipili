
cmake_minimum_required(VERSION 3.13)


###############################################
# Video/CMakeLists.txt


option(VIDEO_OPTIMISTIC_A1W8 "optimize a1w8_rgb mode for moderate color usage. makes 1280x768 work. otherwise not recommended." OFF)
option(VIDEO_SUPPORT_200x150_A1W8 "support int8 bitmap width in a1w8 renderer. slightly slower than otherwise." ON)
option(VIDEO_SUPPORT_400x300_A1W8 "support int16 bitmap width in a1w8 renderer. slightly slower than otherwise." ON)

set(VIDEO_MAX_SYSCLOCK_MHz "290" CACHE STRING "maximum system clock set by VideoBackend.start()")
set(VIDEO_MAX_SCANLINE_BUFFERS "16" CACHE STRING "max. number of prepared scanlines (2^N)")
set(VIDEO_INTERP0_MODE "5"  CACHE STRING "configure interp0: -1=any, 0…3: 1,2,4,8 bit/pixel, 5: 2 bit/2px (default)")
set(VIDEO_INTERP1_MODE "-1" CACHE STRING "configure interp1: -1=any (default), 0…3: 1,2,4,8 bit/pixel, 5: 2 bit/2px")

set_property(CACHE VIDEO_MAX_SCANLINE_BUFFERS PROPERTY STRINGS "2" "4" "8" "16" "32")
set_property(CACHE VIDEO_INTERP1_MODE PROPERTY STRINGS "-1" 0 1 2 3 5)
set_property(CACHE VIDEO_INTERP1_MODE PROPERTY STRINGS "-1" 0 1 2 3 5)


set(video_sources
	FrameBuffer.h
	FrameBuffer.cpp
	MousePointer.cpp
	MousePointer.h
	ScanlineRenderer.cpp
	ScanlineRenderer.h
	VideoPlane.h
	VideoPlane.cpp
	VideoController.cpp
	VideoController.h
	VgaMode.h
	scanline.pio
	video_options.h
	timing.pio
	ScanlineBuffer.h 
	ScanlineBuffer.cpp
	VideoBackend.cpp
	VideoBackend.h
	Shape.h
	Sprite.h
	Sprite.cpp
	AnimatedSprite.h
	SingleSpritePlane.h
	SingleSpritePlane.cpp
	MultiSpritesPlane.cpp
	MultiSpritesPlane.h
	UniColorBackdrop.h
	UniColorBackdrop.cpp  
	Frames.h
	HamImageVideoPlane.h 
	HamImageVideoPlane.cpp
	HamImageInfo.h 
	HamImageInfo.cpp
	Passepartout.h Passepartout.cpp
	Interp.h  
	VerticalLayout.h VerticalLayout.cpp
	HorizontalLayout.h HorizontalLayout.cpp
)

set(video_defines PUBLIC
	VIDEO_OPTIMISTIC_A1W8=${VIDEO_OPTIMISTIC_A1W8}
	VIDEO_SUPPORT_200x150_A1W8=${VIDEO_SUPPORT_200x150_A1W8}
	VIDEO_SUPPORT_400x300_A1W8=${VIDEO_SUPPORT_400x300_A1W8}
	VIDEO_MAX_SYSCLOCK_MHz=${VIDEO_MAX_SYSCLOCK_MHz}
	VIDEO_MAX_SCANLINE_BUFFERS=${VIDEO_MAX_SCANLINE_BUFFERS}
	VIDEO_INTERP0_MODE=${VIDEO_INTERP0_MODE}
	VIDEO_INTERP1_MODE=${VIDEO_INTERP1_MODE}
)

set(video_includes PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
)

set(video_libraries
	kilipili_common
	kilipili_utilities
	kilipili_graphics
	pico_stdlib
	hardware_pio
	hardware_dma
	pico_multicore
	hardware_interp
	hardware_exception
)


if(${MAKE_TOOLS})
	#set(video_sources)
	#set(video_headers)
	#set(video_defines)
	#set(video_libraries)
else()
	#set(video_sources)
	#set(video_headers)
	#set(video_defines)
	#set(video_libraries)	
endif()


add_library               (kilipili_video STATIC  ${video_sources}  )
target_compile_definitions(kilipili_video PUBLIC  ${video_defines}  )
target_include_directories(kilipili_video PUBLIC  ${video_includes} )
target_link_libraries     (kilipili_video PUBLIC  ${video_libraries})

pico_generate_pio_header(kilipili_video ${CMAKE_CURRENT_LIST_DIR}/timing.pio)
pico_generate_pio_header(kilipili_video ${CMAKE_CURRENT_LIST_DIR}/scanline.pio)



































