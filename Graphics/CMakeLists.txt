
cmake_minimum_required(VERSION 3.13)


###############################################
# Graphics/CMakeLists.txt


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
	PicoTerm.h
	PicoTerm.cpp
	)

target_compile_definitions(kilipili_graphics PUBLIC
	)

# add current dir to 'include search path':
target_include_directories(kilipili_graphics PUBLIC
	${CMAKE_CURRENT_LIST_DIR}
	)


if(${UNIT_TEST})

	# dependencies:
	target_link_libraries(kilipili_graphics PUBLIC
		kilipili_common
		)

else()
	
	# dependencies:
	target_link_libraries(kilipili_graphics PUBLIC
		kilipili_common
		pico_stdlib
		)

endif()






