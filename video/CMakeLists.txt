###############################################
# Video/CMakeLists.txt

cmake_minimum_required(VERSION 3.13)

option(VIDEO_OPTIMISTIC_A1W8 "optimize a1w8_rgb mode for moderate color usage. makes 1280x768 work. otherwise not recommended." OFF)
option(VIDEO_SUPPORT_200x150_A1W8 "support int8 bitmap width in a1w8 renderer. slightly slower than otherwise." ON)
option(VIDEO_SUPPORT_400x300_A1W8 "support int16 bitmap width in a1w8 renderer. slightly slower than otherwise." ON)

set(VIDEO_MAX_SYSCLOCK_MHz "290" CACHE STRING "maximum system clock set by VideoBackend.start()")
set(VIDEO_MAX_SCANLINE_BUFFERS "16" CACHE STRING "max. number of prepared scanlines (2^N)")
set(VIDEO_SCANLINE_RENDERER_SECTION "XRAM" CACHE STRING "section for predefined scanline renderers: RAM or XRAM")

set_property(CACHE VIDEO_MAX_SCANLINE_BUFFERS PROPERTY STRINGS "2" "4" "8" "16" "32")
set_property(CACHE VIDEO_SCANLINE_RENDERER_SECTION PROPERTY STRINGS "RAM" "XRAM")


set(video_sources)
set(video_includes)
set(video_defines)
set(video_libraries)


if(CMAKE_SYSTEM_NAME STREQUAL "PICO")
	set(video_libraries
		hardware_pio
		hardware_dma
		pico_multicore
		hardware_interp
		hardware_exception)
	
	set(video_sources
		internal/scanline.pio
		internal/timing.pio
		internal/VideoBackend.h  	
		internal/VideoBackend.cpp
		internal/ScanlineBuffer.h 	
		internal/ScanlineBuffer.cpp
		internal/scanline.pio.h
		internal/timing.pio.h)
endif()


add_library(kilipili_video STATIC  
	${video_sources}  
	AnimatedSprite.h
	MousePointer.h  	
	MousePointer.cpp
	MultiSpritesPlane.h	
	MultiSpritesPlane.cpp
	SingleSpritePlane.h	
	SingleSpritePlane.cpp
	Sprite.cpp      	
	Sprite.h
	VgaMode.h
	internal/video_options.h
	internal/Shape.h
	internal/Frames.h
	internal/Interp.h  
	FrameBuffer.h 
	FrameBuffer<Pixmap>.h   	
	FrameBuffer<Pixmap>.cpp
	VideoPlane.h    	
	VideoPlane.cpp
	VideoController.h	
	VideoController.cpp
	HamImageInfo.h  	
	HamImageInfo.cpp
	Passepartout.h  	
	Passepartout.cpp
	VerticalLayout.h	
	VerticalLayout.cpp
	HorizontalLayout.h	
	HorizontalLayout.cpp
	internal/ScanlineRenderer.h	
	internal/ScanlineRenderer.cpp
	UniColorBackdrop.h 	
	UniColorBackdrop.cpp  
	HamImageVideoPlane.h 
	HamImageVideoPlane.cpp
	FrameBuffer<SimpleCharMap>.h 
	FrameBuffer<SimpleCharMap>.cpp
	FrameBuffer<CharMap>.h 
	FrameBuffer<CharMap>.cpp
)

target_compile_definitions(kilipili_video PUBLIC  
	${video_defines}  
	VIDEO_OPTIMISTIC_A1W8=${VIDEO_OPTIMISTIC_A1W8}
	VIDEO_SUPPORT_200x150_A1W8=${VIDEO_SUPPORT_200x150_A1W8}
	VIDEO_SUPPORT_400x300_A1W8=${VIDEO_SUPPORT_400x300_A1W8}
	VIDEO_MAX_SYSCLOCK_MHz=${VIDEO_MAX_SYSCLOCK_MHz}
	VIDEO_MAX_SCANLINE_BUFFERS=${VIDEO_MAX_SCANLINE_BUFFERS}
	VIDEO_SCANLINE_RENDERER_SECTION=${VIDEO_SCANLINE_RENDERER_SECTION}
	${CMAKE_SYSTEM_NAME}
)

target_include_directories(kilipili_video PUBLIC  
	${video_includes} 
	${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(kilipili_video PUBLIC  
	${video_libraries}
	kilipili_common
	kilipili_graphics
)

# the resulting header files are already included in internal/:
# pico_generate_pio_header(kilipili_video ${CMAKE_CURRENT_LIST_DIR}/internal/timing.pio)
# pico_generate_pio_header(kilipili_video ${CMAKE_CURRENT_LIST_DIR}/internal/scanline.pio)






































