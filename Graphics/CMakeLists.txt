
cmake_minimum_required(VERSION 3.13)


###############################################
# Graphics/CMakeLists.txt


if(${MAKE_TOOLS})
	set(distinct_sources 
		Mock/Pixmap.cpp
		Mock/Pixmap.h
		Mock/TextVDU.cpp
		Mock/TextVDU.h
		)
	set(distinct_libraries 
		)
else()
	set(distinct_sources 
		)
	set(distinct_libraries 
		pico_stdlib)
endif()


add_library(kilipili_graphics STATIC
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
	rsrc/font_12x8.h
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
	${distinct_sources}
	TextVDU.h TextVDU.cpp
	AnsiTerm.cpp AnsiTerm.h
	color_options.h
	Color.h
	ColorMap.h
	ColorMap.cpp
	img/ImageDecoder.h img/ImageDecoder.cpp
	)

target_compile_definitions(kilipili_graphics PUBLIC
	)

# add current dir to 'include search path':
target_include_directories(kilipili_graphics PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)

# dependencies:
target_link_libraries(kilipili_graphics PUBLIC
	kilipili_common
	kilipili_usb_host 	
	${distinct_libraries}
	)


	





