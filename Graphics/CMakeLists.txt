
cmake_minimum_required(VERSION 3.13)


###############################################
# Graphics/CMakeLists.txt


set(graphics_sources)
set(graphics_includes)
set(graphics_defines)
set(graphics_libraries)


if(${MAKE_TOOLS})
else()
endif()


add_library(kilipili_graphics STATIC  
	${graphics_sources}  
	fixint.h
	graphics_types.h
	color_options.h
	Color.h
	gif/GifArray.h
	gif/Colormap.cpp	
	gif/Colormap.h
	gif/GifDecoder.cpp	
	gif/GifDecoder.h
	gif/GifEncoder.cpp	
	gif/GifEncoder.h
	gif/GifReport.cpp	
	gif/GifReport.h
	gif/Pixelmap.cpp	
	gif/Pixelmap.h
	img/ImageDecoder.h	
	img/ImageDecoder.cpp
	Canvas.h	
	Canvas.cpp
	Pixmap.h	
	Pixmap.cpp
	TextVDU.h	
	TextVDU.cpp
	BitBlit.h	
	BitBlit.cpp
	ColorMap.h	
	ColorMap.cpp
	Pixmap_wAttr.h 
	Pixmap_wAttr.cpp
	systemfont.cpp 
	systemfont.h
	AnsiTerm.cpp 
	AnsiTerm.h
	graphics.h
)

target_compile_definitions(kilipili_graphics PUBLIC  
	${graphics_defines}  
)

target_include_directories(kilipili_graphics PUBLIC  
	${graphics_includes} 
	${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(kilipili_graphics PUBLIC  
	${graphics_libraries}
	kilipili_common
)
	






