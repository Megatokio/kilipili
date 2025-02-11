
cmake_minimum_required(VERSION 3.13)


###############################################
# Graphics/CMakeLists.txt


set(graphics_sources
	fixint.h
	Canvas.h
	Canvas.cpp
	Pixmap.h
	Pixmap.cpp
	Pixmap_wAttr.h
	Pixmap_wAttr.cpp
	graphics_types.h
	BitBlit.h
	BitBlit.cpp
	systemfont.cpp
	systemfont.h
	gif/Colormap.cpp
	gif/Colormap.h
	gif/GifArray.h
	gif/GifDecoder.cpp
	gif/GifDecoder.h
	gif/GifEncoder.cpp
	gif/GifEncoder.h
	gif/GifReport.cpp
	gif/GifReport.h
	gif/Pixelmap.cpp
	gif/Pixelmap.h
	TextVDU.h TextVDU.cpp
	AnsiTerm.cpp AnsiTerm.h
	color_options.h
	Color.h
	ColorMap.h
	ColorMap.cpp
	img/ImageDecoder.h img/ImageDecoder.cpp
)

set(graphics_defines
)

set(graphics_includes
	${CMAKE_CURRENT_LIST_DIR}
)

set(graphics_libraries
	kilipili_common
	kilipili_usb_host 	
)


if(${MAKE_TOOLS})
	#set(graphics_sources)
	#set(graphics_headers)
	#set(graphics_defines)
	#set(graphics_libraries)
else()
	#set(graphics_sources)
	#set(graphics_headers)
	#set(graphics_defines)
	#set(graphics_libraries)	
endif()


add_library               (kilipili_graphics STATIC  ${graphics_sources}  )
target_compile_definitions(kilipili_graphics PUBLIC  ${graphics_defines}  )
target_include_directories(kilipili_graphics PUBLIC  ${graphics_includes} )
target_link_libraries     (kilipili_graphics PUBLIC  ${graphics_libraries})
	





